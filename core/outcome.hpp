#pragma once

#include "result.hpp"

#include <exception>

namespace outcome::core {

// ============================================================================
// Policies for status_code-based result/outcome
// ============================================================================
namespace policy {

template <class T, class EC, class E> struct status_code_throw : base {
  template <class Impl> static constexpr void wide_value_check(Impl &&self) {
    if (!base::_has_value(static_cast<Impl &&>(self))) {
      if constexpr (requires { static_cast<Impl &&>(self)._exception; }) {
        if (base::_has_exception(static_cast<Impl &&>(self))) {
#ifdef __cpp_exceptions
          auto &ep = static_cast<Impl &&>(self)._exception;
          std::rethrow_exception(ep);
#else
          std::terminate();
#endif
        }
      }
      if (base::_has_error(static_cast<Impl &&>(self))) {
#ifdef __cpp_exceptions
        base::_error(static_cast<Impl &&>(self)).throw_exception();
#else
        std::terminate();
#endif
      }
    }
  }

  template <class Impl>
  static constexpr void wide_error_check(Impl &&self) noexcept {
    base::narrow_error_check(static_cast<Impl &&>(self));
  }

  template <class Impl>
  static constexpr void wide_exception_check(Impl &&self) noexcept {
    base::narrow_exception_check(static_cast<Impl &&>(self));
  }
};

template <class T, class EC>
using default_status_result_policy = std::conditional_t<
    std::is_void_v<EC>, terminate,
    std::conditional_t<traits::is_status_code<EC>::value ||
                           traits::is_errored_status_code<EC>::value,
                       status_code_throw<T, EC, void>, terminate>>;

template <class T, class EC, class E>
using default_status_outcome_policy = std::conditional_t<
    std::is_void_v<EC> && std::is_void_v<E>, terminate,
    std::conditional_t<traits::is_status_code<EC>::value ||
                           traits::is_errored_status_code<EC>::value,
                       status_code_throw<T, EC, E>, terminate>>;

} // namespace policy

// ============================================================================
// basic_outcome<R, S, P, NoValuePolicy>
// ============================================================================
template <class R, class S, class P, class NoValuePolicy>
class [[nodiscard]] basic_outcome : public basic_result<R, S, NoValuePolicy> {
  using _base = basic_result<R, S, NoValuePolicy>;
  template <class, class, class, class> friend class basic_outcome;
  friend struct policy::base;
  template <class, class, class> friend struct policy::status_code_throw;

public:
  using value_type = R;
  using error_type = S;
  using exception_type = P;
  using no_value_policy_type = NoValuePolicy;

protected:
  detail::devoid<P> _exception{};

public:
  [[nodiscard]] constexpr bool has_exception() const noexcept {
    return this->_state._status.have_exception();
  }
  [[nodiscard]] constexpr bool has_failure() const noexcept {
    return this->_state._status.have_error() ||
           this->_state._status.have_exception();
  }

  constexpr auto &&assume_exception() & noexcept { return _exception; }
  constexpr auto &&assume_exception() const & noexcept { return _exception; }
  constexpr auto &&assume_exception() && noexcept {
    return static_cast<detail::devoid<P> &&>(_exception);
  }

  constexpr auto &&exception() & {
    NoValuePolicy::wide_exception_check(*this);
    return _exception;
  }
  constexpr auto &&exception() const & {
    NoValuePolicy::wide_exception_check(*this);
    return _exception;
  }

  [[nodiscard]] P failure() const noexcept
    requires std::is_same_v<P, std::exception_ptr>
  {
#ifdef __cpp_exceptions
    try {
#endif
      if (this->_state._status.have_exception())
        return _exception;
      if (this->_state._status.have_error()) {
#ifdef __cpp_exceptions
        try {
          this->assume_error().throw_exception();
        } catch (...) {
          return std::current_exception();
        }
#endif
      }
      return P{};
#ifdef __cpp_exceptions
    } catch (...) {
      return std::current_exception();
    }
#endif
  }

  // Override value() so the policy sees basic_outcome (with _exception),
  // not basic_result (which lacks _exception)
  constexpr auto &&value() & {
    NoValuePolicy::wide_value_check(*this);
    return this->_state._value;
  }
  constexpr auto &&value() const & {
    NoValuePolicy::wide_value_check(*this);
    return this->_state._value;
  }
  constexpr auto &&value() && {
    NoValuePolicy::wide_value_check(*this);
    return static_cast<detail::devoid<R> &&>(this->_state._value);
  }

  // Constructors
  basic_outcome() = delete;
  constexpr basic_outcome(const basic_outcome &) = default;
  constexpr basic_outcome(basic_outcome &&) = default;
  constexpr basic_outcome &operator=(const basic_outcome &) = default;
  constexpr basic_outcome &operator=(basic_outcome &&) = default;
  constexpr ~basic_outcome() = default;

  using _base::_base;

