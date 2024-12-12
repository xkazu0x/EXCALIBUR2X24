#include "vk_shader.h"
#include "vk_common.h"
#include "ex_logger.h"
#include <fstream>

void
ex::vulkan::shader::create(ex::vulkan::backend *backend, std::string vertex_path, std::string fragment_path) {
    std::vector<char> vertex_code = read_file(vertex_path);
    std::vector<char> fragment_code = read_file(fragment_path);

    VkShaderModuleCreateInfo vertex_module_create_info = {};
    vertex_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertex_module_create_info.codeSize = static_cast<size_t>(vertex_code.size());
    vertex_module_create_info.pCode = reinterpret_cast<const uint32_t *>(vertex_code.data());
    VK_CHECK(vkCreateShaderModule(backend->logical_device(),
                                  &vertex_module_create_info,
                                  backend->allocator(),
                                  &m_vertex_module));
    
    VkShaderModuleCreateInfo fragment_module_create_info = {};
    fragment_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragment_module_create_info.codeSize = static_cast<size_t>(fragment_code.size());
    fragment_module_create_info.pCode = reinterpret_cast<const uint32_t *>(fragment_code.data());
    VK_CHECK(vkCreateShaderModule(backend->logical_device(),
                                  &fragment_module_create_info,
                                  backend->allocator(),
                                  &m_fragment_module));
}

void
ex::vulkan::shader::destroy(ex::vulkan::backend *backend) {
    if (m_vertex_module) {
        vkDestroyShaderModule(backend->logical_device(),
                              m_vertex_module,
                              backend->allocator());
    }

    if (m_fragment_module) {
        vkDestroyShaderModule(backend->logical_device(),
                              m_fragment_module,
                              backend->allocator());
    }
}

std::vector<char>
ex::vulkan::shader::read_file(std::string file_path) {
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);
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
