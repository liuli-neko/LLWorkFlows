#pragma once

#include <stdio.h>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <array>
#include <queue>

#define COLOR_BLACK "30"
#define COLOR_RED "31"
#define COLOR_GREEN "32"
#define COLOR_YELLOW "33"
#define COLOR_BLUE "34"
#define COLOR_PURPLE "35"
#define COLOR_CYAN "36"
#define COLOR_WHITE "37"
#define COLOR_GRAY "90"


#define TTY_BLUE(str) \
    "\033[" COLOR_BLUE "m" str "\033[0m"
#define TTY_GRAY(str) \
    "\033[" COLOR_GRAY "m" str "\033[0m"
#define TTY_YELLOW(str) \
    "\033[" COLOR_YELLOW "m" str "\033[0m"
#define TTY_RED(str) \
    "\033[" COLOR_RED "m" str "\033[0m"
#define TTY_PURPLE(str) \
    "\033[" COLOR_PURPLE "m" str "\033[0m"
#define LOG(fmt, ...)                   \
    fprintf(stdout, fmt, ##__VA_ARGS__);
#define LOG_INFO(fmt, ...) \
    LOG(TTY_GRAY("INFO    -- [%s:%d(%s)]>> ") fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#define LOG_WARNING(fmt, ...) \
    LOG(TTY_YELLOW("WARNING -- [%s:%d(%s)]>> ") fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#define LOG_ERROR(fmt, ...) \
    LOG(TTY_RED("ERROR   -- [%s:%d(%s)]>> ") fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#define LOG_FATAL(fmt, ...) \
    LOG(TTY_PURPLE("FATAL   -- [%s:%d(%s)]>> ") fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);\
    exit(-1);

#ifdef NDEBUG
#define LOG_DEBUG(fmt, ...) \
    LOG(TTY_BLUE("DEBUG   -- [%s:%d(%s)]>> ") fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__);
#else
#define LOG_DEBUG(fmt, ...)
#endif

#define NEKO_ENUM_SEARCH_DEPTH 60
#ifdef __GNUC__
#define NEKO_FUNCTION __PRETTY_FUNCTION__

template <typename T, T Value>
constexpr auto _Neko_GetEnumName() noexcept {
    // for enum, this function like constexpr auto _Neko_GetEnumName() [with T = MyEnum; T Value = MyValues]
    std::string_view name(NEKO_FUNCTION);
    size_t eqBegin = name.find_last_of(' ');
    size_t end = name.find_last_of(']');
    std::string_view body = name.substr(eqBegin + 1, end - eqBegin - 1);
    if (body[0] == '(') {
        // Failed to get 
        return std::string_view();
    }
    return body;
}
#elif defined(_MSC_VER)
#define NEKO_FUNCTION __FUNCSIG__
template <typename T, T Value>
constexpr auto _Neko_GetEnumName() noexcept {
    // for enum, this function like auto __cdecl _Neko_GetEnumName<enum main::MyEnum,main::MyEnum::MyValue>(void)
    std::string_view name(__FUNCSIG__);
    size_t dotBegin = name.find_first_of(',');
    size_t end = name.find_last_of('>');
    std::string_view body = name.substr(dotBegin + 1, end - dotBegin - 1);
    if (body[0] == '(') {
        // Failed to get
        return std::string_view();
    }
    return body;
}
#else
#define NEKO_FUNCTION __FUNCTION__
template <typename T, T Value>
constexpr auto _Neko_GetEnumName() noexcept {
    return std::string_view();
}
#endif

//> Enum to string
template <typename T, T Value>
constexpr bool _Neko_IsValidEnum() noexcept {
    return !_Neko_GetEnumName<T, Value>().empty();
}
template <typename T, size_t ...N>
constexpr size_t _Neko_GetValidEnumCount(std::index_sequence<N...> seq) noexcept {
    return (... + _Neko_IsValidEnum<T, T(N)>());
}
template <typename T, size_t ...N>
constexpr auto _Neko_GetValidEnumNames(std::index_sequence<N...> seq) noexcept {
    constexpr auto validCount = _Neko_GetValidEnumCount<T>(seq);

    std::array<std::pair<T, std::string_view>, validCount> arr;
    std::string_view vstr[sizeof ...(N)] {
        _Neko_GetEnumName<T, T(N)>()...
    };

    size_t n = 0;
    auto iter = arr.begin();

    for (auto i : vstr) {
        if (!i.empty()) {
            iter->first = T(n);
            iter->second = i;
            ++iter;
        } else {
            break;
        }

        n += 1;
    }

    return arr;
}
template <typename T>
inline std::string _Neko_EnumToString(T en) {
    constexpr auto map = _Neko_GetValidEnumNames<T>(std::make_index_sequence<NEKO_ENUM_SEARCH_DEPTH>());
    for (auto [value, name] : map) {
        if (value == en) {
            return std::string(name);
        }
    }
    return std::to_string(std::underlying_type_t<T>(en));
}

template <typename T,
          typename _Cond = std::enable_if_t<std::is_arithmetic_v<T>>>
inline std::string ToString(T val) {
    if constexpr (std::is_same_v<T, bool>) {
        if (val) {
            return "true";
        }
        return "false";
    }
    return std::to_string(val);
}

template <typename T,
          typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
inline std::string ToString(T en) {
    return _Neko_EnumToString(en);
}

inline std::string ToString(const std::string &s) {
    return '"' + s + '"';
}

inline std::string ToString(std::string_view &s) {
    return '"' + std::string(s) + '"';
}