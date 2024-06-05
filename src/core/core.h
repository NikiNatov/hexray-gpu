#pragma once

#include "core/logger.h"

#include <vector>
#include <iostream>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <string>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>

// Utils
#define HEXRAY_MACRO_TO_STR(x) #x
#define HEXRAY_BIND_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

#define Align(size, alignment) ((size + (alignment - 1)) & ~(alignment - 1))

#define ENUM_FLAGS(EnumType) \
static EnumType operator&(EnumType a, EnumType b) \
{ \
    return (EnumType)((uint32_t)a & (uint32_t)b); \
} \
static EnumType operator|(EnumType a, EnumType b) \
{ \
    return (EnumType)((uint32_t)a | (uint32_t)b); \
} \
static EnumType operator~(EnumType a) \
{ \
    return (EnumType)(~(uint32_t)a); \
} \
static EnumType& operator|=(EnumType& a, EnumType b) \
{ \
    a = (EnumType)((uint32_t)a | (uint32_t)b); \
    return a; \
} \
static EnumType& operator&=(EnumType& a, EnumType b) \
{ \
    a = (EnumType)((uint32_t)a & (uint32_t)b); \
    return a; \
} \
static bool IsSet(EnumType flags, EnumType value) \
{ \
    return ((uint32_t)flags & (uint32_t)value) == (uint32_t)value; \
} \

// Asserts
#if defined(HEXRAY_DEBUG)
#define HEXRAY_ASSERT_IMPL(condition, msg, ...) { if(!(condition)) { HEXRAY_CRITICAL(msg, __VA_ARGS__); __debugbreak(); } }
#define HEXRAY_ASSERT(condition) HEXRAY_ASSERT_IMPL(condition, "Assertion failed \"{}\"\n{},{}", HEXRAY_MACRO_TO_STR(condition), __FILE__, __LINE__)
#define HEXRAY_ASSERT_MSG(condition, msg, ...) HEXRAY_ASSERT_IMPL(condition, "Assertion failed \"{}\": {}\n{},{}", HEXRAY_MACRO_TO_STR(condition), fmt::format(msg, __VA_ARGS__).c_str(), __FILE__, __LINE__)
#else
#define HEXRAY_ASSERT(...)
#define HEXRAY_ASSERT_MSG(...)
#endif

#define UNREACHED HEXRAY_ASSERT(!"This shouldn't be reached")
#define NOT_IMPLEMENTED HEXRAY_ASSERT(!"This is not implemented")
#define NOT_IMPLEMENTED_BECAUSE(...) HEXRAY_ASSERT(!"This is not implemented" && __VA_ARGS__)

#define FRAMES_IN_FLIGHT 3