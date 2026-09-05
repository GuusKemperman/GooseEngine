// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// In a module-file, the optional `module;` must appear first; see [cpp.pre].
module;

// This named module expects to be built with classic headers, not header units.
#define _BUILD_STD_MODULE

// The subset of "C headers" [tab:c.headers] corresponding to
// the "C++ headers for C library facilities" [tab:headers.cpp.c]
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fenv.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <uchar.h>
#include <wchar.h>
#include <wctype.h>

// <intrin.h> defines some types outside of `extern "C"` or `extern "C++"`.
#include <intrin.h>

export module stl;

#pragma warning( push )
#pragma warning( disable : 5244 ) // '#include <meow>' in the purview of module 'std' appears erroneous.

// "C++ library headers" [tab:headers.cpp]
#include <algorithm>
#if _HAS_STATIC_RTTI
#include <any>
#endif // _HAS_STATIC_RTTI
#include <array>
#include <atomic>
#include <barrier>
#include <bit>
#include <bitset>
#include <charconv>
#include <chrono>
#include <codecvt>
#include <compare>
#include <complex>
#include <concepts>
#include <coroutine>
#include <deque>
#include <exception>
#include <execution>

#include <condition_variable>
#if _HAS_CXX23
#include <expected>
#endif // _HAS_CXX23
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>

#include <forward_list>
#if _HAS_CXX23
#include <generator>
#endif // _HAS_CXX23
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <latch>
#include <limits>
#include <list>
#include <locale>
#include <map>

#include <initializer_list>
#if _HAS_CXX23
#include <mdspan>
#endif // _HAS_CXX23
#include <memory>
#include <mutex>
#include <new>
#include <numbers>
#include <numeric>
#include <optional>
#include <ostream>

#include <memory_resource>
#if _HAS_CXX23
#include <print>
#endif // _HAS_CXX23
#include <queue>
#include <random>
#include <ranges>
#include <ratio>
#include <regex>
#include <semaphore>
#include <set>
#include <span>

#include <scoped_allocator>
#include <shared_mutex>
#include <source_location>
#if _HAS_CXX23
#include <spanstream>
#endif // _HAS_CXX23
#include <sstream>
#include <stack>
#if _HAS_CXX23
#include <stacktrace>
#endif // _HAS_CXX23
#include <stdexcept>
#if _HAS_CXX23
#include <stdfloat>
#endif // _HAS_CXX23
#include <streambuf>
#include <string>
#include <strstream>
#include <syncstream>
#include <thread>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <valarray>
#include <variant>
#include <vector>
#include <version>

#include <stop_token>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

// "C++ headers for C library facilities" [tab:headers.cpp.c]
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfenv>
#include <cfloat>
#include <cinttypes>
#include <climits>
#include <clocale>
#include <cmath>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cuchar>
#include <cwchar>
#include <cwctype>

#pragma warning( pop )
