#include "vk_backend.h"
#include "ex_logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <assert.h>
#include <vector>
#include <cstdint>
#include <cstring>

#include <fstream>
#include <string>

#include <array>

#define VK_CHECK(x) if ((x) != VK_SUCCESS) { EXFATAL("Vulkan Error: %d", x); throw std::runtime_error("\"VK_CHECK\" FAILED"); }

VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
               VkDebugUtilsMessageTypeFlagsEXT /*message_type*/,
               const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
               void */*user_data*/) {
    switch (message_severity) {
    default:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
        EXERROR("%s", callback_data->pMessage);
    } break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
        EXWARN("%s", callback_data->pMessage);
    } break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
        EXINFO("%s", callback_data->pMessage);
    } break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: {
        EXDEBUG("%s", callback_data->pMessage);
    } break;
    }

    return VK_FALSE;
}

bool
ex::vulkan::backend::initialize(ex::window *window) {
    if (!create_instance()) {
        EXERROR("Failed to create vulkan instance");
        return false;
    }

    setup_debug_messenger();

    if (!window->create_vulkan_surface(m_instance, m_allocator, &m_surface)) {
        EXERROR("Failed to create vulkan surface");
        return false;
    }

    if (!select_physical_device()) {
        EXERROR("Failed to select vulkan physical device");
        return false;
    }

    if (!create_logical_device()) {
        EXERROR("Failed to create vulkan logical device");
        return false;
    }
    
    return true;
}

void
ex::vulkan::backend::shutdown() {
    vkDeviceWaitIdle(m_logical_device);

    vkDestroyDescriptorPool(m_logical_device,
                            m_descriptor_pool,
                            m_allocator);
    vkDestroyDescriptorSetLayout(m_logical_device,
                                 m_descriptor_set_layout,
                                 m_allocator);
    
    vkFreeMemory(m_logical_device,
                 m_uniform_buffer_memory,
                 m_allocator);
    vkDestroyBuffer(m_logical_device,
                    m_uniform_buffer,
                    m_allocator);
    
    if (m_index_buffer_memory) {
        vkFreeMemory(m_logical_device,
                     m_index_buffer_memory,
                     m_allocator);
        m_index_buffer_memory = 0;
    }
    
    if (m_index_buffer) {
        vkDestroyBuffer(m_logical_device,
                        m_index_buffer,
                        m_allocator);
        m_index_buffer = 0;
    }
    
    if (m_vertex_buffer_memory) {
        vkFreeMemory(m_logical_device,
                     m_vertex_buffer_memory,
                     m_allocator);
        m_vertex_buffer_memory = 0;
    }
    
    if (m_vertex_buffer) {
        vkDestroyBuffer(m_logical_device,
                        m_vertex_buffer,
                        m_allocator);
        m_vertex_buffer = 0;
    }

    // texture image ---------
    if (m_texture_image_sampler) {
        vkDestroySampler(m_logical_device,
                         m_texture_image_sampler,
                         m_allocator);
    }

    if (m_texture_image_view) {
        vkDestroyImageView(m_logical_device,
                           m_texture_image_view,
                           m_allocator);
    }

    if (m_texture_image) {
        vkDestroyImage(m_logical_device,
                       m_texture_image,
                       m_allocator);
    }

    if (m_texture_image_memory) {
        vkFreeMemory(m_logical_device,
                     m_texture_image_memory,
                     m_allocator);
    }
    // --------------------
            
    if (m_graphics_pipeline) {
        vkDestroyPipeline(m_logical_device,
                          m_graphics_pipeline,
                          m_allocator);
        m_graphics_pipeline = 0;
    }

    if (m_pipeline_layout) {
        vkDestroyPipelineLayout(m_logical_device,
                                m_pipeline_layout,
                                m_allocator);
        m_pipeline_layout = 0;
    }
    
    if (m_semaphore_render) {
        vkDestroySemaphore(m_logical_device,
                           m_semaphore_render,
                           m_allocator);
        m_semaphore_render = 0;
    }

    if (m_semaphore_present) {
        vkDestroySemaphore(m_logical_device,
                           m_semaphore_present,
                           m_allocator);
        m_semaphore_present = 0;
    }
    
    if (m_fence) {
        vkDestroyFence(m_logical_device,
                       m_fence,
                       m_allocator);
        m_fence = 0;
    }
    
    if (!m_command_buffers.empty()) {
        vkFreeCommandBuffers(m_logical_device,
                             m_command_pool,
                             m_command_buffers.size(),
                             m_command_buffers.data());
    }
    
    if (!m_swapchain_framebuffers.empty()) {
        for (uint32_t i = 0; i < m_swapchain_framebuffers.size(); i++) {
            vkDestroyFramebuffer(m_logical_device,
                                 m_swapchain_framebuffers[i],
                                 m_allocator);
            m_swapchain_framebuffers[i] = 0;
        }
    }
    
    if (m_render_pass) {
        vkDestroyRenderPass(m_logical_device,
                            m_render_pass,
                            m_allocator);
        m_render_pass = 0;
    }
    
    if (!m_swapchain_image_views.empty()) {
        for (uint32_t i = 0; i < m_swapchain_image_views.size(); i++) {
            vkDestroyImageView(m_logical_device,
                               m_swapchain_image_views[i],
                               m_allocator);
            m_swapchain_image_views[i] = 0;
        }
    }
    
    if (m_swapchain) {
        vkDestroySwapchainKHR(m_logical_device,
                              m_swapchain,
                              m_allocator);
        m_swapchain = 0;
    }

    if (m_command_pool) {
        vkDestroyCommandPool(m_logical_device,
                             m_command_pool,
                             m_allocator);
        m_command_pool = 0;
    }
    
    if (m_logical_device) {        
        vkDestroyDevice(m_logical_device, m_allocator);
        m_logical_device = 0;
    }
    
    if (m_surface) {
        vkDestroySurfaceKHR(m_instance, m_surface, m_allocator);
        m_surface = 0;
    }
    
#ifdef EXCALIBUR_DEBUG
    if (m_debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT destroy_debug_messenger =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        destroy_debug_messenger(m_instance, m_debug_messenger, m_allocator);
        m_debug_messenger = 0;
    }
#endif
    
    if (m_instance) {
        vkDestroyInstance(m_instance, m_allocator);
        m_instance = 0;
    }
}

