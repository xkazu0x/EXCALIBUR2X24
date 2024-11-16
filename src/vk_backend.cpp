#include "vk_backend.h"
#include "ex_logger.h"

#include <assert.h>
#include <vector>
#include <cstdint>
#include <cstring>

#define VK_CHECK(x) assert((x) == VK_SUCCESS);

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

void
ex::vulkan::backend::shutdown() {
    vkDeviceWaitIdle(m_logical_device);
    
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

bool ex::vulkan::backend::create_surface(ex::window *window) {
    if (!window->create_vulkan_surface(m_instance, m_allocator, &m_surface)) {
        return false;
    }

    return true;
}

bool ex::vulkan::backend::select_physical_device() {
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

    bool physical_device_is_suitable = false;
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
                physical_device_is_suitable = true;
            }        
        }

        if (graphics_queue_index < 0 && present_queue_index < 0) {
            EXERROR("Failed to find physical device graphics queue index");
            return false;
        }

        if (physical_device_is_suitable) break;
    }
    
    return physical_device_is_suitable;
}

bool ex::vulkan::backend::create_logical_device() {
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
