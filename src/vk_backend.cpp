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

bool
ex::vulkan::backend::initialize() {
    if (!create_instance()) {
        EXFATAL("Failed to create vulkan instance");
        return false;
    }

    setup_debug_messenger();
    
    return true;
}

void
ex::vulkan::backend::shutdown() {
#ifdef EXCALIBUR_DEBUG
    PFN_vkDestroyDebugUtilsMessengerEXT destroy_debug_messenger =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
    destroy_debug_messenger(_instance, _debug_messenger, _allocator);
#endif
    vkDestroyInstance(_instance, _allocator);
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
            return -1;
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
            return -1;
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
                              _allocator,
                              &_instance));

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
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
    VK_CHECK(create_debug_messenger(_instance,
                                    &debug_messenger_create_info,
                                    _allocator,
                                    &_debug_messenger));
#endif
}
