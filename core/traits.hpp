#pragma once

#include <type_traits>

namespace outcome::core {

// Forward declarations
class status_code_domain;
template <class DomainType> class status_code;
template <class DomainType> class errored_status_code;
template <class DomainType> class status_error;

// Concepts
namespace traits {

// A type is move-bitcopying if trivially copyable and trivially destructible
template <class T>
struct is_move_bitcopying
    : std::bool_constant<std::is_trivially_copyable_v<T>> {};

// Detect if T is a status_code specialization
template <class T> struct is_status_code : std::false_type {};
template <class D> struct is_status_code<status_code<D>> : std::true_type {};
template <class D>
struct is_status_code<const status_code<D>> : std::true_type {};

// Detect if T is an errored_status_code specialization
template <class T> struct is_errored_status_code : std::false_type {};
template <class D>
struct is_errored_status_code<errored_status_code<D>> : std::true_type {};
template <class D>
struct is_errored_status_code<const errored_status_code<D>> : std::true_type {};

} // namespace traits

template <class T>
concept MoveBitCopying = traits::is_move_bitcopying<T>::value;

template <class T>
concept IsStatusCode =
    traits::is_status_code<std::remove_cvref_t<T>>::value ||
    traits::is_errored_status_code<std::remove_cvref_t<T>>::value;

template <class T>
concept IsErasedStatusCode = requires {
  typename std::remove_cvref_t<T>::domain_type;
} && requires { requires IsStatusCode<T>; };

// ADL detection: does make_status_code(T) exist?
template <class T, class... Args>
concept HasMakeStatusCode = requires(T &&t, Args &&...args) {
  make_status_code(std::forward<T>(t), std::forward<Args>(args)...);
};

// Type aliases used across the library
using generic_code = status_code<class _generic_code_domain>;

} // namespace outcome::core