void
ex::vulkan::backend::update(float delta) {
    glm::mat4 model = glm::rotate(glm::mat4(1.0f),
                                  delta * glm::radians(30.0f),
                                  glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, -1.0f),
                                 glm::vec3(0.0f, 0.0f, 1.0f),
                                 glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(80.0f),
                                            m_swapchain_extent.width / (float) m_swapchain_extent.height,
                                            0.01f,
                                            10.0f);
    m_mvp = projection * view * model;

    void *data;
    vkMapMemory(m_logical_device,
                m_uniform_buffer_memory,
                0,
                sizeof(m_mvp),
                0,
                &data);
    memcpy(data, &m_mvp, sizeof(m_mvp));
    vkUnmapMemory(m_logical_device,
                  m_uniform_buffer_memory);
}

bool
ex::vulkan::backend::render() {
    VK_CHECK(vkWaitForFences(m_logical_device,
                             1,
                             &m_fence,
                             VK_TRUE,
                             UINT64_MAX));

    uint32_t next_image_index = 0;
    VkResult result = vkAcquireNextImageKHR(m_logical_device,
                                            m_swapchain,
                                            UINT64_MAX,
                                            m_semaphore_present,
                                            nullptr,
                                            &next_image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        EXFATAL("Failed to acquire swapchain image");
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    VK_CHECK(vkResetFences(m_logical_device, 1, &m_fence));
    VK_CHECK(vkResetCommandBuffer(m_command_buffers[next_image_index], 0));
    
    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(m_command_buffers[next_image_index], &command_buffer_begin_info));

    std::array<VkClearValue, 1> clear_values{};
    clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = m_render_pass;
    render_pass_begin_info.framebuffer = m_swapchain_framebuffers[next_image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = m_swapchain_extent;
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();
    
    vkCmdBeginRenderPass(m_command_buffers[next_image_index],
                         &render_pass_begin_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(m_command_buffers[next_image_index],
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_graphics_pipeline);

    m_pipeline_viewport.x = 0.0f;
    m_pipeline_viewport.y = 0.0f;
    m_pipeline_viewport.width = m_swapchain_extent.width;
    m_pipeline_viewport.height = m_swapchain_extent.height;
    m_pipeline_viewport.minDepth = 0.0f;
    m_pipeline_viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_command_buffers[next_image_index], 0, 1, &m_pipeline_viewport);

    m_pipeline_scissor.offset = {0, 0};
    m_pipeline_scissor.extent = m_swapchain_extent;
    vkCmdSetScissor(m_command_buffers[next_image_index], 0, 1, &m_pipeline_scissor);

    VkBuffer vertex_buffers[] = {m_vertex_buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_command_buffers[next_image_index], 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(m_command_buffers[next_image_index], m_index_buffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(m_command_buffers[next_image_index],
                             VK_PIPELINE_BIND_POINT_GRAPHICS,
                             m_pipeline_layout,
                             0,
                             1,
                             &m_descriptor_set,
                             0,
                             nullptr);
    
    //vkCmdDraw(m_command_buffers[next_image_index], m_vertex_count, 1, 0, 0);
    vkCmdDrawIndexed(m_command_buffers[next_image_index], m_index_count, 1, 0, 0, 0);
    
    vkCmdEndRenderPass(m_command_buffers[next_image_index]);
    VK_CHECK(vkEndCommandBuffer(m_command_buffers[next_image_index]));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &m_semaphore_present;
    
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = wait_stage_mask;
    
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_command_buffers[next_image_index];
    
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &m_semaphore_render;
    VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_fence));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &m_semaphore_render;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &m_swapchain;
    present_info.pImageIndices = &next_image_index;
    
    result = vkQueuePresentKHR(m_graphics_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;
    } else if (result != VK_SUCCESS) {
        EXFATAL("Failed to present swapchain image");
        throw std::runtime_error("Failed to present swapchain image");
    }

    return true;
}

bool
ex::vulkan::backend::create_instance() {
    // INSTANCE LAYERS 
    std::vector<const char *> enabled_layers;
#ifdef EXCALIBUR_DEBUG
    enabled_layers.push_back("VK_LAYER_KHRONOS_validation");

    uint32_t available_layer_count = 0;
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, nullptr));

    std::vector<VkLayerProperties> available_layers(available_layer_count);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&available_layer_count, available_layers.data()));

    for (uint32_t i = 0; i < enabled_layers.size(); i++) {
        bool found = false;
        for (uint32_t j = 0; j < available_layer_count; j++) {
            if (!strcmp(available_layers[j].layerName, enabled_layers[i])) {
                found = true;
                break;
            }
        }
        if (!found) {
            EXERROR("Failed to find required layer: %s", enabled_layers[i]);
            return false;
        }
    }
#endif
    
    // INSTANCE EXTENSIONS
    std::vector<const char *> enabled_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };    
