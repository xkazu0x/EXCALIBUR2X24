#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <cstdint>

namespace ex {
    enum window_state {
        EX_WINDOW_STATE_HIDDEN,
        EX_WINDOW_STATE_ACTIVE,
        EX_WINDOW_STATE_CLOSED,
    };
    
    class window {
    public:
        void create(std::string title, uint32_t width, uint32_t height);
        void destroy();
        void update();
        
        void show();
        void close();

        bool is_active();

    private:
        static LRESULT process_message_setup(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        static LRESULT process_message_redirect(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        LRESULT process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        
    private:
        HINSTANCE m_instance { };
        ATOM m_atom { };
        HWND m_handle { };

        window_state m_state { };

        std::string m_title { };
        uint32_t m_width { };
        uint32_t m_height { };
    };
}
