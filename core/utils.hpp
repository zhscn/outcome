#pragma once

#include <bit>
#include <cstdint>

namespace outcome::core {

namespace detail {

// Tag type for type-erased status codes
template <class T> struct erased {
  using value_type = T;
};

// constexpr strlen
constexpr inline std::size_t cstrlen(const char *s) noexcept {
  const char *p = s;
  while (*p != '\0') {
    ++p;
  }
  return static_cast<std::size_t>(p - s);
}

constexpr inline bool is_hex_digit(char c) noexcept {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

constexpr inline uint8_t hex2val(char c) noexcept {
  if (c >= '0' && c <= '9') {
    return static_cast<uint8_t>(c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return static_cast<uint8_t>(c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return static_cast<uint8_t>(c - 'A' + 10);
  }
  return 0;
}

constexpr inline uint8_t hex2byte(const char *p) noexcept {
  return static_cast<uint8_t>((hex2val(p[0]) << 4) | hex2val(p[1]));
}

// Parse a UUID string like "abcdef01-2345-6789-abcd-ef0123456789" and XOR
// high 64 bits with low 64 bits. Returns 0 if format is invalid.
constexpr inline uint64_t
parse_uuid_from_string(const char *uuid) noexcept {
  // uuid format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 characters)

  // Validate length (must be at least 36 characters)
  if (cstrlen(uuid) < 36) {
    return 0;
  }

  // Validate dashes at correct positions
  if (uuid[8] != '-' || uuid[13] != '-' || uuid[18] != '-' || uuid[23] != '-') {
    return 0;
  }

  // Validate hex digits in each section
  // Section 1: positions 0-7 (8 hex digits)
  for (int i = 0; i < 8; ++i) {
    if (!is_hex_digit(uuid[i])) {
      return 0;
    }
  }
  // Section 2: positions 9-12 (4 hex digits)
  for (int i = 9; i < 13; ++i) {
    if (!is_hex_digit(uuid[i])) {
      return 0;
    }
  }
  // Section 3: positions 14-17 (4 hex digits)
  for (int i = 14; i < 18; ++i) {
    if (!is_hex_digit(uuid[i])) {
      return 0;
    }
  }
  // Section 4: positions 19-22 (4 hex digits)
  for (int i = 19; i < 23; ++i) {
    if (!is_hex_digit(uuid[i])) {
      return 0;
    }
  }
  // Section 5: positions 24-35 (12 hex digits)
  for (int i = 24; i < 36; ++i) {
    if (!is_hex_digit(uuid[i])) {
      return 0;
    }
  }

  // Parse high 64 bits (first 8 bytes)
  uint64_t high = 0;
  // Parse first 4 bytes (positions 0-7)
  for (int i = 0; i < 4; ++i) {
    high = (high << 8) | hex2byte(uuid + i * 2);
  }
  // Skip dash at position 8, parse 2 bytes (positions 9-12)
  for (int i = 0; i < 2; ++i) {
    high = (high << 8) | hex2byte(uuid + 9 + i * 2);
  }
  // Skip dash at position 13, parse 2 bytes (positions 14-17)
  for (int i = 0; i < 2; ++i) {
    high = (high << 8) | hex2byte(uuid + 14 + i * 2);
  }

  // Parse low 64 bits (last 8 bytes)
  uint64_t low = 0;
  // Skip dash at position 18, parse 2 bytes (positions 19-22)
  for (int i = 0; i < 2; ++i) {
    low = (low << 8) | hex2byte(uuid + 19 + i * 2);
  }
  // Skip dash at position 23, parse 6 bytes (positions 24-35)
  for (int i = 0; i < 6; ++i) {
    low = (low << 8) | hex2byte(uuid + 24 + i * 2);
  }

  return high ^ low;
}

// erasure_cast: cast between trivially copyable types via bit_cast
// Supports To/From of the same size directly, or smaller From into larger To
// (zero-padded).
template <class To, class From>
  requires(std::is_trivially_copyable_v<To> &&
           std::is_trivially_copyable_v<From> && sizeof(To) <= sizeof(From))
constexpr To erasure_cast(const From &from) noexcept {
  if constexpr (sizeof(To) == sizeof(From)) {
    return std::bit_cast<To>(from);
  } else {
    struct alignas(To) padded {
      From value;
      char padding[sizeof(To) - sizeof(From)]{};
    };
    padded p{from, {}};
    return std::bit_cast<To>(p);
  }
}

} // namespace detail

} // namespace outcome::core
