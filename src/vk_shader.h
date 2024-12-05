#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace ex::vulkan {
    class shader {
    public:
        void load(const char *file);
        void create(VkDevice logical_device,
                    VkAllocationCallbacks *allocator);
        void destroy(VkDevice logical_device,
                     VkAllocationCallbacks *allocator);

        VkShaderModule module();
        uint32_t size();
        
    private:
        std::vector<char> read_file(const char *file_name);
        
    private:
        VkShaderModule m_module;
        std::vector<char> m_code;
    };
}
