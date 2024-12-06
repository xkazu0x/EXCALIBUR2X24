#include "vk_pipeline.h"
#include "vk_common.h"
#include "ex_vertex.h"
#include <vector>

void
ex::vulkan::pipeline::create(VkShaderModule vertex_module,
                             VkShaderModule fragment_module,
                             VkPrimitiveTopology topology,
                             VkExtent2D extent,
                             VkPolygonMode polygon_mode,
                             VkDescriptorSetLayout *descriptor_set_layout,
                             VkRenderPass render_pass,
                             uint32_t subpass,
                             VkDevice logical_device,
                             VkAllocationCallbacks *allocator) {
    std::vector<VkPipelineShaderStageCreateInfo> shader_stages = create_shader_stages(vertex_module, fragment_module);

    auto vertex_binding_descriptions = ex::vertex::get_binding_descriptions();
    auto vertex_attribute_descriptions = ex::vertex::get_attribute_descriptions();
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = create_vertex_input_state(vertex_binding_descriptions, vertex_attribute_descriptions);
    
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = create_input_assembly_state(topology);    
    VkPipelineViewportStateCreateInfo viewport_state_create_info = create_viewport_state(extent);    
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = create_rasterization_state(polygon_mode);    
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = create_multisample_state();    
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = create_depth_stencil_state();
    VkPipelineColorBlendAttachmentState color_blend_attachment_state = create_color_blend_attachment_state();    
    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = create_color_blend_state(&color_blend_attachment_state);

    std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, };    
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = create_dynamic_state(dynamic_states);

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
    
    VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
    graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphics_pipeline_create_info.pNext = nullptr;
    graphics_pipeline_create_info.flags = 0;
    graphics_pipeline_create_info.stageCount = shader_stages.size();
    graphics_pipeline_create_info.pStages = shader_stages.data();
    graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    graphics_pipeline_create_info.pTessellationState = nullptr;
    graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
    graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
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
ex::vulkan::pipeline::destroy(VkDevice logical_device,
                              VkAllocationCallbacks *allocator) {
    if (m_handle) vkDestroyPipeline(logical_device, m_handle, allocator);
    if (m_layout) vkDestroyPipelineLayout(logical_device, m_layout, allocator);
}

void
ex::vulkan::pipeline::bind(VkCommandBuffer &command_buffer) {
    vkCmdBindPipeline(command_buffer,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_handle);
}

void
ex::vulkan::pipeline::update_dynamic(VkCommandBuffer &command_buffer,
                                     VkExtent2D extent) {
    m_viewport.x = 0.0f;
    m_viewport.y = 0.0f;
    m_viewport.width = static_cast<float>(extent.width);
    m_viewport.height = static_cast<float>(extent.height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &m_viewport);

    m_scissor.offset = { 0, 0 };
    m_scissor.extent = extent;
    vkCmdSetScissor(command_buffer, 0, 1, &m_scissor);
}

VkPipelineLayout
ex::vulkan::pipeline::layout() {
    return m_layout;
}

std::vector<VkPipelineShaderStageCreateInfo>
ex::vulkan::pipeline::create_shader_stages(VkShaderModule vertex_module,
                                           VkShaderModule fragment_module) {
    std::vector<VkPipelineShaderStageCreateInfo> out_shader_stages(2);
    out_shader_stages[0].pNext = nullptr;
    out_shader_stages[0].flags = 0;
    out_shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    out_shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    out_shader_stages[0].module = vertex_module;
    out_shader_stages[0].pName = "main";
    
    out_shader_stages[1].pNext = nullptr;
    out_shader_stages[1].flags = 0;
    out_shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    out_shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    out_shader_stages[1].module = fragment_module;
    out_shader_stages[1].pName = "main";

    return out_shader_stages;
}

VkPipelineVertexInputStateCreateInfo
ex::vulkan::pipeline::create_vertex_input_state(std::vector<VkVertexInputBindingDescription> &vertex_input_bindings,
                                                std::vector<VkVertexInputAttributeDescription> &vertex_input_attributes) {
    VkPipelineVertexInputStateCreateInfo out_vertex_input_state_create_info = {};
    out_vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    out_vertex_input_state_create_info.vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_input_bindings.size());
    out_vertex_input_state_create_info.pVertexBindingDescriptions = vertex_input_bindings.data();
    out_vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attributes.size());
    out_vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attributes.data();

    return out_vertex_input_state_create_info;
}

VkPipelineInputAssemblyStateCreateInfo
ex::vulkan::pipeline::create_input_assembly_state(VkPrimitiveTopology topology) {
    VkPipelineInputAssemblyStateCreateInfo out_input_assembly_state_create_info = {};
    out_input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    out_input_assembly_state_create_info.pNext = nullptr;
    out_input_assembly_state_create_info.flags = 0;
    out_input_assembly_state_create_info.topology = topology;
    out_input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

    return out_input_assembly_state_create_info;
}

