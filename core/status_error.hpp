#pragma once

#include "status_code.hpp"

#include <exception>

namespace outcome::core {

// ============================================================================
// status_error<void> — type-erased exception base
// ============================================================================
template <> class status_error<void> : public std::exception {
protected:
  virtual const status_code<void> &_do_code() const noexcept = 0;

public:
  using domain_type = void;
  using status_code_type = status_code<void>;

  [[nodiscard]] const status_code<void> &code() const noexcept {
    return _do_code();
  }
};

// Forward declare for friend access
template <class DomainType> class status_error;

// ============================================================================
// status_error<DomainType> — typed exception
// ============================================================================
template <class DomainType> class status_error : public status_error<void> {
  status_code<DomainType> _code;
  typename status_code_domain::string_ref _msgref;

  const status_code<void> &_do_code() const noexcept override { return _code; }

public:
  using domain_type = DomainType;
  using status_code_type = status_code<DomainType>;

  explicit status_error(status_code<DomainType> c)
      : _code(static_cast<status_code<DomainType> &&>(c)),
        _msgref(_code.message()) {}

  [[nodiscard]] const char *what() const noexcept override {
    return _msgref.data();
  }

  [[nodiscard]] const status_code_type &code() const & noexcept {
    return _code;
  }
  [[nodiscard]] status_code_type &code() & noexcept { return _code; }
  [[nodiscard]] const status_code_type &&code() const && noexcept {
    return static_cast<const status_code_type &&>(_code);
  }
  [[nodiscard]] status_code_type &&code() && noexcept {
    return static_cast<status_code_type &&>(_code);
  }
};

// ============================================================================
// status_error<erased<T>> — type-erased exception
// ============================================================================
template <class ErasedType>
class status_error<detail::erased<ErasedType>> : public status_error<void> {
  status_code<detail::erased<ErasedType>> _code;
  typename status_code_domain::string_ref _msgref;

  const status_code<void> &_do_code() const noexcept override { return _code; }

public:
  using domain_type = void;
  using status_code_type = status_code<detail::erased<ErasedType>>;

  explicit status_error(status_code_type c)
      : _code(static_cast<status_code_type &&>(c)), _msgref(_code.message()) {}

  [[nodiscard]] const char *what() const noexcept override {
    return _msgref.data();
  }

  [[nodiscard]] const status_code_type &code() const & noexcept {
    return _code;
  }
  [[nodiscard]] status_code_type &code() & noexcept { return _code; }
};

} // namespace outcome::core
