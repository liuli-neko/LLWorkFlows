#pragma once

#include <time.h>

#include <chrono>
#include <ctime>
#include <string_view>

#include "enumToString.hpp"
#include "workflowsglobal.hpp"

#if !defined(NDEBUG) && !defined(LLWFLOWS_NDEBUG)
#define LLWFLOWS_DEBUG_ENABLE
#endif

#if !defined(LLWFLOWS_NLOG)
#define LLWFLOWS_LOG
#endif

#if defined(LLWFLOWS_DEBUG_ENABLE) || defined(LLWFLOWS_LOG)
#if LLWFLOWS_CPP_PLUS >= 20
#include <version>
#if defined(__cpp_lib_format)
#include <format>
#define LLWFLOWS_STD_FORMAT
#endif
#endif

#if defined(LLWFLOWS_STD_FORMAT)
#define LLWFLOWS_FORMAT std::format
template <typename T>
requires std::is_enum_v<std::remove_cvref_t<T>>
struct std::formatter<T> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(const std::remove_cvref_t<T> t, FormatContext& ctx) -> decltype(ctx.out()) {
        auto str = LLWFLOWS_NAMESPACE::detail::enumToString(t);
        if(str.empty()) {
            return std::format_to(ctx.out(), "{}", (int)t);
        } else {
            return std::format_to(ctx.out(), "{}", str);
        }
    }
};
#else
#include <fmt/format.h>
#define LLWFLOWS_FORMAT fmt::format
template <typename T>
struct fmt::formatter<T, char, std::enable_if_t<std::is_enum_v<T> > > {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

    template <typename FormatContext>
    constexpr auto format(T t, FormatContext& ctx) -> decltype(ctx.out()) {
        auto str = LLWFLOWS_NAMESPACE::detail::enumToString(t);
        if(str.empty()) {
            return fmt::format_to(ctx.out(), "{}", (int)t);
        } else {
            return fmt::format_to(ctx.out(), "{}", str);
        }
    }
};
#endif
#endif

#ifndef LLWFLOWS_LOG_OUTPUT
#define LLWFLOWS_LOG_OUTPUT stdout
#endif

#ifdef LLWFLOWS_LOG_LEVEL_DEBUG
#undef LLWFLOWS_LOG_LEVEL_DEBUG
#define LLWFLOWS_LOG_LEVEL 0
#endif
#ifdef LLWFLOWS_LOG_LEVEL_INFO
#undef LLWFLOWS_LOG_LEVEL_INFO
#define LLWFLOWS_LOG_LEVEL 1
#endif
#ifdef LLWFLOWS_LOG_LEVEL_WARN
#undef LLWFLOWS_LOG_LEVEL_WARN
#define LLWFLOWS_LOG_LEVEL 2
#endif
#ifdef LLWFLOWS_LOG_LEVEL_ERROR
#undef LLWFLOWS_LOG_LEVEL_ERROR
#define LLWFLOWS_LOG_LEVEL 3
#endif
#ifdef LLWFLOWS_LOG_LEVEL_FATAL
#undef LLWFLOWS_LOG_LEVEL_FATAL
#define LLWFLOWS_LOG_LEVEL 4
#endif
#ifdef LLWFLOWS_LOG_LEVEL_NONE
#undef LLWFLOWS_LOG_LEVEL_NONE
#define LLWFLOWS_LOG_LEVEL 5
#endif

#define LLWFLOWS_LOG_LEVEL_DEBUG 0
#define LLWFLOWS_LOG_LEVEL_INFO  1
#define LLWFLOWS_LOG_LEVEL_WARN  2
#define LLWFLOWS_LOG_LEVEL_ERROR 3
#define LLWFLOWS_LOG_LEVEL_FATAL 4
#define LLWFLOWS_LOG_LEVEL_NONE  5

#ifndef LLWFLOWS_LOG_LEVEL
#define LLWFLOWS_LOG_LEVEL LLWFLOWS_LOG_LEVEL_INFO
#endif

