#pragma once

#include "domain.hpp"

#include <cstring>
#include <utility>

namespace outcome::core {

// ============================================================================
// status_code<void> — type-erased immutable base
// Must be defined first since storage types inherit from it.
// ============================================================================
template <> class status_code<void> {
  template <class> friend class status_code;
  template <class> friend class errored_status_code;
  friend class status_code_domain;

public:
  using domain_type = void;
  using value_type = void;
  using string_ref = status_code_domain::string_ref;

protected:
  const status_code_domain *_domain{nullptr};

  constexpr status_code() noexcept = default;
  constexpr explicit status_code(const status_code_domain *dom) noexcept
      : _domain(dom) {}

  status_code(const status_code &) = default;
  status_code(status_code &&) = default;
  status_code &operator=(const status_code &) = default;
  status_code &operator=(status_code &&) = default;
  ~status_code() = default;

public:
  [[nodiscard]] constexpr bool empty() const noexcept {
    return _domain == nullptr;
  }

  [[nodiscard]] constexpr const status_code_domain &domain() const noexcept {
    return *_domain;
  }

  [[nodiscard]] string_ref message() const noexcept {
    return _domain ? _domain->_do_message(*this) : string_ref("(empty)");
  }

  [[nodiscard]] bool success() const noexcept {
    return _domain ? !_domain->_do_failure(*this) : false;
  }

  [[nodiscard]] bool failure() const noexcept {
    return _domain ? _domain->_do_failure(*this) : false;
  }

  template <class T>
  [[nodiscard]] bool
  strictly_equivalent(const status_code<T> &o) const noexcept {
    const auto *o_domain =
        reinterpret_cast<const status_code<void> &>(o)._domain;
    if (_domain && o_domain) {
      const auto &o_base = reinterpret_cast<const status_code<void> &>(o);
      if (_domain->_do_equivalent(*this, o_base))
        return true;
      if (o_domain->_do_equivalent(o_base, *this))
        return true;
    }
    return false;
  }

  template <class T>
  [[nodiscard]] bool equivalent(const status_code<T> &o) const noexcept;

#ifdef __cpp_exceptions
  void throw_exception() const {
    if (_domain)
      _domain->_do_throw_exception(*this);
  }
#endif
};

// ============================================================================
// detail: storage types (after status_code<void> is complete)
// ============================================================================
namespace detail {

// Storage base: holds domain pointer + value
template <class DomainType>
struct status_code_storage : public status_code<void> {
  using domain_type = DomainType;
  using value_type = typename domain_type::value_type;

  value_type _value{};

  status_code_storage() = default;

  constexpr explicit status_code_storage(const status_code_domain *dom,
                                         const value_type &v) noexcept
      : status_code<void>(dom), _value(v) {}

  constexpr explicit status_code_storage(const status_code_domain *dom,
                                         value_type &&v) noexcept
      : status_code<void>(dom), _value(static_cast<value_type &&>(v)) {}
};

// Storage for erased status codes
template <class ErasedType>
struct status_code_storage<erased<ErasedType>> : public status_code<void> {
  using domain_type = void;
  using value_type = ErasedType;

  value_type _value{};

  status_code_storage() = default;