VkPipelineViewportStateCreateInfo
ex::vulkan::pipeline::create_viewport_state(VkExtent2D extent) {
    m_viewport.x = 0.0f;
    m_viewport.y = 0.0f;
    m_viewport.width = static_cast<float>(extent.width);
    m_viewport.height = static_cast<float>(extent.height);
    m_viewport.minDepth = 0.0f;
    m_viewport.maxDepth = 1.0f;

    m_scissor.offset = { 0, 0 };
    m_scissor.extent = extent;

    VkPipelineViewportStateCreateInfo out_viewport_state_create_info = {};
    out_viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    out_viewport_state_create_info.pNext = nullptr;
    out_viewport_state_create_info.flags = 0;
    out_viewport_state_create_info.viewportCount = 1;
    out_viewport_state_create_info.pViewports = &m_viewport;
    out_viewport_state_create_info.scissorCount = 1;
    out_viewport_state_create_info.pScissors = &m_scissor;

    return out_viewport_state_create_info;
}

VkPipelineRasterizationStateCreateInfo
ex::vulkan::pipeline::create_rasterization_state(VkPolygonMode polygon_mode) {
    VkPipelineRasterizationStateCreateInfo out_rasterization_state_create_info = {};
    out_rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    out_rasterization_state_create_info.pNext = nullptr;
    out_rasterization_state_create_info.flags = 0;
    out_rasterization_state_create_info.depthClampEnable = VK_FALSE;
    out_rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    out_rasterization_state_create_info.polygonMode = polygon_mode;
    out_rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
    out_rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    out_rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    out_rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    out_rasterization_state_create_info.depthBiasClamp = 0.0f;
    out_rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    out_rasterization_state_create_info.lineWidth = 1.0f;

    return out_rasterization_state_create_info;
}

VkPipelineMultisampleStateCreateInfo
ex::vulkan::pipeline::create_multisample_state() {
    VkPipelineMultisampleStateCreateInfo out_multisample_state_create_info = {};
    out_multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    out_multisample_state_create_info.pNext = nullptr;
    out_multisample_state_create_info.flags = 0;
    out_multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    out_multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    out_multisample_state_create_info.minSampleShading = 1.0f;
    out_multisample_state_create_info.pSampleMask = nullptr;
    out_multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    out_multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    return out_multisample_state_create_info;
}

VkPipelineDepthStencilStateCreateInfo
ex::vulkan::pipeline::create_depth_stencil_state() {
    VkPipelineDepthStencilStateCreateInfo out_depth_stencil_state_create_info = {};
    out_depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    out_depth_stencil_state_create_info.pNext = nullptr;
    out_depth_stencil_state_create_info.flags = 0;
    out_depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    out_depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    out_depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    out_depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    out_depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    out_depth_stencil_state_create_info.front = {};
    out_depth_stencil_state_create_info.back = {};
    out_depth_stencil_state_create_info.minDepthBounds = 0.0f;
    out_depth_stencil_state_create_info.maxDepthBounds = 1.0f;

    return out_depth_stencil_state_create_info;
}

VkPipelineColorBlendAttachmentState
ex::vulkan::pipeline::create_color_blend_attachment_state() {
    VkPipelineColorBlendAttachmentState out_color_blend_attachment_state = {};
    out_color_blend_attachment_state.blendEnable = VK_FALSE;
    out_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    out_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    out_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    out_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    out_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    out_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    out_color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    return out_color_blend_attachment_state;
}

VkPipelineColorBlendStateCreateInfo
ex::vulkan::pipeline::create_color_blend_state(VkPipelineColorBlendAttachmentState *color_blend_attachment_state) {
    VkPipelineColorBlendStateCreateInfo out_color_blend_state_create_info = {};
    out_color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    out_color_blend_state_create_info.pNext = nullptr;
    out_color_blend_state_create_info.flags = 0;
    out_color_blend_state_create_info.logicOpEnable = VK_FALSE;
    out_color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    out_color_blend_state_create_info.attachmentCount = 1;
    out_color_blend_state_create_info.pAttachments = color_blend_attachment_state;
    out_color_blend_state_create_info.blendConstants[0] = 0.0f;
    out_color_blend_state_create_info.blendConstants[1] = 0.0f;
    out_color_blend_state_create_info.blendConstants[2] = 0.0f;
    out_color_blend_state_create_info.blendConstants[3] = 0.0f;

    return out_color_blend_state_create_info;
}

VkPipelineDynamicStateCreateInfo
ex::vulkan::pipeline::create_dynamic_state(std::vector<VkDynamicState> &dynamic_states) {
    VkPipelineDynamicStateCreateInfo out_dynamic_state_create_info = {};
    out_dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    out_dynamic_state_create_info.pNext = nullptr;
    out_dynamic_state_create_info.flags = 0;
    out_dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    out_dynamic_state_create_info.pDynamicStates = dynamic_states.data();

    return out_dynamic_state_create_info;
}