#ifdef EXCALIBUR_DEBUG
    enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    uint32_t available_extension_count = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(0, &available_extension_count, nullptr));

    std::vector<VkExtensionProperties> available_extensions(available_extension_count);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(0, &available_extension_count, available_extensions.data()));

    for (uint32_t i = 0; i < enabled_extensions.size(); i++) {
        bool found = false;
        for (uint32_t j = 0; j < available_extension_count; j++) {
            if (!strcmp(available_extensions[j].extensionName, enabled_extensions[i])) {
                found = true;
                break;
            }
        }
        if (!found) {
            EXERROR("Failed to find required extension: %s", enabled_extensions[i]);
            return false;
        }
    }

    VkApplicationInfo application_info = {};
    application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    application_info.pApplicationName = "EXCALIBUR";
    application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.pEngineName = "EXCALIBUR ENGINE";
    application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    application_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo = &application_info;
    instance_create_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());
    instance_create_info.ppEnabledLayerNames = enabled_layers.data();
    instance_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
    instance_create_info.ppEnabledExtensionNames = enabled_extensions.data();

    VK_CHECK(vkCreateInstance(&instance_create_info,
                              m_allocator,
                              &m_instance));

    return true;
}

void
ex::vulkan::backend::setup_debug_messenger() {
#ifdef EXCALIBUR_DEBUG
    uint32_t message_severity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT; // |
        //VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        //VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

    uint32_t message_type =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info = {};
    debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_create_info.messageSeverity = message_severity;
    debug_messenger_create_info.messageType = message_type;
    debug_messenger_create_info.pfnUserCallback = debug_callback;

    PFN_vkCreateDebugUtilsMessengerEXT create_debug_messenger =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    VK_CHECK(create_debug_messenger(m_instance,
                                    &debug_messenger_create_info,
                                    m_allocator,
                                    &m_debug_messenger));
#endif
}

bool
ex::vulkan::backend::select_physical_device() {
    uint32_t physical_device_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance,
                                        &physical_device_count,
                                        nullptr));
    if (!physical_device_count) {
        EXERROR("Failed to find devices which support vulkan");
        return false;
    }

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance,
                                        &physical_device_count,
                                        physical_devices.data()));

    for (uint32_t i = 0; i < physical_device_count; i++) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
        if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            continue;
        }
        
        uint32_t queue_family_property_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
                                                 &queue_family_property_count,
                                                 nullptr);

        std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_property_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
                                                 &queue_family_property_count,
                                                 queue_family_properties.data());

        int32_t graphics_queue_index = -1;
        int32_t present_queue_index = -1;
        
        for (uint32_t queue_index = 0; queue_index < queue_family_property_count; queue_index++) {
            if (queue_family_properties[queue_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_queue_index = queue_index;
            }

            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physical_devices[i],
                                                 queue_index,
                                                 m_surface,
                                                 &present_support);
            if (present_support) {
                present_queue_index = queue_index;
            }

            if (graphics_queue_index > -1 && present_queue_index > -1) {
                m_graphics_queue_index = graphics_queue_index;
                m_present_queue_index = present_queue_index;
                m_physical_device = physical_devices[i];
                return true;
            }        
        }

        if (graphics_queue_index < 0 && present_queue_index < 0) {
            EXERROR("Failed to find physical device graphics queue index");
            return false;
        }
    }
    
    return false;
}

bool
ex::vulkan::backend::create_logical_device() {
    std::vector<uint32_t> queue_family_indices = {
        m_graphics_queue_index
    };

    bool present_shares_graphics_queue = m_graphics_queue_index == m_present_queue_index;
    if (!present_shares_graphics_queue) {
        queue_family_indices.push_back(m_present_queue_index);
    }

    std::vector<VkDeviceQueueCreateInfo> device_queue_create_infos;

    const float queue_priority[] = { 1.0f };
    for (uint32_t i = 0; i < queue_family_indices.size(); i++) {
        VkDeviceQueueCreateInfo device_queue_create_info = {};
        device_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        device_queue_create_info.queueFamilyIndex = queue_family_indices[i];
        device_queue_create_info.queueCount = 1;
        device_queue_create_info.pQueuePriorities = queue_priority;
        device_queue_create_infos.push_back(device_queue_create_info);
    }

    std::vector<const char *> enabled_layers;
#ifdef EXCALIBUR_DEBUG
    enabled_layers.push_back("VK_LAYER_KHRONOS_validation");

    uint32_t available_layer_count = 0;
    VK_CHECK(vkEnumerateDeviceLayerProperties(m_physical_device,
                                              &available_layer_count,
                                              nullptr));

    std::vector<VkLayerProperties> available_layers{available_layer_count};
    VK_CHECK(vkEnumerateDeviceLayerProperties(m_physical_device,
                                              &available_layer_count,
                                              available_layers.data()));

    for (uint32_t i = 0; i < enabled_layers.size(); i++) {
        bool found = false;
        for (uint32_t j = 0; j < available_layer_count; j++) {
            if (!strcmp(available_layers[j].layerName, enabled_layers[i])) {
                found = true;
                break;
            }
        }
        if (!found) {
            EXERROR("Physical Device does not support layer: %s", enabled_layers[i]);
            return false;
        }
    }
#endif

    std::vector<const char *> enabled_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkPhysicalDeviceFeatures physical_device_features = {};
    physical_device_features.samplerAnisotropy = VK_TRUE;
    
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(device_queue_create_infos.size());
    device_create_info.pQueueCreateInfos = device_queue_create_infos.data();
    device_create_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());
    device_create_info.ppEnabledLayerNames = enabled_layers.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
    device_create_info.ppEnabledExtensionNames = enabled_extensions.data();
    device_create_info.pEnabledFeatures = &physical_device_features;
    VK_CHECK(vkCreateDevice(m_physical_device,
                            &device_create_info,
                            m_allocator,
                            &m_logical_device));

    vkGetDeviceQueue(m_logical_device,
                     m_graphics_queue_index,
                     0,
                     &m_graphics_queue);

    vkGetDeviceQueue(m_logical_device,
                     m_present_queue_index,
                     0,
                     &m_present_queue);
    
    return true;
}

