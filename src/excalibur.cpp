#include "ex_window.h"
#include "ex_logger.h"

#include "vk_backend.h"

#include <memory>

int main() {
    EXFATAL("-+=+EXCALIBUR+=+-");
    const auto window = std::make_unique<ex::window>();
    const auto vulkan_backend = std::make_unique<ex::vulkan::backend>();
    
    window->create("EXCALIBUR", 960, 540);
    window->show();
    
    vulkan_backend->initialize();
    
    while (window->is_active()) {
        window->update();
    }

    vulkan_backend->shutdown();
    window->destroy();
    return 0;
}
