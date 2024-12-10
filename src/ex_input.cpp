#include "ex_input.h"
#include "ex_logger.h"

void
ex::input::initialize() {    
}

void
ex::input::shutdown() {
}

void
ex::input::update() {
    for (uint32_t i = 0; i < EX_KEY_MAX_COUNT; i++) {
        m_keyboard_previous.keys[i] = m_keyboard_current.keys[i];
    }

    for (uint32_t i = 0; i < EX_BUTTON_MAX_COUNT; i++) {
        m_mouse_previous.buttons[i] = m_mouse_current.buttons[i];
    }
}

void
ex::input::process_key(uint32_t key_code, bool pressed) {
    if (m_keyboard_current.keys[key_code] != pressed) {
        m_keyboard_current.keys[key_code] = pressed;
    }
}

void
ex::input::process_button(uint32_t button_code, bool pressed) {
    if (m_mouse_current.buttons[button_code] != pressed) {
        m_mouse_current.buttons[button_code] = pressed;
    }
}

void
ex::input::process_mouse_move(int16_t x, int16_t y) {
    if (m_mouse_current.x != x || m_mouse_current.y != y) {
        m_mouse_current.x = x;
        m_mouse_current.y = y;
        // EXDEBUG("MOUSE X: %d | MOUSE Y: %d", x, y);
    }
}

void
ex::input::process_mouse_wheel(int8_t /*z_delta*/) {
}

bool
ex::input::key_down(uint32_t key_code) {
    return m_keyboard_current.keys[key_code];
}

bool
ex::input::key_pressed(uint32_t key_code) {
    return m_keyboard_current.keys[key_code] && !m_keyboard_previous.keys[key_code];
}

bool
ex::input::button_down(uint32_t button_code) {
    return m_mouse_current.buttons[button_code];
}

bool
ex::input::button_pressed(uint32_t button_code) {
    return m_mouse_current.buttons[button_code] && !m_mouse_previous.buttons[button_code];
}

void
ex::input::mouse_position(int32_t *x, int32_t *y) {
    *x = m_mouse_current.x;
    *y = m_mouse_current.y;
}