void
ex::vulkan::backend::create_command_pool() {
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = m_graphics_queue_index;
    VK_CHECK(vkCreateCommandPool(m_logical_device,
                                 &command_pool_create_info,
                                 m_allocator,
                                 &m_command_pool));
}

void
ex::vulkan::backend::create_swapchain(uint32_t width, uint32_t height) {
    // swapchain capabilities
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device,
                                                       m_surface,
                                                       &m_swapchain_capabilities));

    // swapchain formats
    uint32_t format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device,
                                                  m_surface,
                                                  &format_count,
                                                  nullptr));    
    m_swapchain_formats.resize(format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device,
                                                  m_surface,
                                                  &format_count,
                                                  m_swapchain_formats.data()));

    // swapchain present modes
    uint32_t present_mode_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device,
                                                       m_surface,
                                                       &present_mode_count,
                                                       nullptr));

    m_swapchain_present_modes.resize(present_mode_count);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device,
                                                       m_surface,
                                                       &present_mode_count,
                                                       m_swapchain_present_modes.data()));

    // find swapchain format
    bool found_format = false;
    for (uint32_t i = 0; i < m_swapchain_formats.size(); i++) {
        VkSurfaceFormatKHR format = m_swapchain_formats[i];
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            m_swapchain_format = format;
            found_format = true;
            break;
        }
    }
    if (!found_format) {
        m_swapchain_format = m_swapchain_formats[0];
        EXWARN("Failed to find preferable swapchain image format");
    }

    // find swapchain present mode
    bool found_present_mode = false;
    for (uint32_t i = 0; i < m_swapchain_present_modes.size(); i++) {
        VkPresentModeKHR present_mode = m_swapchain_present_modes[i];
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            EXDEBUG("Present mode: MAILBOX");
            m_swapchain_present_mode = present_mode;
            found_present_mode = true;
            break;
        }
    }
    if (!found_present_mode) {
        m_swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR;
        EXWARN("Failed to find preferable present mode");
        EXDEBUG("Present mode: V-Sync");
    }

    // swapchain extent
    m_swapchain_extent = { width, height };
    if (m_swapchain_capabilities.currentExtent.width != UINT32_MAX) {
        m_swapchain_extent = m_swapchain_capabilities.currentExtent;
    }

    // swapchain min image count
    uint32_t image_count = m_swapchain_capabilities.minImageCount + 1;
    if (m_swapchain_capabilities.minImageCount > 0 && image_count > m_swapchain_capabilities.maxImageCount) {
        image_count = m_swapchain_capabilities.maxImageCount;
    }

    // create swapchain
    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.surface = m_surface;
    swapchain_create_info.minImageCount = image_count;
    swapchain_create_info.imageFormat = m_swapchain_format.format;
    swapchain_create_info.imageColorSpace = m_swapchain_format.colorSpace;
    swapchain_create_info.imageExtent = m_swapchain_extent;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (m_graphics_queue_index != m_present_queue_index) {
        std::vector<uint32_t> queue_family_indices = {
            m_graphics_queue_index,
            m_present_queue_index
        };
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
        swapchain_create_info.pQueueFamilyIndices = queue_family_indices.data();
    } else {
        swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchain_create_info.queueFamilyIndexCount = 0;
        swapchain_create_info.pQueueFamilyIndices = nullptr;
    }

    swapchain_create_info.preTransform = m_swapchain_capabilities.currentTransform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = m_swapchain_present_mode;
    swapchain_create_info.clipped = VK_TRUE;
    swapchain_create_info.oldSwapchain = nullptr;

    VK_CHECK(vkCreateSwapchainKHR(m_logical_device,
                                  &swapchain_create_info,
                                  m_allocator,
                                  &m_swapchain));

    // swapchain images
    uint32_t swapchain_image_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(m_logical_device,
                                     m_swapchain,
                                     &swapchain_image_count,
                                     nullptr));
    m_swapchain_images.resize(swapchain_image_count);
    VK_CHECK(vkGetSwapchainImagesKHR(m_logical_device,
                                     m_swapchain,
                                     &swapchain_image_count,
                                     m_swapchain_images.data()));

    // swapchain image views
    m_swapchain_image_views.resize(swapchain_image_count);
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
        /*
        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.image = m_swapchain_images[i];
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format = m_swapchain_format.format;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(m_logical_device,
                                   &image_view_create_info,
                                   m_allocator,
                                   &m_swapchain_image_views[i]));
        */
        m_swapchain_image_views[i] = create_image_view(m_swapchain_images[i],
                                                       VK_IMAGE_VIEW_TYPE_2D,
                                                       m_swapchain_format.format);
    }
}

