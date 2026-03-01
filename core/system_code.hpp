#pragma once

#include "posix_code.hpp"

#include <cstdint>

namespace outcome::core {

// system_code: a type-erased status code that fits in exactly 2 pointers.
// On POSIX, this erases posix_code; on Windows, it would erase
// win32_code/nt_code.
using system_code = erased_status_code<intptr_t>;

#ifndef NDEBUG
static_assert(sizeof(system_code) == 2 * sizeof(void *),
              "system_code is not exactly two pointers in size!");
#endif

// error: an errored system_code — always represents a failure.
// The closest equivalent to std::error_code, except immutable.
using error = erased_errored_status_code<typename system_code::value_type>;

#ifndef NDEBUG
static_assert(sizeof(error) == 2 * sizeof(void *),
              "error is not exactly two pointers in size!");
#endif

} // namespace outcome::core
