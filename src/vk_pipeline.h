#pragma once

#include "vk_backend.h"
#include "vk_shader.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>

namespace ex::vulkan {
    class pipeline {
    public:
        void push_descriptor_set_layout(VkDescriptorSetLayout descriptor_set_layout);
        void set_push_constant_range(VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size);
        void build_layout(ex::vulkan::backend *backend);

        void set_topology(VkPrimitiveTopology topology);
        void set_polygon_mode(VkPolygonMode polygon_mode);
        void set_cull_mode(VkCullModeFlags cull_mode);
        void set_front_face(VkFrontFace front_face);
        void build(ex::vulkan::backend *backend, ex::vulkan::shader *shader);
        void destroy(ex::vulkan::backend *backend);
        
        void bind(VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point);
        void bind_descriptor_sets(VkCommandBuffer command_buffer, VkPipelineBindPoint bind_point, std::vector<VkDescriptorSet> descriptor_sets);
        void update_dynamic(VkCommandBuffer command_buffer, VkExtent2D extent);
        void push_constants(VkCommandBuffer command_buffer, VkShaderStageFlags stage_flags, const void *data);
        
    private:
        std::vector<VkPipelineShaderStageCreateInfo> create_shader_stages(VkShaderModule vertex_module, VkShaderModule fragment_module);
        VkPipelineVertexInputStateCreateInfo create_vertex_input_state(std::vector<VkVertexInputBindingDescription> &vertex_input_bindings, std::vector<VkVertexInputAttributeDescription> &vertex_input_attributes);
        VkPipelineInputAssemblyStateCreateInfo create_input_assembly_state(VkPrimitiveTopology topology);
        VkPipelineViewportStateCreateInfo create_viewport_state(VkExtent2D extent);
        VkPipelineRasterizationStateCreateInfo create_rasterization_state(VkPolygonMode polygon_mode, VkCullModeFlags cull_mode, VkFrontFace front_face);
        VkPipelineMultisampleStateCreateInfo create_multisample_state();
        VkPipelineDepthStencilStateCreateInfo create_depth_stencil_state();
        VkPipelineColorBlendAttachmentState create_color_blend_attachment_state();
        VkPipelineColorBlendStateCreateInfo create_color_blend_state(VkPipelineColorBlendAttachmentState *color_blend_attachment_state);
        VkPipelineDynamicStateCreateInfo create_dynamic_state(std::vector<VkDynamicState> &dynamic_states);

    private:        
        VkPipeline m_handle;
        VkPipelineLayout m_layout;
        VkPrimitiveTopology m_topology;
        VkPolygonMode m_polygon_mode;
        VkCullModeFlags m_cull_mode;
        VkFrontFace m_front_face;
        VkViewport m_viewport;
        VkRect2D m_scissor;
        std::vector<VkDescriptorSetLayout> m_descriptor_set_layouts;
        VkPushConstantRange m_push_constant_range;
    };
}