void
ex::vulkan::backend::create_render_pass() {
    VkAttachmentDescription color_attachment_description = {};
    color_attachment_description.flags = 0;
    color_attachment_description.format = m_swapchain_format.format;
    color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference = {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_description = {};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = nullptr;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments  = nullptr;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &color_attachment_description;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &subpass_dependency;
    VK_CHECK(vkCreateRenderPass(m_logical_device,
                                &render_pass_create_info,
                                m_allocator,
                                &m_render_pass));
}

void
ex::vulkan::backend::create_framebuffers() {
    m_swapchain_framebuffers.resize(m_swapchain_images.size());
    for (uint32_t i = 0; i < m_swapchain_images.size(); i++) {
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = m_render_pass;
        framebuffer_create_info.attachmentCount = 1;
        framebuffer_create_info.pAttachments = &m_swapchain_image_views[i];
        framebuffer_create_info.width = m_swapchain_extent.width;
        framebuffer_create_info.height = m_swapchain_extent.height;
        framebuffer_create_info.layers = 1;
        VK_CHECK(vkCreateFramebuffer(m_logical_device,
                                     &framebuffer_create_info,
                                     m_allocator,
                                     &m_swapchain_framebuffers[i]));
    }
}

void
ex::vulkan::backend::allocate_command_buffers() {
    m_command_buffers.resize(m_swapchain_images.size());
    
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = m_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = static_cast<uint32_t>(m_command_buffers.size());
    VK_CHECK(vkAllocateCommandBuffers(m_logical_device,
                                      &command_buffer_allocate_info,
                                      m_command_buffers.data()));
}

void
ex::vulkan::backend::create_sync_structures() {
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(m_logical_device,
                           &fence_create_info,
                           m_allocator,
                           &m_fence));

    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr; 
    semaphore_create_info.flags = 0;
    VK_CHECK(vkCreateSemaphore(m_logical_device,
                               &semaphore_create_info,
                               m_allocator,
                               &m_semaphore_present));

    VK_CHECK(vkCreateSemaphore(m_logical_device,
                               &semaphore_create_info,
                               m_allocator,
                               &m_semaphore_render));
}

bool
ex::vulkan::backend::create_depth_resources() {
    /*
    std::vector<VkFormat> formats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlags feature = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkFormat depth_format;
    for (uint32_t i = 0; i < formats.size(); ++i) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_physical_device,
                                            formats[i],
                                            &properties);

        bool found = false;
        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (properties.linearTilingFeatures & feature) == feature) {
            depth_format = formats[i];
            found = true;
            break;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (properties.optimalTilingFeatures & feature) == feature) {
            depth_format = formats[i];
            found = true;
            break;
        }
        if (!found) {
            EXFATAL("Failed to find depth format");
            return false;
        }
    }    
    */
    
    return true;
}

void
ex::vulkan::backend::create_descriptor_set_layout() {
    std::array<VkDescriptorSetLayoutBinding, 2> descriptor_set_layout_bindings;
    descriptor_set_layout_bindings[0].binding = 0;
    descriptor_set_layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_set_layout_bindings[0].descriptorCount = 1;
    descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    descriptor_set_layout_bindings[0].pImmutableSamplers = nullptr;

    descriptor_set_layout_bindings[1].binding = 1;
    descriptor_set_layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_set_layout_bindings[1].descriptorCount = 1;
    descriptor_set_layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descriptor_set_layout_bindings[1].pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext = nullptr;
    descriptor_set_layout_create_info.flags = 0;
    descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();
    VK_CHECK(vkCreateDescriptorSetLayout(m_logical_device,
                                         &descriptor_set_layout_create_info,
                                         m_allocator,
                                         &m_descriptor_set_layout));
}

