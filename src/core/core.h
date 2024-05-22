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

#define HEXRAY_MACRO_TO_STR(x) #x

// Asserts
#if defined(HEXRAY_DEBUG)
#define HEXRAY_ASSERT_IMPL(condition, msg, ...) { if(!(condition)) { HEXRAY_CRITICAL(msg, __VA_ARGS__); __debugbreak(); } }
#define HEXRAY_ASSERT(condition) HEXRAY_ASSERT_IMPL(condition, "Assertion failed \"{}\"\n{},{}", HEXRAY_MACRO_TO_STR(condition), __FILE__, __LINE__)
#define HEXRAY_ASSERT_MSG(condition, msg, ...) HEXRAY_ASSERT_IMPL(condition, "Assertion failed \"{}\": {}\n{},{}", HEXRAY_MACRO_TO_STR(condition), fmt::format(msg, __VA_ARGS__).c_str(), __FILE__, __LINE__)
#else
#define HEXRAY_ASSERT(...)
#define HEXRAY_ASSERT_MSG(...)
#endif