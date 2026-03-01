#pragma once

#include "system_code.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>

namespace outcome::core {

// ============================================================================
// success_type / failure_type — construction helpers
// ============================================================================
template <class T> struct [[nodiscard]] success_type {
  using value_type = T;

  T _value;
  uint16_t _spare_storage{0};

  template <class U>
    requires std::constructible_from<T, U>
  constexpr explicit success_type(U &&v, uint16_t spare = 0) noexcept(
      std::is_nothrow_constructible_v<T, U>)
      : _value(static_cast<U &&>(v)), _spare_storage(spare) {}

  constexpr T &value() & noexcept { return _value; }
  constexpr const T &value() const & noexcept { return _value; }
  constexpr T &&value() && noexcept { return static_cast<T &&>(_value); }
  constexpr const T &&value() const && noexcept {
    return static_cast<const T &&>(_value);
  }
  constexpr uint16_t spare_storage() const noexcept { return _spare_storage; }
};

template <> struct [[nodiscard]] success_type<void> {
  using value_type = void;
  constexpr uint16_t spare_storage() const noexcept { return 0; }
};

template <class EC, class E = void> struct [[nodiscard]] failure_type;

template <class EC> struct [[nodiscard]] failure_type<EC, void> {
  using error_type = EC;
  using exception_type = void;

  EC _error;
  uint16_t _spare_storage{0};

  template <class U>
    requires std::constructible_from<EC, U>
  constexpr explicit failure_type(U &&v, uint16_t spare = 0) noexcept(
      std::is_nothrow_constructible_v<EC, U>)
      : _error(static_cast<U &&>(v)), _spare_storage(spare) {}

  constexpr EC &error() & noexcept { return _error; }
  constexpr const EC &error() const & noexcept { return _error; }
  constexpr EC &&error() && noexcept { return static_cast<EC &&>(_error); }
  constexpr bool has_error() const noexcept { return true; }
  constexpr bool has_exception() const noexcept { return false; }
  constexpr uint16_t spare_storage() const noexcept { return _spare_storage; }
};

template <class EC, class E> struct [[nodiscard]] failure_type {
  using error_type = EC;
  using exception_type = E;

  EC _error;
  E _exception;
  bool _have_error{false};
  bool _have_exception{false};
  uint16_t _spare_storage{0};

  template <class U, class V>
  constexpr failure_type(U &&u, V &&v, uint16_t spare = 0)
      : _error(static_cast<U &&>(u)), _exception(static_cast<V &&>(v)),
        _have_error(true), _have_exception(true), _spare_storage(spare) {}

  template <class U>
  constexpr failure_type(std::in_place_type_t<error_type>, U &&u,
                         uint16_t spare = 0)
      : _error(static_cast<U &&>(u)), _exception{}, _have_error(true),
        _have_exception(false), _spare_storage(spare) {}

  template <class V>
  constexpr failure_type(std::in_place_type_t<exception_type>, V &&v,
                         uint16_t spare = 0)
      : _error{}, _exception(static_cast<V &&>(v)), _have_error(false),
        _have_exception(true), _spare_storage(spare) {}

  constexpr bool has_error() const noexcept { return _have_error; }
  constexpr bool has_exception() const noexcept { return _have_exception; }
  constexpr EC &error() & noexcept { return _error; }
  constexpr const EC &error() const & noexcept { return _error; }
  constexpr E &exception() & noexcept { return _exception; }
  constexpr const E &exception() const & noexcept { return _exception; }
  constexpr uint16_t spare_storage() const noexcept { return _spare_storage; }
};

// Factory functions
inline constexpr success_type<void> success() noexcept { return {}; }

template <class T>
constexpr success_type<std::decay_t<T>> success(T &&v, uint16_t spare = 0) {
  return success_type<std::decay_t<T>>(static_cast<T &&>(v), spare);
}

template <class EC>
constexpr failure_type<std::decay_t<EC>> failure(EC &&v, uint16_t spare = 0) {
  return failure_type<std::decay_t<EC>>(static_cast<EC &&>(v), spare);
}

template <class EC, class E>
constexpr failure_type<std::decay_t<EC>, std::decay_t<E>>
failure(EC &&ec, E &&e, uint16_t spare = 0) {
  return failure_type<std::decay_t<EC>, std::decay_t<E>>(
      static_cast<EC &&>(ec), static_cast<E &&>(e), spare);
}

template <class T> struct is_success_type : std::false_type {};
template <class T> struct is_success_type<success_type<T>> : std::true_type {};

template <class T> struct is_failure_type : std::false_type {};
template <class EC, class E>
struct is_failure_type<failure_type<EC, E>> : std::true_type {};

// ============================================================================
// status_bitfield_type — compact status bits
// ============================================================================
struct status_bitfield_type {
  uint32_t _status{0};

