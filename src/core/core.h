#pragma once

#include "core/logger.h"

#include <vector>
#include <iostream>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <algorithm>

// Asserts
#if defined(HEXRAY_DEBUG)
#define HEXRAY_ASSERT(condition, msg, ...) { if(!(condition)) { HEXRAY_CRITICAL(msg, __VA_ARGS__); __debugbreak(); } }
#else
#define HEXRAY_ASSERT(...)
#endif