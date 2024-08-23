#pragma once

#if !defined(NDEBUG) && !defined(LLWFLOWS_NDEBUG)
#define LLWFLOWS_DEBUG
#endif

#if !defined(LLWFLOWS_NLOG)
#define LLWFLOWS_LOG
#endif

#if defined(LLWFLOWS_DEBUG) && defined(LLWFLOWS_LOG_CONTEXT)
#include <fmt/format.h>
#define LLWFLOWS_DEBUG(FMT, ...)                                                                                   \
    fprintf(stdout, "%s\n",                                                                                        \
            fmt::format("debug - [{}][{}:{}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__) \
                .c_str())
#define LLWFLOWS_ASSERT(cond, FMT, ...)                                                                                \
    if (!(cond)) {                                                                                                     \
        fprintf(stdout, "%s\n",                                                                                        \
                fmt::format("error - [{}][{}:{}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__) \
                    .c_str());                                                                                         \
        abort();                                                                                                       \
    }
#elif defined(LLWFLOWS_DEBUG)
#include <fmt/format.h>
#define LLWFLOWS_DEBUG(...) fprintf(stdout, "%s\n", fmt::format("debug - " __VA_ARGS__).c_str())
#define LLWFLOWS_ASSERT(cond, ...)                                            \
    if (!(cond)) {                                                            \
        fprintf(stdout, "%s\n", fmt::format("error - " __VA_ARGS__).c_str()); \
        abort();                                                              \
    }
#else
#define LLWFLOWS_ASSERT(cond, fmt, ...)
#define LLWFLOWS_DEBUG(...)
#endif

#if defined(LLWFLOWS_LOG) && defined(LLWFLOWS_LOG_CONTEXT)
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/spdlog.h>
#define LLWFLOWS_LOG_INFO(FMT, ...)                                                                                \
    fprintf(stdout, "%s\n",                                                                                        \
            fmt::format("info - [{}][{}:{}}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__) \
                .c_str())
#define LLWFLOWS_LOG_WARN(FMT, ...)                                                                                \
    fprintf(stdout, "%s\n",                                                                                        \
            fmt::format("warn - [{}][{}:{}}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__) \
                .c_str())
#define LLWFLOWS_LOG_ERROR(FMT, ...)                                                                                \
    fprintf(stdout, "%s\n",                                                                                         \
            fmt::format("error - [{}][{}:{}}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__) \
                .c_str())
#define LLWFLOWS_LOG_FATAL(FMT, ...)                                                                                \
    fprintf(stdout, "%s\n",                                                                                         \
            fmt::format("fatal - [{}][{}:{}}][{}] " FMT, __TIME__, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__) \
                .c_str())
#elif defined(LLWFLOWS_LOG)
#include <fmt/format.h>
#define LLWFLOWS_LOG_INFO(...)  fprintf(stdout, "%s\n", fmt::format("info - " __VA_ARGS__).c_str())
#define LLWFLOWS_LOG_WARN(...)  fprintf(stdout, "%s\n", fmt::format("warn - " __VA_ARGS__).c_str())
#define LLWFLOWS_LOG_ERROR(...) fprintf(stdout, "%s\n", fmt::format("error - " __VA_ARGS__).c_str())
#define LLWFLOWS_LOG_FATAL(...) fprintf(stdout, "%s\n", fmt::format("fatal - " __VA_ARGS__).c_str())
#else
#define LLWFLOWS_LOG_INFO(...)
#define LLWFLOWS_LOG_WARN(...)
#define LLWFLOWS_LOG_ERROR(...)
#define LLWFLOWS_LOG_FATAL(...)
#endif