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
        void sleep(uint64_t ms);
        
        bool closed();
        bool inactive();
        uint32_t width();
        uint32_t height();
        int8_t get_key(int32_t key_code);
        
        void change_display_mode();
        void change_title(std::string title);
        void set_cursor_pos(uint32_t x, uint32_t y);

        bool create_vulkan_surface(VkInstance instance,
                                   VkAllocationCallbacks *allocator,
                                   VkSurfaceKHR *surface);
        
    private:
        static LRESULT process_message_setup(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        static LRESULT process_message_redirect(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        LRESULT process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    private:
        struct window_info {
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
        
        enum window_state {
            EX_WINDOW_STATE_HIDDEN = 0x00,
            EX_WINDOW_STATE_ACTIVE = 0x01,
            EX_WINDOW_STATE_INACTIVE = 0x02,
            EX_WINDOW_STATE_CLOSED = 0x04,
        };
        
    private:
        ex::input *m_input;
        
        HINSTANCE m_instance;
        ATOM m_atom;
        HWND m_handle;

        window_info m_info;
        window_state m_state;
    };
}
