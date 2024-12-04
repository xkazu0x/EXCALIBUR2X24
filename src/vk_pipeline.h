#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

namespace ex::vulkan {
    class graphics_pipeline {
    public:
        void create(VkDevice logical_device,
                    VkAllocationCallbacks *allocator,
                    VkRenderPass render_pass,
                    uint32_t subpass);
        void destroy(VkDevice logical_device,
                     VkAllocationCallbacks *allocator);

        void create_shader_stages(VkShaderModule vertex_module,
                                  VkShaderModule fragment_module);
        void create_vertex_input_state(std::vector<VkVertexInputBindingDescription> &vertex_input_bindings,
                                       std::vector<VkVertexInputAttributeDescription> &vertex_input_attributes);
        void create_input_assembly_state(VkPrimitiveTopology topology);
        void create_viewport_state(VkExtent2D extent);
        void create_rasterization_state(VkPolygonMode polygon_mode,
                                        VkCullModeFlags cull_mode,
                                        VkFrontFace front_face);
        void create_multisample_state();
        void create_depth_stencil_state();
        void create_color_blend_attachment_state();
        void create_color_blend_state();
        void create_dynamic_state(std::vector<VkDynamicState> &dynamic_states);
        void create_layout(VkDevice logical_device,
                           VkAllocationCallbacks *allocator,
                           VkDescriptorSetLayout *descriptor_set_layout);

        void bind(VkCommandBuffer &command_buffer);
        
        void update_viewport(VkCommandBuffer &command_buffer,
                             VkExtent2D extent);
        void update_scissor(VkCommandBuffer &command_buffer,
                            VkExtent2D extent);
        
        VkPipelineLayout layout();

    private:
        VkPipeline m_handle;
        VkPipelineLayout m_layout;
        VkViewport m_viewport;
        VkRect2D m_scissor;
        
        std::array<VkPipelineShaderStageCreateInfo, 2> m_shader_stages;
        VkPipelineVertexInputStateCreateInfo m_vertex_input_state_create_info;
        VkPipelineInputAssemblyStateCreateInfo m_input_assembly_state_create_info;
        VkPipelineViewportStateCreateInfo m_viewport_state_create_info;
        VkPipelineRasterizationStateCreateInfo m_rasterization_state_create_info;
        VkPipelineMultisampleStateCreateInfo m_multisample_state_create_info;
        VkPipelineDepthStencilStateCreateInfo m_depth_stencil_state_create_info;
        VkPipelineColorBlendAttachmentState m_color_blend_attachment_state;
        VkPipelineColorBlendStateCreateInfo m_color_blend_state_create_info;
        VkPipelineDynamicStateCreateInfo m_dynamic_state_create_info;
    };
}
