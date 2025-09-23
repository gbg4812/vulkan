#pragma once
#include <type_traits>
#include <variant>
namespace gbg {
template <typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

template <typename T, typename VT>
struct variant_holds_type : std::false_type {};

template <typename T, typename... Ts>
struct variant_holds_type<T, std::variant<Ts...>>
    : std::disjunction<std::is_same<T, Ts>...> {};

// Helper variable template
template <typename T, typename Variant>
inline constexpr bool variant_holds_type_v =
    variant_holds_type<T, Variant>::value;

template <typename T>
struct is_variant : std::false_type {};

template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template <typename T>
constexpr bool is_variant_v = is_variant<T>::value;
};  // namespace gbg
