#pragma once

#include "workflowsglobal.hpp"

#if !defined(NDEBUG) && !defined(LLWFLOWS_NDEBUG)
#define LLWFLOWS_DEBUG
#endif

#if !defined(LLWFLOWS_NLOG)
#define LLWFLOWS_LOG
#endif

#if LLWFLOWS_CPP_PLUS >= 20
#include <version>
#if defined(__cpp_lib_format)
#include <format>
#define LLWFLOWS_STD_FORMAT
#endif
#endif

#if defined(LLWFLOWS_STD_FORMAT)
#define FORMAT std::format
#else
#include <fmt/format.h>
#define FORMAT fmt::format
#endif

#if defined(LLWFLOWS_DEBUG) && defined(LLWFLOWS_LOG_CONTEXT)
#define LLWFLOWS_DEBUG(FMT, ...)                                                                                      \
    fprintf(stdout, "%s\n",                                                                                           \
            FORMAT("debug - [{}][{}:{}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__).c_str()); \
    fflush(stdout);
#define LLWFLOWS_ASSERT(cond, FMT, ...)                                                                               \
    if (!(cond)) {                                                                                                    \
        fprintf(                                                                                                      \
            stdout, "%s\n",                                                                                           \
            FORMAT("error - [{}][{}:{}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__).c_str()); \
        abort();                                                                                                      \
    }
#elif defined(LLWFLOWS_DEBUG)
#define LLWFLOWS_DEBUG(...)                                          \
    fprintf(stdout, "%s\n", FORMAT("debug - " __VA_ARGS__).c_str()); \
    fflush(stdout);
#define LLWFLOWS_ASSERT(cond, ...)                                       \
    if (!(cond)) {                                                       \
        fprintf(stdout, "%s\n", FORMAT("error - " __VA_ARGS__).c_str()); \
        abort();                                                         \
    }
#else
#define LLWFLOWS_ASSERT(cond, fmt, ...)
#define LLWFLOWS_DEBUG(...)
#endif

#if defined(LLWFLOWS_LOG) && defined(LLWFLOWS_LOG_CONTEXT)
#define LLWFLOWS_LOG_INFO(FMT, ...)                                                                                  \
    fprintf(stdout, "%s\n",                                                                                          \
            FORMAT("info - [{}][{}:{}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__).c_str()); \
    fflush(stdout);
#define LLWFLOWS_LOG_WARN(FMT, ...)                                                                                  \
    fprintf(stdout, "%s\n",                                                                                          \
            FORMAT("warn - [{}][{}:{}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__).c_str()); \
    fflush(stdout);
#define LLWFLOWS_LOG_ERROR(FMT, ...)                                                                                  \
    fprintf(stdout, "%s\n",                                                                                           \
            FORMAT("error - [{}][{}:{}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__).c_str()); \
    fflush(stdout);
#define LLWFLOWS_LOG_FATAL(FMT, ...)                                                                                  \
    fprintf(stdout, "%s\n",                                                                                           \
            FORMAT("fatal - [{}][{}:{}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__).c_str()); \
    fflush(stdout);
#elif defined(LLWFLOWS_LOG)
#define LLWFLOWS_LOG_INFO(...)                                      \
    fprintf(stdout, "%s\n", FORMAT("info - " __VA_ARGS__).c_str()); \
    fflush(stdout);
#define LLWFLOWS_LOG_WARN(...)                                      \
    fprintf(stdout, "%s\n", FORMAT("warn - " __VA_ARGS__).c_str()); \
    fflush(stdout);
#define LLWFLOWS_LOG_ERROR(...)                                      \
    fprintf(stdout, "%s\n", FORMAT("error - " __VA_ARGS__).c_str()); \
    fflush(stdout);
#define LLWFLOWS_LOG_FATAL(...)                                      \
    fprintf(stdout, "%s\n", FORMAT("fatal - " __VA_ARGS__).c_str()); \
    fflush(stdout);
#else
#define LLWFLOWS_LOG_INFO(...)
#define LLWFLOWS_LOG_WARN(...)
#define LLWFLOWS_LOG_ERROR(...)
#define LLWFLOWS_LOG_FATAL(...)
#endif