  constexpr bool have_value() const noexcept { return (_status & 1U) != 0; }
  constexpr bool have_error() const noexcept { return (_status & 2U) != 0; }
  constexpr bool have_exception() const noexcept { return (_status & 4U) != 0; }
  constexpr bool have_error_is_errno() const noexcept {
    return (_status & 8U) != 0;
  }
  constexpr bool have_moved_from() const noexcept {
    return (_status & 16U) != 0;
  }
  constexpr bool have_lost_consistency() const noexcept {
    return (_status & 32U) != 0;
  }

  constexpr void set_have_value(bool v) noexcept {
    _status = v ? (_status | 1U) : (_status & ~1U);
  }
  constexpr void set_have_error(bool v) noexcept {
    _status = v ? (_status | 2U) : (_status & ~2U);
  }
  constexpr void set_have_exception(bool v) noexcept {
    _status = v ? (_status | 4U) : (_status & ~4U);
  }
  constexpr void set_have_error_is_errno(bool v) noexcept {
    _status = v ? (_status | 8U) : (_status & ~8U);
  }
  constexpr void set_have_moved_from(bool v) noexcept {
    _status = v ? (_status | 16U) : (_status & ~16U);
  }
  constexpr void set_have_lost_consistency(bool v) noexcept {
    _status = v ? (_status | 32U) : (_status & ~32U);
  }

  constexpr uint16_t spare_storage_value() const noexcept {
    return static_cast<uint16_t>(_status >> 16);
  }
  constexpr void set_spare_storage_value(uint16_t v) noexcept {
    _status = (_status & 0xFFFFU) | (static_cast<uint32_t>(v) << 16);
  }
};

// ============================================================================
// value_storage
// ============================================================================
namespace detail {

struct empty_type {};

template <class T>
using devoid = std::conditional_t<std::is_void_v<T>, empty_type, T>;

template <class R, class EC> struct value_storage {
  using value_type = devoid<R>;
  using error_type = devoid<EC>;

  union {
    value_type _value;
    error_type _error;
  };
  status_bitfield_type _status;

  constexpr value_storage() noexcept : _value{}, _status{} {}

  template <class... Args>
    requires std::constructible_from<value_type, Args...>
  constexpr explicit value_storage(
      std::in_place_type_t<value_type>,
      Args &&...args) noexcept(std::is_nothrow_constructible_v<value_type,
                                                               Args...>)
      : _value(static_cast<Args &&>(args)...), _status{} {
    _status.set_have_value(true);
  }

  template <class... Args>
    requires std::constructible_from<error_type, Args...>
  constexpr explicit value_storage(
      std::in_place_type_t<error_type>,
      Args &&...args) noexcept(std::is_nothrow_constructible_v<error_type,
                                                               Args...>)
      : _error(static_cast<Args &&>(args)...), _status{} {
    _status.set_have_error(true);
  }

  constexpr value_storage(const value_storage &o) = default;
  constexpr value_storage(value_storage &&o) = default;
  constexpr value_storage &operator=(const value_storage &) = default;
  constexpr value_storage &operator=(value_storage &&) = default;

  // Custom destructor for non-trivially-destructible union members
  constexpr ~value_storage()
    requires(std::is_trivially_destructible_v<value_type> &&
             std::is_trivially_destructible_v<error_type>)
  = default;

  constexpr ~value_storage()
    requires(!(std::is_trivially_destructible_v<value_type> &&
               std::is_trivially_destructible_v<error_type>))
  {
    if (_status.have_value()) {
      _value.~value_type();
    } else if (_status.have_error()) {
      _error.~error_type();
    }
  }
};

} // namespace detail

// ============================================================================
// Forward declarations needed for hooks and policies
// ============================================================================
template <class R, class S, class NoValuePolicy> class basic_result;
template <class R, class S, class P, class NoValuePolicy> class basic_outcome;

// ============================================================================
// Policies
// ============================================================================
namespace policy {

struct base {
  template <class Impl> static constexpr bool _has_value(Impl &&self) noexcept {
    return self._state._status.have_value();
  }

  template <class Impl> static constexpr bool _has_error(Impl &&self) noexcept {
    return self._state._status.have_error();
  }

  template <class Impl>
  static constexpr bool _has_exception(Impl &&self) noexcept {
    return self._state._status.have_exception();
  }

