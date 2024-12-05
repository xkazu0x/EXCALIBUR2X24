#include "ex_model.h"

void
ex::model::load(const char *file) {
    m_mesh.create(file);
    m_vertex_count = static_cast<uint32_t>(m_mesh.vertices().size());
    m_index_count = static_cast<uint32_t>(m_mesh.indices().size());
}

void
ex::model::create(VkDevice logical_device,
                  VkPhysicalDevice physical_device,
                  VkAllocationCallbacks *allocator,
                  VkCommandPool command_pool,
                  VkQueue queue) {
    create_vertex_buffer(logical_device,
                         physical_device,
                         allocator,
                         command_pool,
                         queue);
    create_index_buffer(logical_device,
                        physical_device,
                        allocator,
                        command_pool,
                        queue);
}

void
ex::model::destroy(VkDevice logical_device, VkAllocationCallbacks *allocator) {
    m_index_buffer.destroy(logical_device, allocator);
    m_vertex_buffer.destroy(logical_device, allocator);
}

void
ex::model::bind(VkCommandBuffer command_buffer) {
    VkBuffer vertex_buffers[] = { m_vertex_buffer.handle(), };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, m_index_buffer.handle(), 0, VK_INDEX_TYPE_UINT32);
}

void
ex::model::draw(VkCommandBuffer command_buffer) {
    //vkCmdDraw(command_buffer, m_vertex_count, 1, 0, 0);
    vkCmdDrawIndexed(command_buffer, m_index_count, 1, 0, 0, 0);
}


void
ex::model::create_vertex_buffer(VkDevice logical_device,
                                VkPhysicalDevice physical_device,
                                VkAllocationCallbacks *allocator,
                                VkCommandPool command_pool,
                                VkQueue queue) {
    VkDeviceSize vertex_buffer_size = sizeof(ex::vertex) * m_vertex_count;
    
    // STAGING BUFFER
    auto staging_buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    auto staging_buffer_properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    ex::vulkan::buffer staging_buffer;
    staging_buffer.create(logical_device,
                          physical_device,
                          allocator,
                          vertex_buffer_size,
                          staging_buffer_usage,
                          staging_buffer_properties);
    staging_buffer.copy_data(logical_device,
                             m_mesh.vertices().data(),
                             vertex_buffer_size);

    // VERTEX BUFFER
    auto vertex_buffer_usage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    auto vertex_buffer_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    m_vertex_buffer.create(logical_device,
                           physical_device,
                           allocator,
                           vertex_buffer_size,
                           vertex_buffer_usage,
                           vertex_buffer_properties);
    m_vertex_buffer.copy_buffer(logical_device,
                                command_pool,
                                queue,
                                staging_buffer.handle(),
                                vertex_buffer_size);
    
    staging_buffer.destroy(logical_device, allocator);
}

void
ex::model::create_index_buffer(VkDevice logical_device,
                               VkPhysicalDevice physical_device,
                               VkAllocationCallbacks *allocator,
                               VkCommandPool command_pool,
                               VkQueue queue) {
    VkDeviceSize index_buffer_size = sizeof(uint32_t) * m_index_count;

    // STAGING BUFFER
    auto staging_buffer_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    auto staging_buffer_properties =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    ex::vulkan::buffer staging_buffer;
    staging_buffer.create(logical_device,
                          physical_device,
                          allocator,
                          index_buffer_size,
                          staging_buffer_usage,
                          staging_buffer_properties);
    staging_buffer.copy_data(logical_device,
                             m_mesh.indices().data(),
                             index_buffer_size);

    // INDEX BUFFER
    auto index_buffer_usage =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    auto index_buffer_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    m_index_buffer.create(logical_device,
                          physical_device,
                          allocator,
                          index_buffer_size,
                          index_buffer_usage,
                          index_buffer_properties);
    m_index_buffer.copy_buffer(logical_device,
                               command_pool,
                               queue,
                               staging_buffer.handle(),
                               index_buffer_size);
    
    staging_buffer.destroy(logical_device, allocator);
}
