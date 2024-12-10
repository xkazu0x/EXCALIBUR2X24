#pragma once

#include "ex_input.h"

//#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <string>
#include <cstdint>

namespace ex {    
    class window {
    public:
        void create(ex::input *input, std::string title, uint32_t width, uint32_t height, bool fullscreen);
        void destroy();
        void update();
        
        void show();
        void close();
        bool is_active();
        bool is_minimized();
        uint32_t width();
        uint32_t height();
        int8_t get_key(int32_t key_code);
        void change_display_mode();

        bool create_vulkan_surface(VkInstance instance,
                                   VkAllocationCallbacks *allocator,
                                   VkSurfaceKHR *surface);
        
    private:
        static LRESULT process_message_setup(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        static LRESULT process_message_redirect(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        LRESULT process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    private:
        struct info {
            std::string title;
            uint32_t current_width;
            uint32_t current_height;
            uint32_t windowed_width;
            uint32_t windowed_height;
            uint32_t fullscreen_width;
            uint32_t fullscreen_height;
            int32_t xpos;
            int32_t ypos;
            bool fullscreen;
        };
        
        enum state {
            EX_WINDOW_STATE_HIDDEN = 0x00,
            EX_WINDOW_STATE_ACTIVE = 0x01,
            EX_WINDOW_STATE_CLOSED = 0x02,
        };
        
    private:
        ex::input *m_input;
        
        HINSTANCE m_instance;
        ATOM m_atom;
        HWND m_handle;

        info m_info;
        uint32_t m_state;
        bool m_minimized;
    };
}
