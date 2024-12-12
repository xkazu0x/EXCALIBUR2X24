#pragma once

#include "vk_backend.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace ex::vulkan {
    class shader {
    public:
        void create(ex::vulkan::backend *backend, std::string vertex_path, std::string fragment_path);
        void destroy(ex::vulkan::backend *backend);

        VkShaderModule vertex_module() { return m_vertex_module; }
        VkShaderModule fragment_module() { return m_fragment_module; }
        
    private:
        std::vector<char> read_file(std::string file_path);
        
    private:
        VkShaderModule m_vertex_module;
        VkShaderModule m_fragment_module;
    };
}
