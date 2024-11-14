#include "ex_window.h"
#include <memory>

int main() {
    const auto window = std::make_unique<ex::window>();
    
    window->create("EXCALIBUR", 960, 540);
    window->show();

    while (window->is_active()) {
        window->update();
    }
    
    window->destroy();
    return 0;
}
