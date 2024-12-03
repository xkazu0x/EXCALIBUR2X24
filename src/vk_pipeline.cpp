#include "vk_pipeline.h"
#include "vk_common.h"
#include <vector>

void
ex::vulkan::graphics_pipeline::create(VkDevice logical_device,
                                      VkAllocationCallbacks *allocator,
                                      VkRenderPass render_pass,
                                      uint32_t subpass) {    
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = nullptr;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = m_shader_stages.size();
    graphics_pipeline_create_info.pStages = m_shader_stages.data();
    graphics_pipeline_create_info.pVertexInputState = &m_vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &m_input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState = nullptr;
    graphics_pipeline_create_info.pViewportState = &m_viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &m_rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &m_multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &m_depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &m_color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &m_dynamic_state_create_info;
    graphics_pipeline_create_info.layout = m_layout;
    graphics_pipeline_create_info.renderPass = render_pass;
    graphics_pipeline_create_info.subpass = subpass;    
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = 0;
    VK_CHECK(vkCreateGraphicsPipelines(logical_device,
                                       nullptr,
                                       1,
                                       &graphics_pipeline_create_info,
                                       allocator,
                                       &m_handle));
}

void
ex::vulkan::graphics_pipeline::destroy(VkDevice logical_device,
                                       VkAllocationCallbacks *allocator) {
    if (m_handle) {
        vkDestroyPipeline(logical_device,
                          m_handle,
                          allocator);
    }

    if (m_layout) {
        vkDestroyPipelineLayout(logical_device,
                                m_layout,
                                allocator);
    }
}

void
ex::vulkan::graphics_pipeline::create_shader_stages(VkShaderModule vertex_module,
                                                    VkShaderModule fragment_module) {
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {};
    shader_stages[0].pNext = nullptr;
    shader_stages[0].flags = 0;
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_module;
    shader_stages[0].pName = "main";
    
    shader_stages[1].pNext = nullptr;
    shader_stages[1].flags = 0;
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_module;
    shader_stages[1].pName = "main";

    m_shader_stages = shader_stages;
}

void
ex::vulkan::graphics_pipeline::create_vertex_input_state(VkVertexInputBindingDescription *vertex_input_binding,
                                                         std::vector<VkVertexInputAttributeDescription> &vertex_input_attributes) {
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = vertex_input_binding;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attributes.data();

    m_vertex_input_state_create_info = vertex_input_state_create_info;
}

void
ex::vulkan::graphics_pipeline::create_input_assembly_state(VkPrimitiveTopology topology) {
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.pNext = nullptr;
    input_assembly_state_create_info.flags = 0;
    input_assembly_state_create_info.topology = topology;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    m_input_assembly_state_create_info = input_assembly_state_create_info;
}

void
ex::vulkan::graphics_pipeline::create_viewport_state(VkExtent2D extent) {
    // VIEWPORT    
    m_viewport.x = 0.0f;
    m_viewport.y = 0.0f;
    m_viewport.width = static_cast<float>(extent.width);
    m_viewport.height = static_cast<float>(extent.height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;

    // SCISSOR
    m_scissor.offset = { 0, 0 };
    m_scissor.extent = extent;

    // VIEWPORT STATE CREATE INFO
    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.pNext = nullptr;
    viewport_state_create_info.flags = 0;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &m_viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &m_scissor;

    m_viewport_state_create_info = viewport_state_create_info;
}

void
ex::vulkan::graphics_pipeline::create_rasterization_state(VkPolygonMode polygon_mode,
                                                          VkCullModeFlags cull_mode,
                                                          VkFrontFace front_face) {
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.pNext = nullptr;
    rasterization_state_create_info.flags = 0;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = polygon_mode;
    rasterization_state_create_info.cullMode = cull_mode;
    rasterization_state_create_info.frontFace = front_face;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    rasterization_state_create_info.lineWidth = 1.0f;

    m_rasterization_state_create_info = rasterization_state_create_info;
}

void
ex::vulkan::graphics_pipeline::create_multisample_state() {
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.pNext = nullptr;
    multisample_state_create_info.flags = 0;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = nullptr;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    m_multisample_state_create_info = multisample_state_create_info;
}

void
ex::vulkan::graphics_pipeline::create_depth_stencil_state() {
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.pNext = nullptr;
    depth_stencil_state_create_info.flags = 0;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = {};
    depth_stencil_state_create_info.back = {};
    depth_stencil_state_create_info.minDepthBounds = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds = 1.0f;

    m_depth_stencil_state_create_info = depth_stencil_state_create_info;
}

void
ex::vulkan::graphics_pipeline::create_color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
    color_blend_attachment_state.blendEnable = VK_FALSE;
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    m_color_blend_attachment_state = color_blend_attachment_state;
}

void
ex::vulkan::graphics_pipeline::create_color_blend_state() {
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.pNext = nullptr;
    color_blend_state_create_info.flags = 0;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &m_color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;

    m_color_blend_state_create_info = color_blend_state_create_info;
}

void
ex::vulkan::graphics_pipeline::create_dynamic_state(std::vector<VkDynamicState> &dynamic_states) {
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.pNext = nullptr;
    dynamic_state_create_info.flags = 0;
    dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_create_info.pDynamicStates = dynamic_states.data();

    m_dynamic_state_create_info = dynamic_state_create_info;
}

void
ex::vulkan::graphics_pipeline::create_layout(VkDevice logical_device,
                                             VkAllocationCallbacks *allocator,
                                             VkDescriptorSetLayout *descriptor_set_layout) {
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.pNext = nullptr;
    pipeline_layout_create_info.flags = 0;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;
    VK_CHECK(vkCreatePipelineLayout(logical_device,
                                    &pipeline_layout_create_info,
                                    allocator,
                                    &m_layout));
}

void
ex::vulkan::graphics_pipeline::bind(VkCommandBuffer &command_buffer) {
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_handle);
}

void
ex::vulkan::graphics_pipeline::update_viewport(VkCommandBuffer &command_buffer,
                                               VkExtent2D extent) {
    m_viewport.x = 0.0f;
    m_viewport.y = 0.0f;
    m_viewport.width = static_cast<float>(extent.width);
    m_viewport.height = static_cast<float>(extent.height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &m_viewport);
}

void
ex::vulkan::graphics_pipeline::update_scissor(VkCommandBuffer &command_buffer,
                                              VkExtent2D extent) {
    m_scissor.offset = { 0, 0 };
    m_scissor.extent = extent;
    vkCmdSetScissor(command_buffer, 0, 1, &m_scissor);
}

VkPipelineLayout
ex::vulkan::graphics_pipeline::layout() {
    return m_layout;
}
