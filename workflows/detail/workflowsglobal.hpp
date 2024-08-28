#pragma once

#define LLWFLOWS_NS_BEGIN  namespace llflows {
#define LLWFLOWS_NS_END    }
#define LLWFLOWS_NAMESPACE ::llflows
#define LLWFLOWS_NS_USING  using namespace llflows;

#if __cplusplus >= 202002L || _MSVC_LANG >= 202002L
#define LLWFLOWS_CPP_PLUS 20
#elif __cplusplus >= 201703L || _MSVC_LANG >= 201703L
#define LLWFLOWS_CPP_PLUS 17
#elif __cplusplus >= 201402L || _MSVC_LANG >= 201402L
#define LLWFLOWS_CPP_PLUS 14
#elif __cplusplus >= 201103L || _MSVC_LANG >= 201103L
#define LLWFLOWS_CPP_PLUS 11
#endif

static_assert(LLWFLOWS_CPP_PLUS >= 17, "LLWFLOWS requires C++17 or later");

#ifdef _WIN32
#define LLWFLOWS_DECL_EXPORT __declspec(dllexport)
#define LLWFLOWS_DECL_IMPORT __declspec(dllimport)
#define LLWFLOWS_DECL_LOCAL
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#define LLWFLOWS_DECL_EXPORT __attribute__((visibility("default")))
#define LLWFLOWS_DECL_IMPORT __attribute__((visibility("default")))
#define LLWFLOWS_DECL_LOCAL  __attribute__((visibility("hidden")))
#else
#define LLWFLOWS_DECL_EXPORT
#define LLWFLOWS_DECL_IMPORT
#define LLWFLOWS_DECL_LOCAL
#endif

#ifndef LLWFLOWS_STATIC
#ifdef LLWFLOWS_LIBRARY
#define LLWFLOWS_API LLWFLOWS_DECL_EXPORT
#else
#define LLWFLOWS_API LLWFLOWS_DECL_IMPORT
#endif
#else
#define LLWFLOWS_API
#endif