LLWFLOWS_NS_BEGIN
namespace debug {
inline bool supportsColor() {
#ifdef __linux__
    const char* term = std::getenv("TERM");
    return term &&
           (std::string(term) == "xterm" || std::string(term) == "xterm-256color" || std::string(term) == "screen" ||
            std::string(term) == "screen-256color" || std::string(term) == "linux");
#else
    return false;
#endif
}

#define LINUX_ANSI_COLOR_TABLE                 \
    LINUX_ANSI_COLOR(reset, "\033[0m")         \
    LINUX_ANSI_COLOR(black, "\033[30m")        \
    LINUX_ANSI_COLOR(red, "\033[31m")          \
    LINUX_ANSI_COLOR(green, "\033[32m")        \
    LINUX_ANSI_COLOR(yellow, "\033[33m")       \
    LINUX_ANSI_COLOR(blue, "\033[34m")         \
    LINUX_ANSI_COLOR(magenta, "\033[35m")      \
    LINUX_ANSI_COLOR(cyan, "\033[36m")         \
    LINUX_ANSI_COLOR(lightGray, "\033[37m")    \
    LINUX_ANSI_COLOR(darkGray, "\033[90m")     \
    LINUX_ANSI_COLOR(lightRed, "\033[91m")     \
    LINUX_ANSI_COLOR(lightGreen, "\033[92m")   \
    LINUX_ANSI_COLOR(lightYellow, "\033[93m")  \
    LINUX_ANSI_COLOR(lightBlue, "\033[94m")    \
    LINUX_ANSI_COLOR(lightMagenta, "\033[95m") \
    LINUX_ANSI_COLOR(lightCyan, "\033[96m")    \
    LINUX_ANSI_COLOR(white, "\033[97m")

#define LINUX_ANSI_COLOR(color, code)             \
    inline constexpr const char* color##Color() { \
        if (supportsColor) return code;           \
        return "";                                \
    }

LINUX_ANSI_COLOR_TABLE

#undef LINUX_ANSI_COLOR
#undef LINUX_ANSI_COLOR_TABLE

}  // namespace debug
LLWFLOWS_NS_END