void
ex::vulkan::backend::create_pipeline() {
    // shaders
    std::vector<char> vertex_code = read_file("res/shaders/triangle.vert.spv");
    std::vector<char> fragment_code = read_file("res/shaders/triangle.frag.spv");
    EXDEBUG("Vertex shader size: %d", vertex_code.size());
    EXDEBUG("Fragment shader size: %d", fragment_code.size());

    VkShaderModule vertex_shader = create_shader_module(vertex_code);
    VkShaderModule fragment_shader = create_shader_module(fragment_code);

    // shader stages
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{};
    shader_stages[0].pNext = nullptr;
    shader_stages[0].flags = 0;
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vertex_shader;
    shader_stages[0].pName = "main";

    shader_stages[1].pNext = nullptr;
    shader_stages[1].flags = 0;
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fragment_shader;
    shader_stages[1].pName = "main";

    // vertex input
    VkVertexInputBindingDescription vertex_input_binding_description = {};
    vertex_input_binding_description.binding = 0;
    vertex_input_binding_description.stride = sizeof(ex::vertex);
    vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> vertex_input_attribute_descriptions{};
    vertex_input_attribute_descriptions[0].location = 0;
    vertex_input_attribute_descriptions[0].binding = 0;
    vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[0].offset = offsetof(ex::vertex, pos);
    
    vertex_input_attribute_descriptions[1].location = 1;
    vertex_input_attribute_descriptions[1].binding = 0;
    vertex_input_attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertex_input_attribute_descriptions[1].offset = offsetof(ex::vertex, color);

    vertex_input_attribute_descriptions[2].location = 2;
    vertex_input_attribute_descriptions[2].binding = 0;
    vertex_input_attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertex_input_attribute_descriptions[2].offset = offsetof(ex::vertex, uv);
    
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding_description;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_input_attribute_descriptions.size());
    vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attribute_descriptions.data();

    // input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;    

    // viewport and scissor
    m_pipeline_viewport.x = 0.0f;
    m_pipeline_viewport.y = 0.0f;
    m_pipeline_viewport.width = static_cast<float>(m_swapchain_extent.width);
    m_pipeline_viewport.height = static_cast<float>(m_swapchain_extent.height);
    m_pipeline_viewport.minDepth = 0.0f;
    m_pipeline_viewport.maxDepth = 1.0f;

    m_pipeline_scissor.offset = { 0, 0 };
    m_pipeline_scissor.extent = m_swapchain_extent;

    VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &m_pipeline_viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &m_pipeline_scissor;

    // rasterization
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.depthClampEnable = VK_FALSE;
    rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = VK_CULL_MODE_NONE;
    rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_create_info.depthBiasEnable = VK_FALSE;
    rasterization_state_create_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_create_info.depthBiasClamp = 0.0f;
    rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;
    rasterization_state_create_info.lineWidth = 1.0f;

    // multisampling
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.sampleShadingEnable = VK_FALSE;
    multisample_state_create_info.minSampleShading = 1.0f;
    multisample_state_create_info.pSampleMask = nullptr;
    multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_create_info.alphaToOneEnable = VK_FALSE;

    // depth stencil
    /*
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_create_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_create_info.front = {};
    depth_stencil_state_create_info.back = {};
    depth_stencil_state_create_info.minDepthBounds = 0.0f;
    depth_stencil_state_create_info.maxDepthBounds = 1.0f;
    */
    // color blending
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

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = VK_FALSE;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
    color_blend_state_create_info.blendConstants[0] = 0.0f;
    color_blend_state_create_info.blendConstants[1] = 0.0f;
    color_blend_state_create_info.blendConstants[2] = 0.0f;
    color_blend_state_create_info.blendConstants[3] = 0.0f;

    // dynamic state
    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    
    VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_create_info.pDynamicStates = dynamic_states.data();
    
    // pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &m_descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 0;
    pipeline_layout_create_info.pPushConstantRanges = nullptr;
    VK_CHECK(vkCreatePipelineLayout(m_logical_device,
                                    &pipeline_layout_create_info,
                                    m_allocator,
                                    &m_pipeline_layout));

    // create graphics pipeline
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
    //graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    graphics_pipeline_create_info.pDepthStencilState = nullptr;
    graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    
    graphics_pipeline_create_info.layout = m_pipeline_layout;
    graphics_pipeline_create_info.renderPass = m_render_pass;
    graphics_pipeline_create_info.subpass = m_pipeline_subpass;
    
    graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
    graphics_pipeline_create_info.basePipelineIndex = 0;

    VK_CHECK(vkCreateGraphicsPipelines(m_logical_device,
                                       nullptr,
                                       1,
                                       &graphics_pipeline_create_info,
                                       m_allocator,
                                       &m_graphics_pipeline));

    // destroy shaders
    vkDestroyShaderModule(m_logical_device,
                          vertex_shader,
                          m_allocator);

    vkDestroyShaderModule(m_logical_device,
                          fragment_shader,
                          m_allocator);
}

void
ex::vulkan::backend::create_texture_image(const char *file) {
    int width, height, channels;
    stbi_uc *image_data = stbi_load(file, &width, &height, &channels, STBI_rgb_alpha);
    if (!image_data) {
        throw std::runtime_error("Failed to load texture image");
    }

    // Write image data to staging buffer
    VkDeviceSize image_size = width * height * 4;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(image_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer,
                  staging_buffer_memory);

    void *data;
    vkMapMemory(m_logical_device,
                staging_buffer_memory,
                0,
                image_size,
                0,
                &data);
    memcpy(data, image_data, static_cast<size_t>(image_size));
    vkUnmapMemory(m_logical_device, staging_buffer_memory);

    stbi_image_free(image_data);
    
    // Create Image
    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    //image_create_info.format = VK_FORMAT_R8G8B8A8_SRGB; // TODO: check if format is supported
    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM; // TODO: check if format is supported
    image_create_info.extent.width = static_cast<uint32_t>(width);
    image_create_info.extent.height = static_cast<uint32_t>(height);
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;

    m_texture_image_layout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    //image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.initialLayout = m_texture_image_layout;
    VK_CHECK(vkCreateImage(m_logical_device,
                           &image_create_info,
                           m_allocator,
                           &m_texture_image));

    // allocate image
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(m_logical_device,
                                 m_texture_image,
                                 &memory_requirements);

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &physical_device_memory_properties);
    
    uint32_t memory_type_index = 0;
    // TODO: check error
    for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
        if ((memory_requirements.memoryTypeBits & (1 << i)) &&
            (physical_device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            memory_type_index = i;
            break;
        }
    }
    
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = nullptr;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    VK_CHECK(vkAllocateMemory(m_logical_device,
                              &memory_allocate_info,
                              m_allocator,
                              &m_texture_image_memory));

    vkBindImageMemory(m_logical_device, m_texture_image, m_texture_image_memory, 0);

    // Change layout to TRANSFER_DST
    VkCommandBuffer command_buffer = begin_single_time_commands();
    
    VkImageMemoryBarrier image_memory_barrier = {};
    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = nullptr;
    //image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    //image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    image_memory_barrier.srcAccessMask = 0;
    image_memory_barrier.dstAccessMask = 0;
    image_memory_barrier.oldLayout = m_texture_image_layout;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image = m_texture_image;
    image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &image_memory_barrier);
    
    end_single_time_commands(command_buffer);

    m_texture_image_layout = image_memory_barrier.newLayout;
    
    // Copy buffer data to image
    VkCommandBuffer copy_command_buffer = begin_single_time_commands();

    VkBufferImageCopy buffer_image_copy = {};
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageOffset = { 0, 0, 0 };
    buffer_image_copy.imageExtent.width = static_cast<uint32_t>(width);
    buffer_image_copy.imageExtent.height = static_cast<uint32_t>(height);
    buffer_image_copy.imageExtent.depth = 1;    
    vkCmdCopyBufferToImage(copy_command_buffer,
                           staging_buffer,
                           m_texture_image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &buffer_image_copy);
    
    end_single_time_commands(copy_command_buffer);

    // Change layout to SHADER_READ
    VkCommandBuffer shader_command_buffer = begin_single_time_commands();
    
    VkImageMemoryBarrier image_memory_barrier1 = {};
    image_memory_barrier1.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier1.pNext = nullptr;
    //image_memory_barrier1.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    //image_memory_barrier1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    image_memory_barrier1.srcAccessMask = 0;
    image_memory_barrier1.dstAccessMask = 0;
    image_memory_barrier1.oldLayout = m_texture_image_layout;
    image_memory_barrier1.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_memory_barrier1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier1.image = m_texture_image;
    image_memory_barrier1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_memory_barrier1.subresourceRange.baseMipLevel = 0;
    image_memory_barrier1.subresourceRange.levelCount = 1;
    image_memory_barrier1.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier1.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(shader_command_buffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &image_memory_barrier1);
    
    end_single_time_commands(shader_command_buffer);

    m_texture_image_layout = image_memory_barrier1.newLayout;
    
    // Destroy staging buffer
    vkDestroyBuffer(m_logical_device, staging_buffer, m_allocator);
    vkFreeMemory(m_logical_device, staging_buffer_memory, m_allocator);

    // Create image view
    m_texture_image_view = create_image_view(m_texture_image,
                                             VK_IMAGE_VIEW_TYPE_2D,
                                             VK_FORMAT_R8G8B8A8_UNORM);

    // Create sampler
    VkSamplerCreateInfo sampler_create_info = {};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = VK_FILTER_LINEAR;
    sampler_create_info.minFilter = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_create_info.mipLodBias = 0.0f;
    sampler_create_info.anisotropyEnable = VK_TRUE;
    sampler_create_info.maxAnisotropy = 16;
    sampler_create_info.compareEnable = VK_FALSE;
    sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod = 0.0f;
    sampler_create_info.maxLod = 0.0f;
    sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;
    VK_CHECK(vkCreateSampler(m_logical_device,
                             &sampler_create_info,
                             m_allocator,
                             &m_texture_image_sampler));
}

