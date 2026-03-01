#pragma once

#include "errc.hpp"

#include <cerrno>
#include <cstring>

namespace outcome::core {

class _posix_code_domain;
using posix_code = status_code<_posix_code_domain>;

// Mixin: adds current() static method to posix_code
template <class Base> struct mixin<Base, _posix_code_domain> : public Base {
  using Base::Base;

  static posix_code current() noexcept;
};

class _posix_code_domain : public status_code_domain {
  using _self = _posix_code_domain;

public:
  using value_type = int;

  constexpr explicit _posix_code_domain(
      unique_id_type id = 0xa59a56fe5f310933ULL) noexcept
      : status_code_domain(id) {}

  static const _self &get() noexcept {
    static const _self singleton{};
    return singleton;
  }

  [[nodiscard]] string_ref name() const noexcept override {
    return string_ref("posix domain");
  }

  [[nodiscard]] payload_info_t payload_info() const noexcept override {
    return {sizeof(value_type),
            sizeof(status_code_domain *) + sizeof(value_type),
            alignof(value_type)};
  }

protected:
  [[nodiscard]] bool
  _do_failure(const status_code<void> &code) const noexcept override {
    return detail::downcast<_self>(code).value() != 0;
  }

  [[nodiscard]] bool
  _do_equivalent(const status_code<void> &code1,
                 const status_code<void> &code2) const noexcept override {
    if (code1.domain() == *this && code2.domain() == *this) {
      return detail::downcast<_self>(code1).value() ==
             detail::downcast<_self>(code2).value();
    }
    return false;
  }

  [[nodiscard]] generic_code
  _generic_code(const status_code<void> &code) const noexcept override {
    return generic_code(
        static_cast<errc>(detail::downcast<_self>(code).value()));
  }

  [[nodiscard]] string_ref
  _do_message(const status_code<void> &code) const noexcept override {
    auto val = detail::downcast<_self>(code).value();
    if (val == 0)
      return string_ref("success");
    return atomic_refcounted_string_ref::create(::strerror(val));
  }

#ifdef __cpp_exceptions
  [[noreturn]] void
  _do_throw_exception(const status_code<void> &code) const override {
    throw status_error<_self>(
        status_code<_self>(detail::downcast<_self>(code).value()));
  }
#endif

  void _do_erased_copy(status_code<void> &dst, const status_code<void> &src,
                       payload_info_t /*info*/) const override {
    std::memcpy(&dst, &src, sizeof(status_code_domain *) + sizeof(value_type));
  }

  void _do_erased_destroy(status_code<void> & /*code*/,
                          payload_info_t /*info*/) const noexcept override {}
};

inline const _posix_code_domain &posix_code_domain = _posix_code_domain::get();

template <class Base>
posix_code mixin<Base, _posix_code_domain>::current() noexcept {
  return posix_code(std::in_place_t{}, errno);
}

} // namespace outcome::core
