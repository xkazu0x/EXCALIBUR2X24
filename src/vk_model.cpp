#include "vk_model.h"
#include "ex_logger.h"

void
ex::vulkan::model::create(ex::vulkan::backend *backend, ex::mesh *mesh) {
    create_vertex_buffer(backend, mesh->vertices());
    create_index_buffer(backend, mesh->indices());
}

void
ex::vulkan::model::destroy(ex::vulkan::backend *backend) {        
    m_index_buffer.destroy(backend);
    m_vertex_buffer.destroy(backend);
}

void
ex::vulkan::model::bind(VkCommandBuffer command_buffer) {
    VkBuffer vertex_buffers[] = { m_vertex_buffer.handle() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, m_index_buffer.handle(), 0, VK_INDEX_TYPE_UINT32);
}

void
ex::vulkan::model::draw(VkCommandBuffer command_buffer) {
    //vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
    vkCmdDrawIndexed(command_buffer, m_index_count, 1, 0, 0, 0);
}


void
ex::vulkan::model::create_vertex_buffer(ex::vulkan::backend *backend, std::vector<ex::vertex> vertices) {
    m_vertex_count = static_cast<uint32_t>(vertices.size());
    VkDeviceSize vertex_buffer_size = sizeof(ex::vertex) * m_vertex_count;
    
    ex::vulkan::buffer staging_buffer;
    staging_buffer.set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    staging_buffer.set_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging_buffer.build(backend, vertex_buffer_size);
    
    staging_buffer.bind(backend);
    staging_buffer.map(backend);
    staging_buffer.copy_to(vertices.data(), vertex_buffer_size);
    staging_buffer.unmap(backend);

    m_vertex_buffer.set_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m_vertex_buffer.set_properties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_vertex_buffer.build(backend, vertex_buffer_size);

    m_vertex_buffer.bind(backend);
    VkCommandBuffer command_buffer = backend->begin_single_time_commands();
    m_vertex_buffer.copy_buffer(command_buffer,
                                staging_buffer.handle(),
                                vertex_buffer_size);
    backend->end_single_time_commands(command_buffer);
    staging_buffer.destroy(backend);
}

void
ex::vulkan::model::create_index_buffer(ex::vulkan::backend *backend, std::vector<uint32_t> indices) {
    m_index_count = static_cast<uint32_t>(indices.size());
    VkDeviceSize index_buffer_size = sizeof(uint32_t) * m_index_count;

    ex::vulkan::buffer staging_buffer;
    staging_buffer.set_usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    staging_buffer.set_properties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    staging_buffer.build(backend, index_buffer_size);

    staging_buffer.bind(backend);
    staging_buffer.map(backend);
    staging_buffer.copy_to(indices.data(), index_buffer_size);
    staging_buffer.unmap(backend);

    m_index_buffer.set_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    m_index_buffer.set_properties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_index_buffer.build(backend, index_buffer_size);

    m_index_buffer.bind(backend);
    VkCommandBuffer command_buffer = backend->begin_single_time_commands();
    m_index_buffer.copy_buffer(command_buffer,
                               staging_buffer.handle(),
                               index_buffer_size);
    backend->end_single_time_commands(command_buffer);
    staging_buffer.destroy(backend);
}
