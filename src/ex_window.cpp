#include "ex_window.h"
#include "ex_logger.h"

void
ex::window::create(ex::input *input, std::string title, uint32_t width, uint32_t height, bool fullscreen) {
    m_input = input;
    m_info.title = title;
    
    m_instance = GetModuleHandle(nullptr);

    WNDCLASSEXA window_class { };
    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = process_message_setup;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = m_instance;
    window_class.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    window_class.hbrBackground = nullptr;
    window_class.lpszClassName = m_info.title.c_str();
    window_class.hIconSm = nullptr;

    m_atom = RegisterClassExA(&window_class);

    m_info.windowed_width = width;
    m_info.windowed_height = height;
    m_info.fullscreen_width = GetSystemMetrics(SM_CXSCREEN);
    m_info.fullscreen_height = GetSystemMetrics(SM_CYSCREEN);
    
    uint32_t window_ex_style = 0;
    uint32_t window_style = 0;
    
    if (fullscreen) {
        m_info.current_width = m_info.fullscreen_width;
        m_info.current_height = m_info.fullscreen_height;
        m_info.xpos = 0;
        m_info.ypos = 0;
        m_info.fullscreen = true;

        window_ex_style = WS_EX_APPWINDOW;
        window_style = WS_POPUP;
        
        DEVMODE screen_settings = {};
        screen_settings.dmSize = sizeof(screen_settings);
        screen_settings.dmPelsWidth = m_info.current_width;
        screen_settings.dmPelsHeight = m_info.current_height;
        screen_settings.dmBitsPerPel = 32;
        screen_settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        ChangeDisplaySettings(&screen_settings, CDS_FULLSCREEN);
    } else {
        m_info.current_width = m_info.windowed_width;
        m_info.current_height = m_info.windowed_height;
        m_info.xpos = (m_info.fullscreen_width - m_info.current_width) / 2;
        m_info.ypos = (m_info.fullscreen_height - m_info.current_height) / 2;
        m_info.fullscreen = false;
        
        window_ex_style = WS_EX_OVERLAPPEDWINDOW;
        window_style = WS_OVERLAPPEDWINDOW;
        
        RECT border_rect = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);
        
        m_info.current_width += border_rect.right - border_rect.left;
        m_info.current_height += border_rect.bottom - border_rect.top;
        m_info.xpos += border_rect.left;
        m_info.ypos += border_rect.top;
    }

    m_handle = CreateWindowExA(window_ex_style, MAKEINTATOM(m_atom),
                               m_info.title.c_str(), window_style,
                               m_info.xpos, m_info.ypos,
                               m_info.current_width, m_info.current_height,
                               nullptr, nullptr, m_instance, this);
}

void
ex::window::destroy() {
    DestroyWindow(m_handle);
    UnregisterClass(MAKEINTATOM(m_atom), m_instance);
}