  constexpr explicit status_code_storage(const status_code_domain *dom,
                                         value_type v) noexcept
      : status_code<void>(dom), _value(v) {}
};

// Check if a domain's value_type can be safely erased into ErasedType
template <class DomainType, class ErasedType>
concept ErasureSafe =
    sizeof(typename DomainType::value_type) <= sizeof(ErasedType) &&
    alignof(ErasedType) % alignof(typename DomainType::value_type) == 0 &&
    std::is_trivially_copyable_v<typename DomainType::value_type>;

// Check if a domain has a stateful mixin
template <class DomainType>
constexpr bool has_stateful_mixin =
    sizeof(mixin<status_code_storage<DomainType>, DomainType>) !=
    sizeof(status_code_storage<DomainType>);

// Helper: downcast from status_code<void> to typed code.
// This is safe because domain virtual functions are only called with matching
// types.
template <class DomainType>
const status_code<DomainType> &
downcast(const status_code<void> &code) noexcept {
  return reinterpret_cast<const status_code<DomainType> &>(code);
}

} // namespace detail

// ============================================================================
// status_code<DomainType> — typed status code
// ============================================================================
template <class DomainType>
class status_code
    : public mixin<detail::status_code_storage<DomainType>, DomainType> {
  using _base = mixin<detail::status_code_storage<DomainType>, DomainType>;

public:
  using domain_type = DomainType;
  using value_type = typename domain_type::value_type;
  using string_ref = typename status_code_domain::string_ref;

  status_code() = default;
  status_code(const status_code &) = default;
  status_code(status_code &&) = default;
  status_code &operator=(const status_code &) = default;
  status_code &operator=(status_code &&) = default;
  ~status_code() = default;

  // Explicit from value_type
  constexpr explicit status_code(const value_type &v) noexcept
      : _base(&domain_type::get(), v) {}

  constexpr explicit status_code(value_type &&v) noexcept
      : _base(&domain_type::get(), static_cast<value_type &&>(v)) {}

  // In-place construction
  template <class... Args>
    requires std::constructible_from<value_type, Args...>
  constexpr explicit status_code(std::in_place_t, Args &&...args) noexcept(
      std::is_nothrow_constructible_v<value_type, Args...>)
      : _base(&domain_type::get(), value_type(static_cast<Args &&>(args)...)) {}

  // Implicit from make_status_code ADL hook
  template <class T>
    requires(!std::is_same_v<std::remove_cvref_t<T>, status_code> &&
             !std::is_same_v<std::remove_cvref_t<T>, value_type> &&
             !std::is_same_v<std::remove_cvref_t<T>, std::in_place_t> &&
             requires(T &&t) {
               {
                 make_status_code(static_cast<T &&>(t))
               } -> std::same_as<status_code>;
             })
  constexpr status_code(T &&v) // NOLINT(google-explicit-constructor)
      : status_code(make_status_code(static_cast<T &&>(v))) {}

  // Reconstruct from erased status code
  template <class ErasedType>
    requires detail::ErasureSafe<DomainType, ErasedType>
  constexpr explicit status_code(
      const status_code<detail::erased<ErasedType>> &v) noexcept
      : _base(&domain_type::get(),
              detail::erasure_cast<value_type>(v.value())) {}

  [[nodiscard]] constexpr const value_type &value() const & noexcept {
    return this->_value;
  }

  [[nodiscard]] static constexpr const domain_type &domain() noexcept {
    return domain_type::get();
  }

  [[nodiscard]] string_ref message() const noexcept {
    const status_code_domain &dom = domain_type::get();
    return dom._do_message(*this);
  }

  [[nodiscard]] bool success() const noexcept {
    const status_code_domain &dom = domain_type::get();
    return !dom._do_failure(*this);
  }

  [[nodiscard]] bool failure() const noexcept {
    const status_code_domain &dom = domain_type::get();
    return dom._do_failure(*this);
  }

  [[nodiscard]] status_code clone() const { return *this; }

#ifdef __cpp_exceptions
  void throw_exception() const {
    const status_code_domain &dom = domain_type::get();
    dom._do_throw_exception(*this);
  }
#endif
};

// ============================================================================
// status_code<erased<T>> — type-erased, move-only
// ============================================================================
template <class ErasedType>
class status_code<detail::erased<ErasedType>>
    : public detail::status_code_storage<detail::erased<ErasedType>> {
  using _base = detail::status_code_storage<detail::erased<ErasedType>>;

public:
  using domain_type = void;
  using value_type = ErasedType;
  using string_ref = typename status_code_domain::string_ref;

  status_code() = default;
  status_code(const status_code &) = delete;
  status_code(status_code &&o) noexcept : _base(o._domain, o._value) {
    o._domain = nullptr;
    o._value = {};
  }
  status_code &operator=(const status_code &) = delete;
  status_code &operator=(status_code &&o) noexcept {
    if (this != &o) {
      _destroy();
      this->_domain = o._domain;
      this->_value = o._value;
      o._domain = nullptr;
      o._value = {};
    }
    return *this;
  }

  ~status_code() { _destroy(); }

  // Implicit from typed status_code (erasure)
  template <class DomainType>
    requires(!std::is_void_v<DomainType> &&
             detail::ErasureSafe<DomainType, ErasedType> &&
             !detail::has_stateful_mixin<DomainType>)
  constexpr status_code(const status_code<DomainType> &v) noexcept // NOLINT
      : _base(&DomainType::get(), detail::erasure_cast<ErasedType>(v.value())) {
  }

  template <class DomainType>
    requires(!std::is_void_v<DomainType> &&
             detail::ErasureSafe<DomainType, ErasedType> &&
             !detail::has_stateful_mixin<DomainType>)
  status_code(status_code<DomainType> &&v) noexcept // NOLINT
      : _base(&DomainType::get(), detail::erasure_cast<ErasedType>(v.value())) {
  }

  // Implicit from make_status_code ADL hook
  template <class T>
    requires(!traits::is_status_code<std::remove_cvref_t<T>>::value &&
             !traits::is_errored_status_code<std::remove_cvref_t<T>>::value &&
             requires(T &&t) {
               {
                 make_status_code(static_cast<T &&>(t))
               } -> std::convertible_to<status_code>;
             })
  constexpr status_code(T &&v) // NOLINT(google-explicit-constructor)
      : status_code(make_status_code(static_cast<T &&>(v))) {}

  [[nodiscard]] constexpr value_type value() const noexcept {
    return this->_value;
  }

  [[nodiscard]] const status_code_domain &domain() const noexcept {
    return *this->_domain;
  }

  [[nodiscard]] string_ref message() const noexcept {
    return this->_domain ? this->_domain->_do_message(*this)
                         : string_ref("(empty)");
  }

  [[nodiscard]] bool success() const noexcept {
    return this->_domain ? !this->_domain->_do_failure(*this) : false;
  }

  [[nodiscard]] bool failure() const noexcept {
    return this->_domain ? this->_domain->_do_failure(*this) : false;
  }

  [[nodiscard]] status_code clone() const {
    status_code ret;
    if (this->_domain) {
      this->_domain->_do_erased_copy(ret, *this, this->_domain->payload_info());
    }
    return ret;
  }

  void clear() noexcept {
    _destroy();
    this->_domain = nullptr;
    this->_value = {};
  }

#ifdef __cpp_exceptions
  void throw_exception() const {
    if (this->_domain)
      this->_domain->_do_throw_exception(*this);
  }
#endif

private:
  void _destroy() noexcept {
    if (this->_domain) {
      this->_domain->_do_erased_destroy(*this, this->_domain->payload_info());
    }
  }
};

template <class ErasedType>
using erased_status_code = status_code<detail::erased<ErasedType>>;

// ============================================================================
// errored_status_code<DomainType> — always a failure
// ============================================================================
template <class DomainType>
class errored_status_code : public status_code<DomainType> {
  using _base = status_code<DomainType>;

