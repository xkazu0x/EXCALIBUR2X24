#include "vk_backend.h"
#include "vk_common.h"

#include <cstdint>
#include <vector>
#include <array>

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
    m_window = window;
    
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

    create_command_pool();
    
    create_swapchain(window->width(), window->height());
    create_depth_resources();
    create_render_pass();
    create_framebuffers();
    create_sync_structures();
    allocate_command_buffers();
    
    return true;
}

void
ex::vulkan::backend::shutdown() {
    vkDeviceWaitIdle(m_logical_device);

    if (!m_command_buffers.empty()) vkFreeCommandBuffers(m_logical_device, m_command_pool, m_command_buffers.size(), m_command_buffers.data());
    if (m_semaphore_render) vkDestroySemaphore(m_logical_device, m_semaphore_render, m_allocator);
    if (m_semaphore_present) vkDestroySemaphore(m_logical_device, m_semaphore_present, m_allocator);
    if (m_fence) vkDestroyFence(m_logical_device, m_fence, m_allocator);
    
    if (!m_swapchain_framebuffers.empty()) {
        for (uint32_t i = 0; i < m_swapchain_framebuffers.size(); i++) {
            vkDestroyFramebuffer(m_logical_device, m_swapchain_framebuffers[i], m_allocator);
        }
    }
    
    if (m_render_pass) vkDestroyRenderPass(m_logical_device, m_render_pass, m_allocator);
    destroy_depth_resources();
    
    if (!m_swapchain_image_views.empty()) {
        for (uint32_t i = 0; i < m_swapchain_image_views.size(); i++) {
            vkDestroyImageView(m_logical_device, m_swapchain_image_views[i], m_allocator);
        }
    }
    
    if (m_swapchain) vkDestroySwapchainKHR(m_logical_device, m_swapchain, m_allocator);
    if (m_command_pool) vkDestroyCommandPool(m_logical_device, m_command_pool, m_allocator);
    if (m_logical_device) vkDestroyDevice(m_logical_device, m_allocator);    
    if (m_surface) vkDestroySurfaceKHR(m_instance, m_surface, m_allocator);    
#ifdef EXCALIBUR_DEBUG
    if (m_debug_messenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT destroy_debug_messenger =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        destroy_debug_messenger(m_instance, m_debug_messenger, m_allocator);
    }
#endif    
    if (m_instance) vkDestroyInstance(m_instance, m_allocator);
    m_window = nullptr;
}

void
ex::vulkan::backend::begin_render() {
    if (m_window->width() == 0 || m_window->height() == 0) return;
    
    VK_CHECK(vkWaitForFences(m_logical_device,
                             1,
                             &m_fence,
                             VK_TRUE,
                             UINT64_MAX));

    VkResult result = vkAcquireNextImageKHR(m_logical_device,
                                            m_swapchain,
                                            UINT64_MAX,
                                            m_semaphore_present,
                                            nullptr,
                                            &m_next_image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain(m_window->width(), m_window->height());
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        EXFATAL("Failed to acquire swapchain image");
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    VK_CHECK(vkResetFences(m_logical_device, 1, &m_fence));
    VK_CHECK(vkResetCommandBuffer(m_command_buffers[m_next_image_index], 0));
    
    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(m_command_buffers[m_next_image_index], &command_buffer_begin_info));

    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    clear_values[1].depthStencil = { 1.0f, 0 };
    
    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = m_render_pass;
    render_pass_begin_info.framebuffer = m_swapchain_framebuffers[m_next_image_index];
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = m_swapchain_extent;
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();
    
    vkCmdBeginRenderPass(m_command_buffers[m_next_image_index],
                         &render_pass_begin_info,
                         VK_SUBPASS_CONTENTS_INLINE);    
}

