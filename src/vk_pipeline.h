#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

#include <glm/glm.hpp>

namespace ex::vulkan {
    struct push_data {
        glm::vec3 offset;
    };

    class pipeline {
    public:
        void create(VkShaderModule vertex_module,
                    VkShaderModule fragment_module,
                    VkPrimitiveTopology topology,
                    VkExtent2D extent,
                    VkPolygonMode polygon_mode,
                    VkDescriptorSetLayout *descriptor_set_layout,
                    VkRenderPass render_pass,
                    uint32_t subpass,
                    VkDevice logical_device,
                    VkAllocationCallbacks *allocator);
        void destroy(VkDevice logical_device,
                     VkAllocationCallbacks *allocator);
        
        void bind(VkCommandBuffer command_buffer,
                  VkPipelineBindPoint bind_point);
        void bind_descriptor(VkCommandBuffer command_buffer,
                             VkPipelineBindPoint bind_point,
                             VkDescriptorSet *descriptor_set);
        void update_dynamic(VkCommandBuffer command_buffer,
                            VkExtent2D extent);
        
        VkPipelineLayout layout();

    private:
        std::vector<VkPipelineShaderStageCreateInfo> create_shader_stages(VkShaderModule vertex_module,
                                                                          VkShaderModule fragment_module);
        VkPipelineVertexInputStateCreateInfo create_vertex_input_state(std::vector<VkVertexInputBindingDescription> &vertex_input_bindings,
                                                                       std::vector<VkVertexInputAttributeDescription> &vertex_input_attributes);
        VkPipelineInputAssemblyStateCreateInfo create_input_assembly_state(VkPrimitiveTopology topology);
        VkPipelineViewportStateCreateInfo create_viewport_state(VkExtent2D extent);
        VkPipelineRasterizationStateCreateInfo create_rasterization_state(VkPolygonMode polygon_mode);
        VkPipelineMultisampleStateCreateInfo create_multisample_state();
        VkPipelineDepthStencilStateCreateInfo create_depth_stencil_state();
        VkPipelineColorBlendAttachmentState create_color_blend_attachment_state();
        VkPipelineColorBlendStateCreateInfo create_color_blend_state(VkPipelineColorBlendAttachmentState *color_blend_attachment_state);
        VkPipelineDynamicStateCreateInfo create_dynamic_state(std::vector<VkDynamicState> &dynamic_states);

    private:
        VkPipeline m_handle;
        VkPipelineLayout m_layout;
        VkViewport m_viewport;
        VkRect2D m_scissor;        
    };
}