  void _check() const noexcept {
    if (!_base::failure()) {
      std::terminate();
    }
  }

public:
  using domain_type = DomainType;
  using value_type = typename _base::value_type;
  using string_ref = typename _base::string_ref;

  errored_status_code() = default;
  errored_status_code(const errored_status_code &) = default;
  errored_status_code(errored_status_code &&) = default;
  errored_status_code &operator=(const errored_status_code &) = default;
  errored_status_code &operator=(errored_status_code &&) = default;
  ~errored_status_code() = default;

  constexpr explicit errored_status_code(const _base &o) noexcept : _base(o) {
    _check();
  }
  constexpr explicit errored_status_code(_base &&o) noexcept
      : _base(static_cast<_base &&>(o)) {
    _check();
  }

  constexpr explicit errored_status_code(const value_type &v) noexcept
      : _base(v) {
    _check();
  }
  constexpr explicit errored_status_code(value_type &&v) noexcept
      : _base(static_cast<value_type &&>(v)) {
    _check();
  }

  template <class T>
    requires(!std::is_same_v<std::remove_cvref_t<T>, errored_status_code> &&
             !std::is_same_v<std::remove_cvref_t<T>, value_type> &&
             !std::is_same_v<std::remove_cvref_t<T>, _base> &&
             requires(T &&t) {
               {
                 make_status_code(static_cast<T &&>(t))
               } -> std::same_as<_base>;
             })
  constexpr errored_status_code(T &&v) // NOLINT
      : _base(make_status_code(static_cast<T &&>(v))) {
    _check();
  }