void
ex::vulkan::backend::end_render() {
    if (m_window->width() == 0 || m_window->height() == 0) return;
    
    vkCmdEndRenderPass(m_command_buffers[m_next_image_index]);
    VK_CHECK(vkEndCommandBuffer(m_command_buffers[m_next_image_index]));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &m_semaphore_present;
    
    VkPipelineStageFlags wait_stage_mask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.pWaitDstStageMask = wait_stage_mask;
    
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_command_buffers[m_next_image_index];
    
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &m_semaphore_render;
    VK_CHECK(vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_fence));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &m_semaphore_render;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &m_swapchain;
    present_info.pImageIndices = &m_next_image_index;
    
    VkResult result = vkQueuePresentKHR(m_graphics_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreate_swapchain(m_window->width(), m_window->height());
    } else if (result != VK_SUCCESS) {
        EXFATAL("Failed to present swapchain image");
        throw std::runtime_error("Failed to present swapchain image");
    }
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
    physical_device_features.fillModeNonSolid = VK_TRUE;
    
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
        //if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
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
        // if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
        //            EXDEBUG("Present mode: MAILBOX");
        if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            EXDEBUG("Present mode: IMMEDIATE");
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
        m_swapchain_image_views[i] = create_image_view(m_swapchain_images[i],
                                                       VK_IMAGE_VIEW_TYPE_2D,
                                                       m_swapchain_format.format,
                                                       VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

bool
ex::vulkan::backend::create_depth_resources() {
    std::vector<VkFormat> formats = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlags feature = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    // FIND SUPPORTED FORMAT
    bool found = false;
    for (uint32_t i = 0; i < formats.size(); ++i) {
        VkFormatProperties format_properties;
        vkGetPhysicalDeviceFormatProperties(m_physical_device,
                                            formats[i],
                                            &format_properties);
        
        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (format_properties.linearTilingFeatures & feature) == feature) {
            m_depth_format = formats[i];
            found = true;
            break;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (format_properties.optimalTilingFeatures & feature) == feature) {
            m_depth_format = formats[i];
            found = true;
            break;
        }
    }
    if (!found) {
        EXFATAL("Failed to find supported depth formats");
        std::runtime_error("Failed to find supported depth formats");
        return false;
    }

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.flags = 0;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = m_depth_format;
    image_create_info.extent.width = m_swapchain_extent.width;
    image_create_info.extent.height = m_swapchain_extent.height;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = tiling;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK(vkCreateImage(m_logical_device,
                           &image_create_info,
                           m_allocator,
                           &m_depth_image));

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(m_logical_device,
                                 m_depth_image,
                                 &memory_requirements);

    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    uint32_t memory_type_index = get_memory_type_index(memory_requirements, properties);
    
    VkMemoryAllocateInfo memory_allocate_info = {};
    memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_allocate_info.pNext = nullptr;
    memory_allocate_info.allocationSize = memory_requirements.size;
    memory_allocate_info.memoryTypeIndex = memory_type_index;
    VK_CHECK(vkAllocateMemory(m_logical_device,
                              &memory_allocate_info,
                              m_allocator,
                              &m_depth_image_memory));

    vkBindImageMemory(m_logical_device, m_depth_image, m_depth_image_memory, 0);
    
    VkImageViewCreateInfo image_view_create_info = {};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = m_depth_image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = m_depth_format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    VK_CHECK(vkCreateImageView(m_logical_device,
                               &image_view_create_info,
                               m_allocator,
                               &m_depth_image_view));
    
    return true;
}

void
ex::vulkan::backend::destroy_depth_resources() {
    if (m_depth_image_view) vkDestroyImageView(m_logical_device, m_depth_image_view, m_allocator);
    if (m_depth_image) vkDestroyImage(m_logical_device, m_depth_image, m_allocator);
    if (m_depth_image_memory) vkFreeMemory(m_logical_device, m_depth_image_memory, m_allocator);
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

    VkAttachmentDescription depth_attachment_description = {};
    depth_attachment_description.flags = 0;
    depth_attachment_description.format = m_depth_format;
    depth_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::array<VkAttachmentDescription, 2> attachments = {
        color_attachment_description,
        depth_attachment_description,
    };

    VkAttachmentReference color_attachment_reference = {};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_reference = {};
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass_description = {};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &color_attachment_reference;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = &depth_attachment_reference;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments  = nullptr;

    VkSubpassDependency subpass_dependency = {};
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpass_dependency.srcAccessMask = 0;
    subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = 0;
    
    VkRenderPassCreateInfo render_pass_create_info = {};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments = attachments.data();
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
        std::array<VkImageView, 2> attachments = {
            m_swapchain_image_views[i],
            m_depth_image_view,
        };
        
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = m_render_pass;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_create_info.pAttachments = attachments.data();
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

uint32_t
ex::vulkan::backend::get_memory_type_index(VkMemoryRequirements memory_requirements,
                                           VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memory_properties);
    
    uint32_t index = 0;
    
    bool found = false;
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        if ((memory_requirements.memoryTypeBits & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            found = true;
            index = i;
            break;
        }
    }
    if (!found) {
        EXFATAL("Failed to find memory type index");
        std::runtime_error("Failed to find memory type index");
    }

    return index;
}

float
ex::vulkan::backend::get_swapchain_aspect_ratio() {
    return (float) m_swapchain_extent.width / (float) m_swapchain_extent.height;
}

void
ex::vulkan::backend::recreate_swapchain(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    EXINFO("-+SWAPCHAIN_RECREATED+-");
    EXDEBUG("Width: %d -+- Height: %d", width, height);

    // DESTROY
    vkDeviceWaitIdle(m_logical_device);

    for (size_t i = 0; i < m_swapchain_framebuffers.size(); ++i) {
        vkDestroyFramebuffer(m_logical_device, m_swapchain_framebuffers[i], m_allocator);
    }

    destroy_depth_resources();
    
    for (size_t i = 0; i < m_swapchain_image_views.size(); ++i) {
        vkDestroyImageView(m_logical_device, m_swapchain_image_views[i], m_allocator);
    }
    
    vkDestroySwapchainKHR(m_logical_device, m_swapchain, m_allocator);

    // RECREATE
    create_swapchain(width, height);
    create_depth_resources();
    create_framebuffers();
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

VkImageView
ex::vulkan::backend::create_image_view(VkImage image,
                                       VkImageViewType type,
                                       VkFormat format,
                                       VkImageAspectFlags aspect_flags) {
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
    image_view_create_info.subresourceRange.aspectMask = aspect_flags;
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
