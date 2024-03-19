#pragma once

#include <version>

#ifdef __cpp_lib_generator

#include <generator>

template<class... Ts>
using Generator = std::generator<Ts...>;

#else

#include <experimental/generator>

template<class... Ts>
using Generator = std::experimental::generator<Ts...>;

#endif