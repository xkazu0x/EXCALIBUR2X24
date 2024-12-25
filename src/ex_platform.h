#pragma once

#include "ex_input.h"

#include <string>
#include <cstdint>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <windowsx.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

enum ex_window_mode {
    EX_WINDOW_MODE_WINDOWED = 0,
    EX_WINDOW_MODE_FULLSCREEN = 1,
};

namespace ex::platform {
    class window {
    public:        
        struct create_info {
            std::string title;
            uint32_t width;
            uint32_t height;
            ex_window_mode mode;
            ex::input *pinput;
        };
                
    public:
        void create(window::create_info *create_info);
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
        
        void toggle_fullscreen();
        void change_title(std::string title);
        void set_cursor_pos(uint32_t x, uint32_t y);
        void hide_cursor(bool show);
        
        bool create_vulkan_surface(VkInstance instance, VkAllocationCallbacks *allocator, VkSurfaceKHR *surface);
        
    private:
        static LRESULT process_message_setup(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        static LRESULT process_message_redirect(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

        LRESULT process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

    private:
        struct ex_window_info {
            std::string title;
            uint32_t current_width;
            uint32_t current_height;
            uint32_t windowed_width;
            uint32_t windowed_height;
            uint32_t fullscreen_width;
            uint32_t fullscreen_height;
            int32_t xpos;
            int32_t ypos;
        };
        
        enum ex_window_state {
            EX_WINDOW_STATE_HIDDEN = 0x00,
            EX_WINDOW_STATE_ACTIVE = 0x01,
            EX_WINDOW_STATE_INACTIVE = 0x02,
            EX_WINDOW_STATE_CLOSED = 0x04,
        };
        
    private:
        ex::input *m_pinput;

        ATOM m_atom;
        HWND m_handle;
        HCURSOR m_cursor;
        HINSTANCE m_instance;
        //WINDOWPLACEMENT m_window_placement;
        
        ex_window_info m_window_info;
        ex_window_state m_current_state;
        ex_window_mode m_current_mode;

        bool m_hide_cursor;
    };

    class timer {
    public:
        void init();
        uint64_t get_time();
        uint64_t get_frequency();
        
    private:
        LARGE_INTEGER m_counter_frequency;
    };
}
