#pragma once

#include "status_error.hpp"

#include <cerrno>
#include <cstring>

namespace outcome::core {

// POSIX error codes as a strongly-typed enum
enum class errc : int {
  success = 0,
  unknown = -1,
  address_family_not_supported = EAFNOSUPPORT,
  address_in_use = EADDRINUSE,
  address_not_available = EADDRNOTAVAIL,
  already_connected = EISCONN,
  argument_list_too_long = E2BIG,
  argument_out_of_domain = EDOM,
  bad_address = EFAULT,
  bad_file_descriptor = EBADF,
  bad_message = EBADMSG,
  broken_pipe = EPIPE,
  connection_aborted = ECONNABORTED,
  connection_already_in_progress = EALREADY,
  connection_refused = ECONNREFUSED,
  connection_reset = ECONNRESET,
  cross_device_link = EXDEV,
  destination_address_required = EDESTADDRREQ,
  device_or_resource_busy = EBUSY,
  directory_not_empty = ENOTEMPTY,
  executable_format_error = ENOEXEC,
  file_exists = EEXIST,
  file_too_large = EFBIG,
  filename_too_long = ENAMETOOLONG,
  function_not_supported = ENOSYS,
  host_unreachable = EHOSTUNREACH,
  identifier_removed = EIDRM,
  illegal_byte_sequence = EILSEQ,
  inappropriate_io_control_operation = ENOTTY,
  interrupted = EINTR,
  invalid_argument = EINVAL,
  invalid_seek = ESPIPE,
  io_error = EIO,
  is_a_directory = EISDIR,
  message_size = EMSGSIZE,
  network_down = ENETDOWN,
  network_reset = ENETRESET,
  network_unreachable = ENETUNREACH,
  no_buffer_space = ENOBUFS,
  no_child_process = ECHILD,
  no_link = ENOLINK,
  no_lock_available = ENOLCK,
  no_message = ENOMSG,
  no_protocol_option = ENOPROTOOPT,
  no_space_on_device = ENOSPC,
  no_such_device_or_address = ENXIO,
  no_such_device = ENODEV,
  no_such_file_or_directory = ENOENT,
  no_such_process = ESRCH,
  not_a_directory = ENOTDIR,
  not_a_socket = ENOTSOCK,
  not_connected = ENOTCONN,
  not_enough_memory = ENOMEM,
  not_supported = ENOTSUP,
  operation_canceled = ECANCELED,
  operation_in_progress = EINPROGRESS,
  operation_not_permitted = EPERM,
  operation_would_block = EWOULDBLOCK,
  owner_dead = EOWNERDEAD,
  permission_denied = EACCES,
  protocol_error = EPROTO,
  protocol_not_supported = EPROTONOSUPPORT,
  read_only_file_system = EROFS,
  resource_deadlock_would_occur = EDEADLK,
  resource_unavailable_try_again = EAGAIN,
  result_out_of_range = ERANGE,
  state_not_recoverable = ENOTRECOVERABLE,
  text_file_busy = ETXTBSY,
  timed_out = ETIMEDOUT,
  too_many_files_open_in_system = ENFILE,
  too_many_files_open = EMFILE,
  too_many_links = EMLINK,
  too_many_symbolic_link_levels = ELOOP,
  value_too_large = EOVERFLOW,
  wrong_protocol_type = EPROTOTYPE,
};

// ============================================================================
// _generic_code_domain
// ============================================================================
class _generic_code_domain : public status_code_domain {
  using _self = _generic_code_domain;

public:
  using value_type = errc;

  constexpr explicit _generic_code_domain(
      unique_id_type id = 0x746d6354f4f733e9ULL) noexcept
      : status_code_domain(id) {}

  static const _self &get() noexcept {
    static const _self singleton{};
    return singleton;
  }

  [[nodiscard]] string_ref name() const noexcept override {
    return string_ref("generic domain");
  }

  [[nodiscard]] payload_info_t payload_info() const noexcept override {
    return {sizeof(value_type),
            sizeof(status_code_domain *) + sizeof(value_type),
            alignof(value_type)};
  }

protected:
  [[nodiscard]] bool
  _do_failure(const status_code<void> &code) const noexcept override {
    return detail::downcast<_self>(code).value() != errc::success;
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
    return generic_code(detail::downcast<_self>(code).value());
  }

  [[nodiscard]] string_ref
  _do_message(const status_code<void> &code) const noexcept override {
    auto val = static_cast<int>(detail::downcast<_self>(code).value());
    if (val == -1)
      return string_ref("unknown error");
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
    // Layout: [domain_ptr | value]. Just memcpy the whole thing.
    std::memcpy(&dst, &src, sizeof(status_code_domain *) + sizeof(value_type));
  }

  void _do_erased_destroy(status_code<void> & /*code*/,
                          payload_info_t /*info*/) const noexcept override {}
};

inline const _generic_code_domain &generic_code_domain = // NOLINT
    _generic_code_domain::get();

// ADL hook: construct generic_code from errc
inline generic_code make_status_code(errc c) noexcept {
  return generic_code(c);
}

// Now define equivalent() which needs generic_code to be complete
template <class T>
bool status_code<void>::equivalent(const status_code<T> &o) const noexcept {
  if (strictly_equivalent(o))
    return true;
  const auto *o_domain = reinterpret_cast<const status_code<void> &>(o)._domain;
  if (_domain && o_domain) {
    generic_code gc1 = _domain->_generic_code(*this);
    generic_code gc2 =
        o_domain->_generic_code(reinterpret_cast<const status_code<void> &>(o));
    if (gc1.value() != errc::unknown && gc2.value() != errc::unknown) {
      return gc1.value() == gc2.value();
    }
  }
  return false;
}

using generic_error = errored_status_code<_generic_code_domain>;

} // namespace outcome::core