  template <class Impl> static constexpr auto &&_value(Impl &&self) noexcept {
    return static_cast<Impl &&>(self)._state._value;
  }

  template <class Impl> static constexpr auto &&_error(Impl &&self) noexcept {
    return static_cast<Impl &&>(self)._state._error;
  }

  template <class Impl>
  static constexpr void narrow_value_check(Impl && /*self*/) noexcept {}

  template <class Impl>
  static constexpr void narrow_error_check(Impl && /*self*/) noexcept {}

  template <class Impl>
  static constexpr void narrow_exception_check(Impl && /*self*/) noexcept {}
};

struct all_narrow : base {
  template <class Impl>
  static constexpr void wide_value_check(Impl &&) noexcept {}
  template <class Impl>
  static constexpr void wide_error_check(Impl &&) noexcept {}
  template <class Impl>
  static constexpr void wide_exception_check(Impl &&) noexcept {}
};

struct terminate : base {
  template <class Impl>
  static constexpr void wide_value_check(Impl &&self) noexcept {
    if (!base::_has_value(static_cast<Impl &&>(self)))
      std::terminate();
  }
  template <class Impl>
  static constexpr void wide_error_check(Impl &&self) noexcept {
    if (!base::_has_error(static_cast<Impl &&>(self)))
      std::terminate();
  }
  template <class Impl>
  static constexpr void wide_exception_check(Impl &&self) noexcept {
    if (!base::_has_exception(static_cast<Impl &&>(self)))
      std::terminate();
  }
};

} // namespace policy

// ============================================================================
// basic_result<R, S, NoValuePolicy>
// ============================================================================
template <class R, class S, class NoValuePolicy>
class [[nodiscard]] basic_result {
  template <class, class, class> friend class basic_result;
  template <class, class, class, class> friend class basic_outcome;
  friend struct policy::base;

public:
  using value_type = R;
  using error_type = S;
  using no_value_policy_type = NoValuePolicy;

  template <class T, class U = S, class V = NoValuePolicy>
  using rebind = basic_result<T, U, V>;

protected:
  detail::value_storage<R, S> _state;

public:
  [[nodiscard]] constexpr bool has_value() const noexcept {
    return _state._status.have_value();
  }
  [[nodiscard]] constexpr bool has_error() const noexcept {
    return _state._status.have_error();
  }
  [[nodiscard]] constexpr bool has_failure() const noexcept {
    return _state._status.have_error();
  }
  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return has_value();
  }

  constexpr auto &&assume_value() & noexcept { return _state._value; }
  constexpr auto &&assume_value() const & noexcept { return _state._value; }
  constexpr auto &&assume_value() && noexcept {
    return static_cast<detail::devoid<R> &&>(_state._value);
  }

  constexpr auto &&value() & {
    NoValuePolicy::wide_value_check(*this);
    return _state._value;
  }
  constexpr auto &&value() const & {
    NoValuePolicy::wide_value_check(*this);
    return _state._value;
  }
  constexpr auto &&value() && {
    NoValuePolicy::wide_value_check(*this);
    return static_cast<detail::devoid<R> &&>(_state._value);
  }

  constexpr auto &&assume_error() & noexcept { return _state._error; }
  constexpr auto &&assume_error() const & noexcept { return _state._error; }
  constexpr auto &&assume_error() && noexcept {
    return static_cast<detail::devoid<S> &&>(_state._error);
  }

  constexpr auto &&error() & {
    NoValuePolicy::wide_error_check(*this);
    return _state._error;
  }
  constexpr auto &&error() const & {
    NoValuePolicy::wide_error_check(*this);
    return _state._error;
  }
  constexpr auto &&error() && {
    NoValuePolicy::wide_error_check(*this);
    return static_cast<detail::devoid<S> &&>(_state._error);
  }

  // ---- Constructors ----
  basic_result() = delete;
  constexpr basic_result(const basic_result &) = default;
  constexpr basic_result(basic_result &&) = default;
  constexpr basic_result &operator=(const basic_result &) = default;
  constexpr basic_result &operator=(basic_result &&) = default;
  constexpr ~basic_result() = default;

  // From value_type (implicit)
  template <class T>
    requires(!std::is_same_v<std::remove_cvref_t<T>, basic_result> &&
             !is_success_type<std::remove_cvref_t<T>>::value &&
             !is_failure_type<std::remove_cvref_t<T>>::value &&
             std::constructible_from<detail::devoid<R>, T> &&
             !std::constructible_from<detail::devoid<S>, T>)
  constexpr basic_result(T &&v) noexcept( // NOLINT
      std::is_nothrow_constructible_v<detail::devoid<R>, T>)
      : _state(std::in_place_type<detail::devoid<R>>, static_cast<T &&>(v)) {}

