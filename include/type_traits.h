#ifndef ESQLITE_TYPE_TRAITS_H
#define ESQLITE_TYPE_TRAITS_H

#pragma once

#include <tuple>

namespace esqlite {
template <class T, class... Args>
inline constexpr bool is_brace_constructible_v =
    requires { T{std::declval<Args>()...}; };

struct any_type {
  template <typename T> constexpr operator T() { std::unreachable(); }
};

template <typename T, typename... Ts>
inline constexpr bool is_any_of_v = (std::is_same_v<T, Ts> || ...);

template <typename T>
inline constexpr bool is_sqlite_numeric_v =
    is_any_of_v<std::decay_t<T>, int, int64_t, double>;

template <typename T>
inline constexpr bool is_sqlite_null =
    is_any_of_v<std::decay_t<T>, void, nullptr_t>;

template <typename T>
inline constexpr bool is_sqlite_text =
    is_any_of_v<std::decay_t<T>, std::string, std::string_view>;

template <typename T>
inline constexpr bool is_sqlite_blob =
    is_any_of_v<std::decay_t<T>, std::vector<char>, std::vector<const char>,
                std::vector<uint8_t>, std::vector<const uint8_t>,
                std::span<char>, std::span<const char>, std::span<uint8_t>,
                std::span<const uint8_t>>;

template <class T> auto asRefTuple(T &Object) noexcept {
  using type = std::decay_t<T>;
  if constexpr (is_brace_constructible_v<type, any_type, any_type, any_type,
                                         any_type, any_type, any_type, any_type,
                                         any_type, any_type, any_type>) {
    auto &[p1, p2, p3, p4, p5, p6, p7, p8, p9, p10] = Object;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
  } else if constexpr (is_brace_constructible_v<
                           type, any_type, any_type, any_type, any_type,
                           any_type, any_type, any_type, any_type, any_type>) {
    auto &[p1, p2, p3, p4, p5, p6, p7, p8, p9] = Object;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8, p9);
  } else if constexpr (is_brace_constructible_v<type, any_type, any_type,
                                                any_type, any_type, any_type,
                                                any_type, any_type, any_type>) {
    auto &[p1, p2, p3, p4, p5, p6, p7, p8] = Object;
    return std::tie(p1, p2, p3, p4, p5, p6, p7, p8);
  } else if constexpr (is_brace_constructible_v<type, any_type, any_type,
                                                any_type, any_type, any_type,
                                                any_type, any_type>) {
    auto &[p1, p2, p3, p4, p5, p6, p7] = Object;
    return std::tie(p1, p2, p3, p4, p5, p6, p7);
  } else if constexpr (is_brace_constructible_v<type, any_type, any_type,
                                                any_type, any_type, any_type,
                                                any_type>) {
    auto &[p1, p2, p3, p4, p5, p6] = Object;
    return std::tie(p1, p2, p3, p4, p5, p6);
  } else if constexpr (is_brace_constructible_v<type, any_type, any_type,
                                                any_type, any_type, any_type>) {
    auto &[p1, p2, p3, p4, p5] = Object;
    return std::tie(p1, p2, p3, p4, p5);
  } else if constexpr (is_brace_constructible_v<type, any_type, any_type,
                                                any_type, any_type>) {
    auto &[p1, p2, p3, p4] = Object;
    return std::tie(p1, p2, p3, p4);
  } else if constexpr (is_brace_constructible_v<type, any_type, any_type,
                                                any_type>) {
    auto &[p1, p2, p3] = Object;
    return std::tie(p1, p2, p3);
  } else if constexpr (is_brace_constructible_v<type, any_type, any_type>) {
    auto &[p1, p2] = Object;
    return std::tie(p1, p2);
  } else if constexpr (is_brace_constructible_v<type, any_type>) {
    auto &[p1] = Object;
    return std::tie(p1);
  } else {
    return std::tie();
  }
}

static_assert(is_any_of_v<int, double, int>);
static_assert(!is_any_of_v<int32_t, double, int64_t>);

template <typename F, std::size_t... S>
constexpr void staticFor(F &&Function, std::index_sequence<S...>) {
  int Unpack[] = {
      0, void(Function(std::integral_constant<std::size_t, S>{}), 0)...};

  (void)Unpack;
}

template <std::size_t iterations, typename F>
constexpr void staticFor(F &&Function) {
  static_for(std::forward<F>(Function), std::make_index_sequence<iterations>());
}

} // namespace esqlite

#endif // ESQLITE_TYPE_TRAITS_H
