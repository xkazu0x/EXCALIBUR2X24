#pragma once

#include <cstdint>

enum ex_buttons {
    EX_BUTTON_LEFT,
    EX_BUTTON_RIGHT,
    EX_BUTTON_MIDDLE,
    EX_BUTTON_MAX_COUNT,
};

#define DEFINE_KEY(name, code) EX_KEY_##name = code

enum ex_keys {
    DEFINE_KEY(TAB, 0x09),
    
    DEFINE_KEY(ESCAPE, 0x1B),
    
    DEFINE_KEY(LEFT, 0x25),
    DEFINE_KEY(UP, 0x26),
    DEFINE_KEY(RIGHT, 0x27),
    DEFINE_KEY(DOWN, 0x28),

    DEFINE_KEY(0, 0x30),
    DEFINE_KEY(1, 0x31),
    DEFINE_KEY(2, 0x32),
    DEFINE_KEY(3, 0x33),
    DEFINE_KEY(4, 0x34),
    DEFINE_KEY(5, 0x35),
    DEFINE_KEY(6, 0x36),
    DEFINE_KEY(7, 0x37),
    DEFINE_KEY(8, 0x38),
    DEFINE_KEY(9, 0x39),
    
    DEFINE_KEY(A, 0x41),
    DEFINE_KEY(B, 0x42),
    DEFINE_KEY(C, 0x43),
    DEFINE_KEY(D, 0x44),
    DEFINE_KEY(E, 0x45),
    DEFINE_KEY(F, 0x46),
    DEFINE_KEY(G, 0x47),
    DEFINE_KEY(H, 0x48),
    DEFINE_KEY(I, 0x49),
    DEFINE_KEY(J, 0x4A),
    DEFINE_KEY(K, 0x4B),
    DEFINE_KEY(L, 0x4C),
    DEFINE_KEY(M, 0x4D),
    DEFINE_KEY(N, 0x4E),
    DEFINE_KEY(O, 0x4F),
    DEFINE_KEY(P, 0x50),
    DEFINE_KEY(Q, 0x51),
    DEFINE_KEY(R, 0x52),
    DEFINE_KEY(S, 0x53),
    DEFINE_KEY(T, 0x54),
    DEFINE_KEY(U, 0x55),
    DEFINE_KEY(V, 0x56),
    DEFINE_KEY(W, 0x57),
    DEFINE_KEY(X, 0x58),
    DEFINE_KEY(Y, 0x59),
    DEFINE_KEY(Z, 0x5A),
    
    DEFINE_KEY(F1, 0x70),
    DEFINE_KEY(F2, 0x71),
    DEFINE_KEY(F3, 0x72),
    DEFINE_KEY(F4, 0x73),
    DEFINE_KEY(F5, 0x74),
    DEFINE_KEY(F6, 0x75),
    DEFINE_KEY(F7, 0x76),
    DEFINE_KEY(F8, 0x77),
    DEFINE_KEY(F9, 0x78),
    DEFINE_KEY(F10, 0x79),
    DEFINE_KEY(F11, 0x7A),
    DEFINE_KEY(F12, 0x7B),
    DEFINE_KEY(F13, 0x7C),
    DEFINE_KEY(F14, 0x7D),
    DEFINE_KEY(F15, 0x7E),
    DEFINE_KEY(F16, 0x7F),
    DEFINE_KEY(F17, 0x80),
    DEFINE_KEY(F18, 0x81),
    DEFINE_KEY(F19, 0x82),
    DEFINE_KEY(F20, 0x83),
    DEFINE_KEY(F21, 0x84),
    DEFINE_KEY(F22, 0x85),
    DEFINE_KEY(F23, 0x86),
    DEFINE_KEY(F24, 0x87),
    
    EX_KEY_MAX_COUNT,
};

namespace ex {
    class input {
    public:
        void initialize();
        void shutdown();
        void update();
        
        void process_key(uint32_t key_code, bool pressed);
        void process_button(uint32_t button_code, bool pressed);
        void process_mouse_move(int16_t x, int16_t y);
        void process_mouse_wheel(int8_t z_delta);

        bool key_down(uint32_t key_code);
        bool key_pressed(uint32_t key_code);
        
        bool button_down(uint32_t button_code);
        bool button_pressed(uint32_t button_code);
        void mouse_position(int32_t *x, int32_t *y);

    private:
        struct keyboard_state {
            bool keys[256];
        };
        
        struct mouse_state {
            int16_t x;
            int16_t y;
            bool buttons[EX_BUTTON_MAX_COUNT];
        };
        
    private:
        keyboard_state m_keyboard_current;
        keyboard_state m_keyboard_previous;
        mouse_state m_mouse_current;
        mouse_state m_mouse_previous;
    };
}