  // From error_type (implicit)
  template <class T>
    requires(!std::is_same_v<std::remove_cvref_t<T>, basic_result> &&
             !is_success_type<std::remove_cvref_t<T>>::value &&
             !is_failure_type<std::remove_cvref_t<T>>::value &&
             std::constructible_from<detail::devoid<S>, T> &&
             !std::constructible_from<detail::devoid<R>, T>)
  constexpr basic_result(T &&v) noexcept( // NOLINT
      std::is_nothrow_constructible_v<detail::devoid<S>, T>)
      : _state(std::in_place_type<detail::devoid<S>>, static_cast<T &&>(v)) {}

  // Explicit in-place value
  template <class... Args>
    requires std::constructible_from<detail::devoid<R>, Args...>
  constexpr explicit basic_result(std::in_place_type_t<value_type>,
                                  Args &&...args)
      : _state(std::in_place_type<detail::devoid<R>>,
               static_cast<Args &&>(args)...) {}

  // Explicit in-place error
  template <class... Args>
    requires std::constructible_from<detail::devoid<S>, Args...>
  constexpr explicit basic_result(std::in_place_type_t<error_type>,
                                  Args &&...args)
      : _state(std::in_place_type<detail::devoid<S>>,
               static_cast<Args &&>(args)...) {}

  // From success_type<void>
  constexpr basic_result(success_type<void> &&) noexcept // NOLINT
    requires std::is_void_v<R>
      : _state(std::in_place_type<detail::empty_type>) {}

  // From success_type<T>
  template <class T>
    requires(!std::is_void_v<R> &&
             std::constructible_from<detail::devoid<R>, T>)
  constexpr basic_result(success_type<T> &&o) noexcept( // NOLINT
      std::is_nothrow_constructible_v<detail::devoid<R>, T>)
      : _state(std::in_place_type<detail::devoid<R>>,
               static_cast<T &&>(o._value)) {
    _state._status.set_spare_storage_value(o.spare_storage());
  }

  // From failure_type<EC, void>
  template <class EC>
    requires std::constructible_from<detail::devoid<S>, EC>
  constexpr basic_result(failure_type<EC, void> &&o) noexcept( // NOLINT
      std::is_nothrow_constructible_v<detail::devoid<S>, EC>)
      : _state(std::in_place_type<detail::devoid<S>>,
               static_cast<EC &&>(o._error)) {
    _state._status.set_spare_storage_value(o.spare_storage());
  }

  // Converting from compatible basic_result
  template <class T, class U, class V>
    requires(!std::is_same_v<basic_result, basic_result<T, U, V>> &&
             std::constructible_from<detail::devoid<R>, detail::devoid<T>> &&
             std::constructible_from<detail::devoid<S>, detail::devoid<U>>)
  constexpr explicit basic_result(const basic_result<T, U, V> &o) : _state() {
    if (o.has_value()) {
      new (&_state._value) detail::devoid<R>(o._state._value);
      _state._status.set_have_value(true);
    } else if (o.has_error()) {
      new (&_state._error) detail::devoid<S>(o._state._error);
      _state._status.set_have_error(true);
    }
    _state._status.set_spare_storage_value(
        o._state._status.spare_storage_value());
  }

  // as_failure
  auto as_failure() const & {
    return failure(this->assume_error(), _state._status.spare_storage_value());
  }

  auto as_failure() && {
    _state._status.set_have_moved_from(true);
    return failure(static_cast<detail::devoid<S> &&>(this->_state._error),
                   _state._status.spare_storage_value());
  }

  // Spare storage
  [[nodiscard]] constexpr uint16_t spare_storage_value() const noexcept {
    return _state._status.spare_storage_value();
  }
  constexpr void set_spare_storage_value(uint16_t v) noexcept {
    _state._status.set_spare_storage_value(v);
  }

  // Comparison
  template <class T>
  [[nodiscard]] constexpr bool
  operator==(const success_type<T> &o) const noexcept {
    if (!has_value())
      return false;
    if constexpr (std::is_void_v<T> || std::is_void_v<R>) {
      return true;
    } else {
      return _state._value == o._value;
    }
  }

  template <class EC, class E>
  [[nodiscard]] constexpr bool
  operator==(const failure_type<EC, E> &o) const noexcept {
    if (!has_error())
      return false;
    return _state._error == o._error;
  }
};

} // namespace outcome::core
