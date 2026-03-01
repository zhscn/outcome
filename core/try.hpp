#pragma once

#include "outcome.hpp"

namespace outcome::core {

// ============================================================================
// TRY customization points — found via ADL
// ============================================================================

template <class T>
  requires requires(T &&v) { v.has_value(); }
constexpr inline bool try_operation_has_value(T &&v) noexcept {
  return v.has_value();
}

namespace detail {

template <class T>
concept HasAsFailure = requires(T &&v) {
  { static_cast<T &&>(v).as_failure() };
};

} // namespace detail

template <class T>
  requires detail::HasAsFailure<T>
constexpr inline decltype(auto) try_operation_return_as(T &&v) {
  return static_cast<T &&>(v).as_failure();
}

template <class T>
  requires requires(T &&v) { v.assume_value(); }
constexpr inline decltype(auto) try_operation_extract_value(T &&v) {
  return static_cast<T &&>(v).assume_value();
}

} // namespace outcome::core

// ============================================================================
// TRY macros
// ============================================================================

#define OUTCOME_TRY_GLUE2(x, y) x##y
#define OUTCOME_TRY_GLUE(x, y) OUTCOME_TRY_GLUE2(x, y)
#define OUTCOME_TRY_UNIQUE_NAME OUTCOME_TRY_GLUE(_outcome_try_, __COUNTER__)

// Internal: TRYV with pre-generated unique name
#define OUTCOME_TRYV_IMPL(unique, ...)                                         \
  auto unique = (__VA_ARGS__);                                                 \
  if (!::outcome::core::try_operation_has_value(unique)) [[unlikely]]          \
  return ::outcome::core::try_operation_return_as(                             \
      static_cast<decltype(unique) &&>(unique))

#define OUTCOME_TRYV(...)                                                      \
  OUTCOME_TRYV_IMPL(OUTCOME_TRY_UNIQUE_NAME, __VA_ARGS__)

// OUTCOME_TRYV2(failure_handler, expr)
#define OUTCOME_TRYV2_IMPL(unique, failure_handler, ...)                       \
  auto unique = (__VA_ARGS__);                                                 \
  if (!::outcome::core::try_operation_has_value(unique)) [[unlikely]]          \
  return failure_handler(static_cast<decltype(unique) &&>(unique))

#define OUTCOME_TRYV2(failure_handler, ...)                                    \
  OUTCOME_TRYV2_IMPL(OUTCOME_TRY_UNIQUE_NAME, failure_handler, __VA_ARGS__)

// OUTCOME_TRY(var, expr)
#define OUTCOME_TRY_IMPL(unique, var, ...)                                     \
  auto unique = (__VA_ARGS__);                                                 \
  if (!::outcome::core::try_operation_has_value(unique)) [[unlikely]]          \
    return ::outcome::core::try_operation_return_as(                           \
        static_cast<decltype(unique) &&>(unique));                             \
  auto &&var = ::outcome::core::try_operation_extract_value(                   \
      static_cast<decltype(unique) &&>(unique))

#define OUTCOME_TRY_DISPATCH2(var, ...)                                        \
  OUTCOME_TRY_IMPL(OUTCOME_TRY_UNIQUE_NAME, var, __VA_ARGS__)
#define OUTCOME_TRY_DISPATCH1(...) OUTCOME_TRYV(__VA_ARGS__)

#define OUTCOME_TRY_SELECT(_1, _2, NAME, ...) NAME
#define OUTCOME_TRY(...)                                                       \
  OUTCOME_TRY_SELECT(__VA_ARGS__, OUTCOME_TRY_DISPATCH2,                       \
                     OUTCOME_TRY_DISPATCH1)                                    \
  (__VA_ARGS__)

// ============================================================================
// Coroutine variants
// ============================================================================

#define OUTCOME_CO_TRYV_IMPL(unique, ...)                                      \
  auto unique = (__VA_ARGS__);                                                 \
  if (!::outcome::core::try_operation_has_value(unique)) [[unlikely]]          \
  co_return ::outcome::core::try_operation_return_as(                          \
      static_cast<decltype(unique) &&>(unique))

#define OUTCOME_CO_TRYV(...)                                                   \
  OUTCOME_CO_TRYV_IMPL(OUTCOME_TRY_UNIQUE_NAME, __VA_ARGS__)

#define OUTCOME_CO_TRYV2_IMPL(unique, failure_handler, ...)                    \
  auto unique = (__VA_ARGS__);                                                 \
  if (!::outcome::core::try_operation_has_value(unique)) [[unlikely]]          \
  co_return failure_handler(static_cast<decltype(unique) &&>(unique))

#define OUTCOME_CO_TRYV2(failure_handler, ...)                                 \
  OUTCOME_CO_TRYV2_IMPL(OUTCOME_TRY_UNIQUE_NAME, failure_handler, __VA_ARGS__)

#define OUTCOME_CO_TRY_IMPL(unique, var, ...)                                  \
  auto unique = (__VA_ARGS__);                                                 \
  if (!::outcome::core::try_operation_has_value(unique)) [[unlikely]]          \
    co_return ::outcome::core::try_operation_return_as(                        \
        static_cast<decltype(unique) &&>(unique));                             \
  auto &&var = ::outcome::core::try_operation_extract_value(                   \
      static_cast<decltype(unique) &&>(unique))

#define OUTCOME_CO_TRY_DISPATCH2(var, ...)                                     \
  OUTCOME_CO_TRY_IMPL(OUTCOME_TRY_UNIQUE_NAME, var, __VA_ARGS__)
#define OUTCOME_CO_TRY_DISPATCH1(...) OUTCOME_CO_TRYV(__VA_ARGS__)

#define OUTCOME_CO_TRY_SELECT(_1, _2, NAME, ...) NAME
#define OUTCOME_CO_TRY(...)                                                    \
  OUTCOME_CO_TRY_SELECT(__VA_ARGS__, OUTCOME_CO_TRY_DISPATCH2,                 \
                        OUTCOME_CO_TRY_DISPATCH1)                              \
  (__VA_ARGS__)

// ============================================================================
// GCC/Clang statement expression variant
// ============================================================================
#if defined(__GNUC__) || defined(__clang__)
#define OUTCOME_TRYX(...)                                                      \
  ({                                                                           \
    auto _outcome_tryx = (__VA_ARGS__);                                        \
    if (!::outcome::core::try_operation_has_value(_outcome_tryx))              \
        [[unlikely]] {                                                         \
      return ::outcome::core::try_operation_return_as(                         \
          static_cast<decltype(_outcome_tryx) &&>(_outcome_tryx));             \
    }                                                                          \
    ::outcome::core::try_operation_extract_value(                              \
        static_cast<decltype(_outcome_tryx) &&>(_outcome_tryx));               \
  })

#define OUTCOME_CO_TRYX(...)                                                   \
  ({                                                                           \
    auto _outcome_tryx = (__VA_ARGS__);                                        \
    if (!::outcome::core::try_operation_has_value(_outcome_tryx))              \
        [[unlikely]] {                                                         \
      co_return ::outcome::core::try_operation_return_as(                      \
          static_cast<decltype(_outcome_tryx) &&>(_outcome_tryx));             \
    }                                                                          \
    ::outcome::core::try_operation_extract_value(                              \
        static_cast<decltype(_outcome_tryx) &&>(_outcome_tryx));               \
  })
#endif