void
ex::vulkan::backend::create_vertex_buffer(std::vector<ex::vertex> &vertices) {
    m_vertex_count = static_cast<uint32_t>(vertices.size());
    VkDeviceSize buffer_size = sizeof(ex::vertex) * m_vertex_count;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer,
                  staging_buffer_memory);

    void *data;
    vkMapMemory(m_logical_device,
                staging_buffer_memory,
                0,
                buffer_size,
                0,
                &data);
    memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
    vkUnmapMemory(m_logical_device, staging_buffer_memory);
        
    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  m_vertex_buffer, m_vertex_buffer_memory);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = m_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    VK_CHECK(vkAllocateCommandBuffers(m_logical_device,
                                      &command_buffer_allocate_info,
                                      &command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

    VkBufferCopy buffer_copy;
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = buffer_size;
    vkCmdCopyBuffer(command_buffer, staging_buffer, m_vertex_buffer, 1, &buffer_copy);
    
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

    vkQueueWaitIdle(m_graphics_queue);
    vkFreeCommandBuffers(m_logical_device, m_command_pool, 1, &command_buffer);
    
    vkDestroyBuffer(m_logical_device, staging_buffer, m_allocator);
    vkFreeMemory(m_logical_device, staging_buffer_memory, m_allocator);
}

void
ex::vulkan::backend::create_index_buffer(std::vector<uint32_t> &indices) {
    m_index_count = static_cast<uint32_t>(indices.size());
    VkDeviceSize buffer_size = sizeof(uint32_t) * m_index_count;

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  staging_buffer,
                  staging_buffer_memory);

    void *data;
    vkMapMemory(m_logical_device,
                staging_buffer_memory,
                0,
                buffer_size,
                0,
                &data);
    memcpy(data, indices.data(), (size_t) buffer_size);
    vkUnmapMemory(m_logical_device, staging_buffer_memory);
        
    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  m_index_buffer, m_index_buffer_memory);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = m_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    VK_CHECK(vkAllocateCommandBuffers(m_logical_device,
                                      &command_buffer_allocate_info,
                                      &command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

    VkBufferCopy buffer_copy;
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = buffer_size;
    vkCmdCopyBuffer(command_buffer, staging_buffer, m_index_buffer, 1, &buffer_copy);
    
    VK_CHECK(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE));

    vkQueueWaitIdle(m_graphics_queue);
    vkFreeCommandBuffers(m_logical_device, m_command_pool, 1, &command_buffer);
    
    vkDestroyBuffer(m_logical_device, staging_buffer, m_allocator);
    vkFreeMemory(m_logical_device, staging_buffer_memory, m_allocator);
}

void
ex::vulkan::backend::create_uniform_buffer() {
    VkDeviceSize buffer_size = sizeof(m_mvp);
    create_buffer(buffer_size,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  m_uniform_buffer,
                  m_uniform_buffer_memory);
}

void
ex::vulkan::backend::create_descriptor_pool() {
    std::array<VkDescriptorPoolSize, 2> descriptor_pool_sizes;
    descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_sizes[0].descriptorCount = 1;

    descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_sizes[1].descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = 0;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();
    VK_CHECK(vkCreateDescriptorPool(m_logical_device,
                                    &descriptor_pool_create_info,
                                    m_allocator,
                                    &m_descriptor_pool));
}

