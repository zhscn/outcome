#pragma once

#include "utils.hpp"
#include "traits.hpp"
#include "string_ref.hpp"

namespace outcome::core {

// Default mixin — passthrough, inherits from Base
template <class Base, class T> struct mixin : public Base {
  using Base::Base;
};

template <class Enum> struct quick_status_code_from_enum;

// Abstract base class for all status code domains.
// Each domain defines a set of status codes, their messages, equivalence, and
// failure semantics.
class status_code_domain {
public:
  using unique_id_type = uint64_t;
  using string_ref = core::string_ref;

  struct payload_info_t {
    size_t payload_size{0};
    size_t total_size{0};
    size_t total_alignment{0};
  };

protected:
  unique_id_type _id;

  // Construct with a pre-computed unique id
  constexpr explicit status_code_domain(unique_id_type id) noexcept : _id(id) {}

  // Construct from a UUID string
  constexpr explicit status_code_domain(const char *uuid) noexcept
      : _id(detail::parse_uuid_from_string(uuid)) {}

  status_code_domain(const status_code_domain &) = default;
  status_code_domain(status_code_domain &&) = default;
  status_code_domain &operator=(const status_code_domain &) = default;
  status_code_domain &operator=(status_code_domain &&) = default;
  ~status_code_domain() = default;

public:
  [[nodiscard]] constexpr unique_id_type id() const noexcept { return _id; }

  [[nodiscard]] constexpr bool
  operator==(const status_code_domain &o) const noexcept {
    return _id == o._id;
  }

  [[nodiscard]] constexpr auto
  operator<=>(const status_code_domain &o) const noexcept {
    return _id <=> o._id;
  }

  // Return the name of this domain
  [[nodiscard]] virtual string_ref name() const noexcept = 0;

  // Return payload size information for erased status codes
  [[nodiscard]] virtual payload_info_t payload_info() const noexcept {
    return {0, sizeof(void *) + sizeof(const status_code_domain *),
            alignof(void *)};
  }

  // Return true if this status code represents a failure
  [[nodiscard]] virtual bool
  _do_failure(const status_code<void> &code) const noexcept = 0;

  // Return true if code1 is semantically equivalent to code2
  [[nodiscard]] virtual bool
  _do_equivalent(const status_code<void> &code1,
                 const status_code<void> &code2) const noexcept = 0;

  // Return the generic_code equivalent, or empty generic_code if none
  [[nodiscard]] virtual generic_code
  _generic_code(const status_code<void> &code) const noexcept = 0;

  // Return the human-readable message for this code
  [[nodiscard]] virtual string_ref
  _do_message(const status_code<void> &code) const noexcept = 0;

#ifdef __cpp_exceptions
  // Throw this status code as an exception
  [[noreturn]] virtual void
  _do_throw_exception(const status_code<void> &code) const = 0;
#endif

  // Copy an erased status code's payload
  virtual void _do_erased_copy(status_code<void> &dst,
                               const status_code<void> &src,
                               payload_info_t info) const = 0;

  // Destroy an erased status code's payload
  virtual void _do_erased_destroy(status_code<void> &code,
                                  payload_info_t info) const noexcept = 0;
};

} // namespace outcome::core
