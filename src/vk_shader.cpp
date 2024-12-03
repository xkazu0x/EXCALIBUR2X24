#include "vk_shader.h"
#include "vk_common.h"
#include "ex_logger.h"
#include <fstream>

ex::vulkan::shader::shader(const char *file) {
    m_code = read_file(file);
    EXINFO("shader size: %d", m_code.size());
}

ex::vulkan::shader::~shader() {
}

void
ex::vulkan::shader::create(VkDevice logical_device,
                           VkAllocationCallbacks *allocator) {
    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = static_cast<size_t>(m_code.size());
    shader_module_create_info.pCode = reinterpret_cast<const uint32_t *>(m_code.data());
    VK_CHECK(vkCreateShaderModule(logical_device,
                                  &shader_module_create_info,
                                  allocator,
                                  &m_module));
}

void
ex::vulkan::shader::destroy(VkDevice logical_device,
                            VkAllocationCallbacks *allocator) {
    if (m_module) {
        vkDestroyShaderModule(logical_device,
                              m_module,
                              allocator);
    }
}

VkShaderModule
ex::vulkan::shader::module() {
    return m_module;
}

std::vector<char>
ex::vulkan::shader::read_file(const char *file_name) {
    std::ifstream file(file_name, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        EXFATAL("[SHADER] Failed to open file");
        throw std::runtime_error("[SHADER] Failed to open file");
    }
    
    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);
    
    file.seekg(0, file.beg);
    file.read(buffer.data(), file_size);
    file.close();
    
    return buffer;
}
