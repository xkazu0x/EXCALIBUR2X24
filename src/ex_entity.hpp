#pragma once

#include "ex_component.hpp"
#include "vk_model.h"

namespace ex {
    struct entity {
        ex::vulkan::model *model;
        ex::comp::transform transform;
    };
}
