#pragma once

#include "system_code.hpp"

#include <initializer_list>

namespace outcome::core {

// ============================================================================
// quick_status_code_from_enum — user specializes this for their enum
// ============================================================================

// Forward declare
template <class Enum> class _quick_status_code_from_enum_domain;

// Default base providing common types and patterns
template <class Enum> struct quick_status_code_from_enum_defaults {
  using code_type =
      status_code<class _quick_status_code_from_enum_domain<Enum>>;

  struct mapping {
    using enumeration_type = Enum;
    const Enum value;
    const char *message;
    const std::initializer_list<errc> code_mappings;
  };

  template <class Base> struct mixin : public Base {
    using Base::Base;
  };
};

// Primary template — user must specialize
// Must provide:
//   static constexpr const auto domain_name = "...";
//   static constexpr const auto domain_uuid = "...";
//   static const std::initializer_list<mapping>& value_mappings();
//   template <class Base> struct mixin : Base { using Base::Base; };
template <class Enum> struct quick_status_code_from_enum;

// ============================================================================
// _quick_status_code_from_enum_domain<Enum> — auto-generated domain
// ============================================================================
template <class Enum>
class _quick_status_code_from_enum_domain : public status_code_domain {
  using _info = quick_status_code_from_enum<Enum>;

public:
  using value_type = Enum;

  constexpr _quick_status_code_from_enum_domain() noexcept
      : status_code_domain(detail::parse_uuid_from_string(_info::domain_uuid)) {
  }

  static const _quick_status_code_from_enum_domain &get() noexcept {
    static const _quick_status_code_from_enum_domain singleton{};
    return singleton;
  }

  [[nodiscard]] string_ref name() const noexcept override {
    return string_ref(_info::domain_name);
  }

  [[nodiscard]] payload_info_t payload_info() const noexcept override {
    return {sizeof(value_type),
            sizeof(status_code_domain *) + sizeof(value_type),
            alignof(value_type)};
  }

protected:
  [[nodiscard]] bool
  _do_failure(const status_code<void> &code) const noexcept override {
    auto &c =
        static_cast<const status_code<_quick_status_code_from_enum_domain> &>(
            code);
    for (const auto &m : _info::value_mappings()) {
      if (m.value == c.value()) {
        for (auto e : m.code_mappings) {
          if (e == errc::success)
            return false;
        }
        return true;
      }
    }
    return true;
  }

  [[nodiscard]] bool
  _do_equivalent(const status_code<void> &code1,
                 const status_code<void> &code2) const noexcept override {
    if (code1.domain() == *this && code2.domain() == *this) {
      auto &c1 =
          static_cast<const status_code<_quick_status_code_from_enum_domain> &>(
              code1);
      auto &c2 =
          static_cast<const status_code<_quick_status_code_from_enum_domain> &>(
              code2);
      return c1.value() == c2.value();
    }
    // Check if other code maps to the same generic code
    if (code2.domain() == generic_code_domain) {
      auto &c1 =
          static_cast<const status_code<_quick_status_code_from_enum_domain> &>(
              code1);
      auto &c2 = static_cast<const status_code<_generic_code_domain> &>(code2);
      for (const auto &m : _info::value_mappings()) {
        if (m.value == c1.value()) {
          for (auto e : m.code_mappings) {
            if (e == c2.value())
              return true;
          }
        }
      }
    }
    return false;
  }

  [[nodiscard]] generic_code
  _generic_code(const status_code<void> &code) const noexcept override {
    auto &c =
        static_cast<const status_code<_quick_status_code_from_enum_domain> &>(
            code);
    for (const auto &m : _info::value_mappings()) {
      if (m.value == c.value()) {
        if (m.code_mappings.size() > 0) {
          return generic_code(*m.code_mappings.begin());
        }
      }
    }
    return generic_code(errc::unknown);
  }

  [[nodiscard]] string_ref
  _do_message(const status_code<void> &code) const noexcept override {
    auto &c =
        static_cast<const status_code<_quick_status_code_from_enum_domain> &>(
            code);
    for (const auto &m : _info::value_mappings()) {
      if (m.value == c.value()) {
        return string_ref(m.message);
      }
    }
    return string_ref("unknown enum value");
  }

#ifdef __cpp_exceptions
  [[noreturn]] void
  _do_throw_exception(const status_code<void> &code) const override {
    auto &c =
        static_cast<const status_code<_quick_status_code_from_enum_domain> &>(
            code);
    throw status_error<_quick_status_code_from_enum_domain>(
        status_code<_quick_status_code_from_enum_domain>(c.value()));
  }
#endif

  void _do_erased_copy(status_code<void> &dst, const status_code<void> &src,
                       payload_info_t /*info*/) const override {
    std::memcpy(&dst, &src, sizeof(status_code_domain *) + sizeof(value_type));
  }

  void _do_erased_destroy(status_code<void> & /*code*/,
                          payload_info_t /*info*/) const noexcept override {}
};

// Wire the user's mixin into the status_code
template <class Base, class Enum>
struct mixin<Base, _quick_status_code_from_enum_domain<Enum>>
    : public quick_status_code_from_enum<Enum>::template mixin<Base> {
  using quick_status_code_from_enum<Enum>::template mixin<Base>::mixin;
};

// Type alias for user's status code type
template <class Enum>
using quick_status_code_from_enum_code =
    status_code<_quick_status_code_from_enum_domain<Enum>>;

// ADL hook: construct from enum
template <class Enum>
  requires requires { quick_status_code_from_enum<Enum>::domain_name; }
inline quick_status_code_from_enum_code<Enum>
make_status_code(Enum e) noexcept {
  return quick_status_code_from_enum_code<Enum>(std::in_place_t{}, e);
}

} // namespace outcome::core
