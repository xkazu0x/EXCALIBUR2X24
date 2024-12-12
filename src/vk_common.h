#pragma once

#include "ex_logger.h"
#include <stdexcept>

#define VK_CHECK(x) if ((x) != VK_SUCCESS) { EXFATAL("Vulkan Error: %d", x); throw std::runtime_error("\"VK_CHECK\" FAILED"); }
#define VK_ARRAY_SIZE(array) sizeof(array[0]) / sizeof(array)