void
ex::vulkan::backend::create_descriptor_set() {
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = m_descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &m_descriptor_set_layout;
    VK_CHECK(vkAllocateDescriptorSets(m_logical_device,
                                      &descriptor_set_allocate_info,
                                      &m_descriptor_set));

    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer = m_uniform_buffer;
    descriptor_buffer_info.offset = 0;
    descriptor_buffer_info.range = sizeof(m_mvp);

    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler = m_texture_image_sampler;
    descriptor_image_info.imageView = m_texture_image_view;
    descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    
    std::array<VkWriteDescriptorSet, 2> write_descriptor_sets;
    write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[0].pNext = nullptr;
    write_descriptor_sets[0].dstSet = m_descriptor_set;
    write_descriptor_sets[0].dstBinding = 0;
    write_descriptor_sets[0].dstArrayElement = 0;
    write_descriptor_sets[0].descriptorCount = 1;
    write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_sets[0].pImageInfo = nullptr;
    write_descriptor_sets[0].pBufferInfo = &descriptor_buffer_info;
    write_descriptor_sets[0].pTexelBufferView = nullptr;

    write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_sets[1].pNext = nullptr;
    write_descriptor_sets[1].dstSet = m_descriptor_set;
    write_descriptor_sets[1].dstBinding = 1;
    write_descriptor_sets[1].dstArrayElement = 0;
    write_descriptor_sets[1].descriptorCount = 1;
    write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write_descriptor_sets[1].pImageInfo = &descriptor_image_info;
    write_descriptor_sets[1].pBufferInfo = nullptr;
    write_descriptor_sets[1].pTexelBufferView = nullptr;
    
    vkUpdateDescriptorSets(m_logical_device,
                           write_descriptor_sets.size(),
                           write_descriptor_sets.data(),
                           0, nullptr);
}

void
ex::vulkan::backend::recreate_swapchain(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    EXINFO("+ SWAPCHAIN RECREATED");
    
    vkDeviceWaitIdle(m_logical_device);
    
    for (size_t i = 0; i < m_swapchain_framebuffers.size(); ++i) {
        vkDestroyFramebuffer(m_logical_device,
                             m_swapchain_framebuffers[i],
                             m_allocator);
    }
    
    for (size_t i = 0; i < m_swapchain_image_views.size(); ++i) {
        vkDestroyImageView(m_logical_device,
                           m_swapchain_image_views[i],
                           m_allocator);
    }
    
    vkDestroySwapchainKHR(m_logical_device, m_swapchain, m_allocator);

    create_swapchain(width, height);
    create_framebuffers();    
}

VkImageView
ex::vulkan::backend::create_image_view(VkImage image,
                                       VkImageViewType type,
                                       VkFormat format) {
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image;
    image_view_create_info.viewType = type;
    image_view_create_info.format = format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    VkImageView out_image_view;
    VK_CHECK(vkCreateImageView(m_logical_device,
                               &image_view_create_info,
                               m_allocator,
                               &out_image_view));
    return out_image_view;
}

std::vector<char>
ex::vulkan::backend::read_file(const char *filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        EXFATAL("Failed to open file");
        throw std::runtime_error("Failed to open file");
    }
    
    size_t file_size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(file_size);
    
    file.seekg(0, file.beg);
    file.read(buffer.data(), file_size);
    file.close();
    
    return buffer;
}

VkShaderModule
ex::vulkan::backend::create_shader_module(const std::vector<char> &shader_code) {
    VkShaderModule out_shader_module;
    VkShaderModuleCreateInfo shader_module_create_info = {};
    shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_create_info.codeSize = static_cast<size_t>(shader_code.size());
    shader_module_create_info.pCode = reinterpret_cast<const uint32_t *>(shader_code.data());
    VK_CHECK(vkCreateShaderModule(m_logical_device,
                                  &shader_module_create_info,
                                  m_allocator,
                                  &out_shader_module));
    return out_shader_module;
}

void
ex::vulkan::backend::create_buffer(VkDeviceSize size,
                                   VkBufferUsageFlags usage,
                                   VkMemoryPropertyFlags properties,
                                   VkBuffer &buffer,
                                   VkDeviceMemory &buffer_memory) {
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;
    VK_CHECK(vkCreateBuffer(m_logical_device,
                            &buffer_create_info,
                            m_allocator,
                            &buffer));

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(m_logical_device,
                                  buffer,
                                  &memory_requirements);
    
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &physical_device_memory_properties);
    
    uint32_t memory_type_index = 0;
    // TODO: check error
    for (uint32_t i = 0; i < physical_device_memory_properties.memoryTypeCount; ++i) {
        if ((memory_requirements.memoryTypeBits & (1 << i)) &&
            (physical_device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            memory_type_index = i;
            break;
        }
    }
    
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    VK_CHECK(vkAllocateMemory(m_logical_device,
                              &memory_allocate_info,
                              m_allocator,
                              &buffer_memory));

    vkBindBufferMemory(m_logical_device,
                       buffer,
                       buffer_memory,
                       0);
}

VkCommandBuffer
ex::vulkan::backend::begin_single_time_commands() {
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = m_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    VK_CHECK(vkAllocateCommandBuffers(m_logical_device,
                                      &command_buffer_allocate_info,
                                      &command_buffer));

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;
    VK_CHECK(vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info));

    return command_buffer;
}

void
ex::vulkan::backend::end_single_time_commands(VkCommandBuffer command_buffer) {
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = nullptr;
    submit_info.pWaitDstStageMask = nullptr;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = nullptr;
    
    vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphics_queue);

    vkFreeCommandBuffers(m_logical_device, m_command_pool, 1, &command_buffer);
}
