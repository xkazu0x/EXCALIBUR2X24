#pragma once

#include "ex_window.h"
#include <cstdint>

#define MAX_KEY_COUNT 256

namespace ex {
    class input {
    public:
        void update(ex::window *window);
        bool key_pressed(uint32_t key_code);
        bool key_down(uint32_t key_code);

    private:
        bool m_key_pressed[MAX_KEY_COUNT];
        bool m_key_down[MAX_KEY_COUNT];
        bool m_key_down_last_frame[MAX_KEY_COUNT];
    };
}
