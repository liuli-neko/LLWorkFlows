#pragma once

#include <time.h>

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
#else
#include <fmt/format.h>
#define LLWFLOWS_FORMAT fmt::format
#endif
#endif

#ifndef LLWFLOWS_LOG_OUTPUT
#define LLWFLOWS_LOG_OUTPUT stdout
#endif

#define LLWFLOWS_LOG_LEVEL_DEBUG 0
#define LLWFLOWS_LOG_LEVEL_INFO  1
#define LLWFLOWS_LOG_LEVEL_WARN  2
#define LLWFLOWS_LOG_LEVEL_ERROR 3
#define LLWFLOWS_LOG_LEVEL_FATAL 4
#define LLWFLOWS_LOG_LEVEL_NONE  5

#ifndef LLWFLOWS_LOG_LEVEL
#define LLWFLOWS_LOG_LEVEL LLWFLOWS_LOG_LEVEL_DEBUG
#endif

#ifdef LLWFLOWS_LOG_CONTEXT
#define LLWFLOWS_LOG_OUTPUT_FUNC(level, fmt, ...)                                                           \
    fprintf(LLWFLOWS_LOG_OUTPUT, "%s%s\n",                                                                  \
            LLWFLOWS_FORMAT(level " - [{}][{}:{}][{}]", clock(), __FILE__, __LINE__, __FUNCTION__).c_str(), \
            LLWFLOWS_FORMAT(fmt, ##__VA_ARGS__).c_str())
#else
#define LLWFLOWS_LOG_OUTPUT_FUNC(level, ...) \
    fprintf(LLWFLOWS_LOG_OUTPUT, "%s%s\n", level " - ", LLWFLOWS_FORMAT(__VA_ARGS__).c_str())
#endif

#if defined(LLWFLOWS_DEBUG_ENABLE)
#define LLWFLOWS_DEBUG(...) LLWFLOWS_LOG_OUTPUT_FUNC("debug", __VA_ARGS__);

#define LLWFLOWS_ASSERT(cond, ...)                        \
    if (!(cond)) {                                        \
        LLWFLOWS_LOG_OUTPUT_FUNC("%assert", __VA_ARGS__); \
        abort();                                          \
    }
#else
#define LLWFLOWS_ASSERT(cond, fmt, ...)
#define LLWFLOWS_DEBUG(...)
#endif

#if defined(LLWFLOWS_LOG)
#define LLWFLOWS_LOG_INFO(...)  LLWFLOWS_LOG_OUTPUT_FUNC("info", __VA_ARGS__);
#define LLWFLOWS_LOG_WARN(...)  LLWFLOWS_LOG_OUTPUT_FUNC("warn", __VA_ARGS__);
#define LLWFLOWS_LOG_ERROR(...) LLWFLOWS_LOG_OUTPUT_FUNC("error", __VA_ARGS__);
#define LLWFLOWS_LOG_FATAL(...) LLWFLOWS_LOG_OUTPUT_FUNC("fatal", __VA_ARGS__);
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