  template <class ErasedType>
    requires detail::ErasureSafe<DomainType, ErasedType>
  constexpr explicit errored_status_code(
      const status_code<detail::erased<ErasedType>> &v) noexcept
      : _base(v) {
    _check();
  }

  [[nodiscard]] constexpr bool success() const noexcept { return false; }
};

// ============================================================================
// errored_status_code<erased<T>> — type-erased, move-only, always failure
// ============================================================================
template <class ErasedType>
class errored_status_code<detail::erased<ErasedType>>
    : public status_code<detail::erased<ErasedType>> {
  using _base = status_code<detail::erased<ErasedType>>;

  void _check() const noexcept {
    if (!_base::failure()) {
      std::terminate();
    }
  }

public:
  using domain_type = void;
  using value_type = ErasedType;
  using string_ref = typename _base::string_ref;

  errored_status_code() = default;
  errored_status_code(const errored_status_code &) = delete;
  errored_status_code(errored_status_code &&o) noexcept
      : _base(static_cast<_base &&>(o)) {}
  errored_status_code &operator=(const errored_status_code &) = delete;
  errored_status_code &operator=(errored_status_code &&o) noexcept {
    _base::operator=(static_cast<_base &&>(o));
    return *this;
  }
  ~errored_status_code() = default;

  explicit errored_status_code(_base &&o) noexcept
      : _base(static_cast<_base &&>(o)) {
    _check();
  }

  template <class DomainType>
    requires(!std::is_void_v<DomainType> &&
             detail::ErasureSafe<DomainType, ErasedType> &&
             !detail::has_stateful_mixin<DomainType>)
  errored_status_code(const status_code<DomainType> &v) noexcept // NOLINT
      : _base(v) {
    _check();
  }

  template <class DomainType>
    requires(!std::is_void_v<DomainType> &&
             detail::ErasureSafe<DomainType, ErasedType> &&
             !detail::has_stateful_mixin<DomainType>)
  errored_status_code(status_code<DomainType> &&v) noexcept // NOLINT
      : _base(static_cast<status_code<DomainType> &&>(v)) {
    _check();
  }

  template <class DomainType>
    requires(!std::is_void_v<DomainType> &&
             detail::ErasureSafe<DomainType, ErasedType> &&
             !detail::has_stateful_mixin<DomainType>)
  errored_status_code(
      const errored_status_code<DomainType> &v) noexcept // NOLINT
      : _base(static_cast<const status_code<DomainType> &>(v)) {}

  template <class DomainType>
    requires(!std::is_void_v<DomainType> &&
             detail::ErasureSafe<DomainType, ErasedType> &&
             !detail::has_stateful_mixin<DomainType>)
  errored_status_code(errored_status_code<DomainType> &&v) noexcept // NOLINT
      : _base(static_cast<status_code<DomainType> &&>(v)) {}

  template <class T>
    requires(!traits::is_status_code<std::remove_cvref_t<T>>::value &&
             !traits::is_errored_status_code<std::remove_cvref_t<T>>::value &&
             requires(T &&t) {
               {
                 make_status_code(static_cast<T &&>(t))
               } -> std::convertible_to<_base>;
             })
  constexpr errored_status_code(T &&v) // NOLINT
      : errored_status_code(_base(make_status_code(static_cast<T &&>(v)))) {}

  [[nodiscard]] constexpr bool success() const noexcept { return false; }

  [[nodiscard]] errored_status_code clone() const {
    return errored_status_code(_base::clone());
  }
};

template <class ErasedType>
using erased_errored_status_code =
    errored_status_code<detail::erased<ErasedType>>;

// ============================================================================
// Comparison operators
// ============================================================================
template <class DomainType1, class DomainType2>
[[nodiscard]] inline bool
operator==(const status_code<DomainType1> &a,
           const status_code<DomainType2> &b) noexcept {
  const auto &a_base = reinterpret_cast<const status_code<void> &>(a);
  return a_base.strictly_equivalent(b);
}

} // namespace outcome::core