  // From exception_type
  template <class T>
    requires(!std::is_same_v<std::remove_cvref_t<T>, basic_outcome> &&
             !is_success_type<std::remove_cvref_t<T>>::value &&
             !is_failure_type<std::remove_cvref_t<T>>::value &&
             std::constructible_from<detail::devoid<P>, T> &&
             !std::constructible_from<detail::devoid<R>, T> &&
             !std::constructible_from<detail::devoid<S>, T>)
  constexpr basic_outcome(T &&v) noexcept( // NOLINT
      std::is_nothrow_constructible_v<detail::devoid<P>, T>)
      : _base(std::in_place_type<error_type>),
        _exception(static_cast<T &&>(v)) {
    this->_state._status.set_have_error(false);
    this->_state._status.set_have_exception(true);
  }

  constexpr basic_outcome(success_type<void> &&o) noexcept // NOLINT
    requires std::is_void_v<R>
      : _base(static_cast<success_type<void> &&>(o)) {}

  template <class T>
    requires(!std::is_void_v<R> &&
             std::constructible_from<detail::devoid<R>, T>)
  constexpr basic_outcome(success_type<T> &&o) noexcept( // NOLINT
      std::is_nothrow_constructible_v<detail::devoid<R>, T>)
      : _base(static_cast<success_type<T> &&>(o)) {}

  template <class EC>
    requires std::constructible_from<detail::devoid<S>, EC>
  constexpr basic_outcome(failure_type<EC, void> &&o) noexcept( // NOLINT
      std::is_nothrow_constructible_v<detail::devoid<S>, EC>)
      : _base(static_cast<failure_type<EC, void> &&>(o)) {}

  template <class EC, class E>
    requires(std::constructible_from<detail::devoid<S>, EC> &&
             std::constructible_from<detail::devoid<P>, E>)
  constexpr basic_outcome(failure_type<EC, E> &&o) // NOLINT
      : _base(std::in_place_type<error_type>, static_cast<EC &&>(o._error)),
        _exception(static_cast<E &&>(o._exception)) {
    if (o.has_exception()) {
      this->_state._status.set_have_exception(true);
    }
    this->_state._status.set_spare_storage_value(o.spare_storage());
  }

  failure_type<error_type, exception_type> as_failure() const & {
    if (this->has_error() && this->has_exception()) {
      return failure_type<error_type, exception_type>(
          this->assume_error(), _exception,
          this->_state._status.spare_storage_value());
    }
    if (this->has_exception()) {
      return failure_type<error_type, exception_type>(
          std::in_place_type<exception_type>, _exception,
          this->_state._status.spare_storage_value());
    }
    return failure_type<error_type, exception_type>(
        std::in_place_type<error_type>, this->assume_error(),
        this->_state._status.spare_storage_value());
  }

  failure_type<error_type, exception_type> as_failure() && {
    this->_state._status.set_have_moved_from(true);
    if (this->has_error() && this->has_exception()) {
      return failure_type<error_type, exception_type>(
          static_cast<detail::devoid<S> &&>(this->_state._error),
          static_cast<detail::devoid<P> &&>(_exception),
          this->_state._status.spare_storage_value());
    }
    if (this->has_exception()) {
      return failure_type<error_type, exception_type>(
          std::in_place_type<exception_type>,
          static_cast<detail::devoid<P> &&>(_exception),
          this->_state._status.spare_storage_value());
    }
    return failure_type<error_type, exception_type>(
        std::in_place_type<error_type>,
        static_cast<detail::devoid<S> &&>(this->_state._error),
        this->_state._status.spare_storage_value());
  }
};

// ============================================================================
// Type aliases
// ============================================================================
template <class R,
          class S =
              erased_errored_status_code<typename system_code::value_type>,
          class NoValuePolicy = policy::default_status_result_policy<R, S>>
using status_result = basic_result<R, S, NoValuePolicy>;

template <class R,
          class S =
              erased_errored_status_code<typename system_code::value_type>,
          class P = std::exception_ptr,
          class NoValuePolicy = policy::default_status_outcome_policy<R, S, P>>
using status_outcome = basic_outcome<R, S, P, NoValuePolicy>;

namespace hooks {

template <class R, class S, class NoValuePolicy>
constexpr uint16_t
spare_storage(const basic_result<R, S, NoValuePolicy> *r) noexcept {
  return r->spare_storage_value();
}

template <class R, class S, class NoValuePolicy>
constexpr void set_spare_storage(basic_result<R, S, NoValuePolicy> *r,
                                 uint16_t v) noexcept {
  r->set_spare_storage_value(v);
}

} // namespace hooks

// ============================================================================
// clone()
// ============================================================================
template <class R, class S, class NoValuePolicy>
  requires(std::is_copy_constructible_v<R> &&
           (traits::is_status_code<S>::value ||
            traits::is_errored_status_code<S>::value))
inline basic_result<R, S, NoValuePolicy>
clone(const basic_result<R, S, NoValuePolicy> &v) {
  if (v)
    return success_type<R>(v.assume_value());
  return failure_type<S>(v.assume_error().clone(), hooks::spare_storage(&v));
}

template <class S, class NoValuePolicy>
  requires(traits::is_status_code<S>::value ||
           traits::is_errored_status_code<S>::value)
inline basic_result<void, S, NoValuePolicy>
clone(const basic_result<void, S, NoValuePolicy> &v) {
  if (v)
    return success_type<void>();
  return failure_type<S>(v.assume_error().clone(), hooks::spare_storage(&v));
}

} // namespace outcome::core