void
ex::window::update() {
    MSG msg;
    while (PeekMessage(&msg, m_handle, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void
ex::window::show() {
    ShowWindow(m_handle, SW_SHOW);
    SetForegroundWindow(m_handle);
    SetFocus(m_handle);
    m_state = EX_WINDOW_STATE_ACTIVE;
}

void
ex::window::close() {
    if (m_info.fullscreen) ChangeDisplaySettings(NULL, 0);
    m_state = EX_WINDOW_STATE_CLOSED;
}

void
ex::window::sleep(uint64_t ms) {
    Sleep(ms);
}

bool
ex::window::closed() {
    return m_state == EX_WINDOW_STATE_CLOSED;
}

bool
ex::window::inactive() {
    return m_state == EX_WINDOW_STATE_INACTIVE;
}

uint32_t
ex::window::width() {
    return m_info.current_width;
}

uint32_t
ex::window::height() {
    return m_info.current_height;
}

int8_t
ex::window::get_key(int32_t key_code) {
    return GetAsyncKeyState(key_code);
}

void
ex::window::change_display_mode() {
    uint32_t window_flags = SWP_SHOWWINDOW;
    LONG window_ex_style = GetWindowLong(m_handle, GWL_EXSTYLE);
    LONG window_style = GetWindowLong(m_handle, GWL_STYLE);
    
    if (m_info.fullscreen) {
        window_ex_style = WS_EX_OVERLAPPEDWINDOW;
        window_style = WS_OVERLAPPEDWINDOW;
        SetWindowLong(m_handle, GWL_EXSTYLE, window_ex_style);
        SetWindowLong(m_handle, GWL_STYLE, window_style);

        m_info.current_width = m_info.windowed_width;
        m_info.current_height = m_info.windowed_height;
        m_info.xpos = (m_info.fullscreen_width - m_info.current_width) / 2;
        m_info.ypos = (m_info.fullscreen_height - m_info.current_height) / 2;
        m_info.fullscreen = false;

        RECT border_rect = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);
        
        m_info.current_width += border_rect.right - border_rect.left;
        m_info.current_height += border_rect.bottom - border_rect.top;
        m_info.xpos += border_rect.left;
        m_info.ypos += border_rect.top;
        SetWindowPos(m_handle,
                     0,
                     m_info.xpos,
                     m_info.ypos,
                     m_info.current_width,
                     m_info.current_height,
                     window_flags);
    } else {
        window_ex_style = WS_EX_APPWINDOW;
        window_style = WS_POPUP;
        SetWindowLong(m_handle, GWL_EXSTYLE, window_ex_style);
        SetWindowLong(m_handle, GWL_STYLE, window_style);
        
        m_info.current_width = m_info.fullscreen_width;
        m_info.current_height = m_info.fullscreen_height;
        m_info.xpos = 0;
        m_info.ypos = 0;
        m_info.fullscreen = true;
        SetWindowPos(m_handle,
                     0,
                     m_info.xpos,
                     m_info.ypos,
                     m_info.current_width,
                     m_info.current_height,
                     window_flags);
    }
}

void
ex::window::change_title(std::string title) {
    SetWindowTextA(m_handle, title.c_str());
}

void
ex::window::set_cursor_pos(uint32_t x, uint32_t y) {
    if (m_state == EX_WINDOW_STATE_ACTIVE) {
        POINT point;
        point.x = x;
        point.y = y;
    
        ClientToScreen(m_handle, &point);
        SetCursorPos(point.x, point.y);
    }
}

bool
ex::window::create_vulkan_surface(VkInstance instance,
                                       VkAllocationCallbacks *allocator,
                                       VkSurfaceKHR *surface) {
    VkWin32SurfaceCreateInfoKHR surface_create_info = {};
    surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_create_info.hinstance = m_instance;
    surface_create_info.hwnd = m_handle;
    VkResult result = vkCreateWin32SurfaceKHR(instance,
                                              &surface_create_info,
                                              allocator,
                                              surface);
    if (result != VK_SUCCESS) {
        return false;
    }

    return true;
}

LRESULT
ex::window::process_message_setup(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        auto *create_struct = (CREATESTRUCTA*)lparam;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)create_struct->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)&process_message_redirect);

        return process_message_redirect(hwnd, msg, wparam, lparam);
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

LRESULT
ex::window::process_message_redirect(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto *window = (ex::window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    return window->process_message(hwnd, msg, wparam, lparam);
}

LRESULT
ex::window::process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CLOSE: {
        EXDEBUG("-+WINDOW_CLOSED+-");
        close();
        return 0;
    } break;
    case WM_DESTROY: {
        EXDEBUG("-+WINDOW_DESTROYED+-");
        close();
    } break;
    case WM_SIZE: {
        m_info.current_width = LOWORD(lparam);
        m_info.current_height = HIWORD(lparam);
    } break;
    case WM_ACTIVATE: {
        if (LOWORD(wparam) == WA_INACTIVE) {
            EXDEBUG("-+WINDOW_INACTIVE+-");
            ShowCursor(true);
            m_state = EX_WINDOW_STATE_INACTIVE;
        } else {
            EXDEBUG("-+WINDOW_ACTIVATED+-");
            ShowCursor(false);
            m_state = EX_WINDOW_STATE_ACTIVE;
        }
    } break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        bool pressed = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        uint32_t key_code = wparam;
        m_input->process_key(key_code, pressed);
    } break;
    case WM_MOUSEMOVE: {
        int32_t x = GET_X_LPARAM(lparam);
        int32_t y = GET_Y_LPARAM(lparam);
        m_input->process_mouse_move(x, y);
    } break;
    case WM_MOUSEWHEEL: {
        int32_t z_delta = GET_WHEEL_DELTA_WPARAM(wparam);
        if (z_delta != 0) {
            z_delta= (z_delta < 0) ? -1 : 1;
            m_input->process_mouse_wheel(z_delta);
        }
    } break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP: {
        bool pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
        uint32_t mouse_button = EX_BUTTON_MAX_COUNT;
        switch(msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP: {
            mouse_button = EX_BUTTON_LEFT;
        } break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP: {
            mouse_button = EX_BUTTON_RIGHT;
        } break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: {
            mouse_button = EX_BUTTON_MIDDLE;
        } break;
        }

        m_input->process_button(mouse_button, pressed);
    } break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}
