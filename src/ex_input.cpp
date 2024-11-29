#include "ex_input.h"

void
ex::input::update(ex::window *window) {
    // KEYBOARD -----
    for (uint32_t i = 0; i < MAX_KEY_COUNT; ++i) {
        // DOWN
        if (window->get_key(i)) m_key_down[i] = true;
        else m_key_down[i] = false;

        // PRESSED
        if (m_key_down[i] && !m_key_down_last_frame[i]) m_key_pressed[i] = true;
        else m_key_pressed[i] = false;

        // UPDATE LAST FRAME
        m_key_down_last_frame[i] = m_key_down[i];
    }
}

bool
ex::input::key_pressed(uint32_t key_code) {
    return m_key_pressed[key_code];
}

bool
ex::input::key_down(uint32_t key_code) {
    return m_key_down[key_code];
}
