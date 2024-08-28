/**
 * @file enum.hpp
 * @author llhsdmd (llhsdmd@gmail.com)
 * @brief
 * @version 0.1
 * @date 2024-06-18
 *
 * @copyright Copyright (c) 2024 llhsdmd BusyStudent
 *
 */
#pragma once

#include <cstring>
#include <string>

#ifdef __GNUC__
#include <cxxabi.h>

#include <list>
#include <map>
#include <vector>
#endif

#include "workflowsglobal.hpp"

LLWFLOWS_NS_BEGIN
namespace detail {
#if __cplusplus >= 201703L || _MSVC_LANG > 201402L
/// ====================== enum string =========================
#ifndef NEKO_ENUM_SEARCH_DEPTH
#define NEKO_ENUM_SEARCH_DEPTH 60
#endif

#ifdef __GNUC__
#define NEKO_STRINGIFY_TYPE_RAW(x) NEKO_STRINGIFY_TYPEINFO(typeid(x))
#define NEKO_FUNCTION              __PRETTY_FUNCTION__

namespace {
template <typename T, T Value>
constexpr auto _Neko_GetEnumName() noexcept {
    // constexpr auto _Neko_GetEnumName() [with T = MyEnum; T Value = MyValues]
    // constexpr auto _Neko_GetEnumName() [with T = MyEnum; T Value =
    // (MyEnum)114514]"
    std::string_view name(__PRETTY_FUNCTION__);
    size_t           eqBegin = name.find_last_of(' ');
    size_t           nBegin  = name.find_last_of(':');
    size_t           end     = name.find_last_of(']');
    eqBegin                  = nBegin == std::string_view::npos ? eqBegin : nBegin;
    std::string_view body    = name.substr(eqBegin + 1, end - eqBegin - 1);
    if (body[0] == '(') {
        // Failed
        return std::string_view();
    }
    return body;
}
inline std::string NEKO_STRINGIFY_TYPEINFO(const std::type_info& info) {
    int  status;
    auto str = ::abi::__cxa_demangle(info.name(), nullptr, nullptr, &status);
    if (str) {
        std::string ret(str);
        ::free(str);
        return ret;
    }
    return info.name();
}
#elif defined(_MSC_VER)
#define NEKO_STRINGIFY_TYPE_RAW(type) NEKO_STRINGIFY_TYPEINFO(typeid(type))
#define NEKO_ENUM_TO_NAME(enumType)
#define NEKO_FUNCTION __FUNCTION__
namespace {
inline const char* NEKO_STRINGIFY_TYPEINFO(const std::type_info& info) {
    // Skip struct class prefix
    auto name = info.name();
    if (::strncmp(name, "class ", 6) == 0) {
        return name + 6;
    }
    if (::strncmp(name, "struct ", 7) == 0) {
        return name + 7;
    }
    if (::strncmp(name, "enum ", 5) == 0) {
        return name + 5;
    }
    return name;
}
template <typename T, T Value>
constexpr auto _Neko_GetEnumName() noexcept {
    // auto __cdecl _Neko_GetEnumName<enum main::MyEnum,(enum
    // main::MyEnum)0x2>(void) auto __cdecl _Neko_GetEnumName<enum
    // main::MyEnum,main::MyEnum::Wtf>(void)
    std::string_view name(__FUNCSIG__);
    size_t           dotBegin = name.find_first_of(',');
    size_t           end      = name.find_last_of('>');
    std::string_view body     = name.substr(dotBegin + 1, end - dotBegin - 1);
    if (body[0] == '(') {
        // Failed
        return std::string_view();
    }
    return body;
}
#else
namespace {
#define NEKO_STRINGIFY_TYPE_RAW(type) typeid(type).name()
template <typename T, T Value>
constexpr auto _Neko_GetEnumName() noexcept {
    // Unsupported
    return std::string_view();
}
#endif
template <typename T, T Value>
constexpr bool _Neko_IsValidEnum() noexcept {
    return !_Neko_GetEnumName<T, Value>().empty();
}
template <typename T, size_t... N>
constexpr size_t _Neko_GetValidEnumCount(std::index_sequence<N...> seq) noexcept {
    return (... + _Neko_IsValidEnum<T, T(N)>());
}
template <typename T, size_t... N>
constexpr auto _Neko_GetValidEnumNames(std::index_sequence<N...> seq) noexcept {
    constexpr auto                                         validCount = _Neko_GetValidEnumCount<T>(seq);

    std::array<std::pair<T, std::string_view>, validCount> arr;
    std::string_view                                       vstr[sizeof...(N)]{_Neko_GetEnumName<T, T(N)>()...};

    size_t                                                 n    = 0;
    size_t                                                 left = validCount;
    auto                                                   iter = arr.begin();

    for (auto i : vstr) {
        if (!i.empty()) {
            // Valid name
            iter->first  = T(n);
            iter->second = i;
            ++iter;
        }
        if (left == 0) {
            break;
        }

        n += 1;
    }

    return arr;
}
}  // namespace
/// ====================== end enum string =====================
template <typename T>
std::string_view enumToString(T value) noexcept {
    constexpr static auto kEnumArr = _Neko_GetValidEnumNames<T>(std::make_index_sequence<NEKO_ENUM_SEARCH_DEPTH>());
    for (int i = 0; i < kEnumArr.size(); ++i) {
        if (kEnumArr[i].first == value) {
            return kEnumArr[i].second;
        }
    }
    return std::string_view{};
}
#else
template <typename T>
std::string enumToString(T value) noexcept {
    return {};
}
#endif
}  // namespace detail
LLWFLOWS_NS_END