#ifdef LLWFLOWS_LOG_CONTEXT
#define LLWFLOWS_LOG_OUTPUT_FUNC(level, color, fmt, ...)                                                            \
    {                                                                                                               \
        auto file_offset                    = std::string_view(__FILE__).find_last_of("/");                         \
        auto now                            = std::chrono::system_clock::now();                                     \
        auto now_c                          = std::chrono::system_clock::to_time_t(now);                            \
        auto time_str                       = std::ctime(&now_c);                                                   \
        time_str[std::strlen(time_str) - 1] = '\0';                                                                 \
        fprintf(LLWFLOWS_LOG_OUTPUT, "%s: %s\n",                                                                    \
                LLWFLOWS_FORMAT("{}{:<8}- [{}][{}:{}][{}]{}", color(), level, time_str,                             \
                                file_offset == std::string::npos ? __FILE__ : __FILE__ + file_offset + 1, __LINE__, \
                                __FUNCTION__, LLWFLOWS_NAMESPACE::debug::resetColor())                              \
                    .c_str(),                                                                                       \
                LLWFLOWS_FORMAT(fmt, ##__VA_ARGS__).c_str());                                                       \
    }
#else
#define LLWFLOWS_LOG_OUTPUT_FUNC(level, color, ...)                                                          \
    {                                                                                                        \
        auto now                            = std::chrono::system_clock::now();                              \
        auto now_c                          = std::chrono::system_clock::to_time_t(now);                     \
        auto time_str                       = std::ctime(&now_c);                                            \
        time_str[std::strlen(time_str) - 1] = '\0';                                                          \
        fprintf(LLWFLOWS_LOG_OUTPUT, "%s%s%s[%s]: %s\n", color(), LLWFLOWS_FORMAT("{:<8}- ", level).c_str(), \
                LLWFLOWS_NAMESPACE::debug::resetColor(), time_str, LLWFLOWS_FORMAT(__VA_ARGS__).c_str());    \
    }
#endif

#if defined(LLWFLOWS_DEBUG_ENABLE)
#define LLWFLOWS_DEBUG(...) LLWFLOWS_LOG_OUTPUT_FUNC("debug", LLWFLOWS_NAMESPACE::debug::blueColor, __VA_ARGS__);

#define LLWFLOWS_ASSERT(cond, ...)                                                             \
    if (!(cond)) {                                                                             \
        LLWFLOWS_LOG_OUTPUT_FUNC("%assert", LLWFLOWS_NAMESPACE::debug::redColor, __VA_ARGS__); \
        abort();                                                                               \
    }
#else
#define LLWFLOWS_ASSERT(cond, fmt, ...)
#define LLWFLOWS_DEBUG(...)
#endif

#if defined(LLWFLOWS_LOG)
#define LLWFLOWS_LOG_INFO(...)  LLWFLOWS_LOG_OUTPUT_FUNC("info", LLWFLOWS_NAMESPACE::debug::lightGrayColor, __VA_ARGS__);
#define LLWFLOWS_LOG_WARN(...)  LLWFLOWS_LOG_OUTPUT_FUNC("warn", LLWFLOWS_NAMESPACE::debug::yellowColor, __VA_ARGS__);
#define LLWFLOWS_LOG_ERROR(...) LLWFLOWS_LOG_OUTPUT_FUNC("error", LLWFLOWS_NAMESPACE::debug::redColor, __VA_ARGS__);
#define LLWFLOWS_LOG_FATAL(...) LLWFLOWS_LOG_OUTPUT_FUNC("fatal", LLWFLOWS_NAMESPACE::debug::redColor, __VA_ARGS__);
#else
#define LLWFLOWS_LOG_INFO(...)
#define LLWFLOWS_LOG_WARN(...)
#define LLWFLOWS_LOG_ERROR(...)
#define LLWFLOWS_LOG_FATAL(...)
#endif

#if LLWFLOWS_LOG_LEVEL > LLWFLOWS_LOG_LEVEL_DEBUG
#undef LLWFLOWS_DEBUG
#define LLWFLOWS_DEBUG(...)
#endif

#if LLWFLOWS_LOG_LEVEL > LLWFLOWS_LOG_LEVEL_INFO
#undef LLWFLOWS_INFO
#define LLWFLOWS_INFO(...)
#endif

#if LLWFLOWS_LOG_LEVEL > LLWFLOWS_LOG_LEVEL_WARN
#undef LLWFLOWS_WARN
#define LLWFLOWS_WARN(...)
#endif

#if LLWFLOWS_LOG_LEVEL > LLWFLOWS_LOG_LEVEL_ERROR
#undef LLWFLOWS_ERROR
#define LLWFLOWS_ERROR(...)
#endif

#if LLWFLOWS_LOG_LEVEL > LLWFLOWS_LOG_LEVEL_FATAL
#undef LLWFLOWS_FATAL
#define LLWFLOWS_FATAL(...)
#endif

#ifdef LLWFLOWS_LOG
#undef LLWFLOWS_LOG
#endif

#ifdef LLWFLOWS_DEBUG_ENABLE
#undef LLWFLOWS_DEBUG_ENABLE
#endif

#ifdef LLWFLOWS_LOG_CONTEXT
#undef LLWFLOWS_LOG_CONTEXT
#endif

#undef LLWFLOWS_LOG_LEVEL
#undef LLWFLOWS_LOG_LEVEL_DEBUG
#undef LLWFLOWS_LOG_LEVEL_INFO
#undef LLWFLOWS_LOG_LEVEL_WARN
#undef LLWFLOWS_LOG_LEVEL_ERROR
#undef LLWFLOWS_LOG_LEVEL_FATAL
#undef LLWFLOWS_LOG_LEVEL_NONE
