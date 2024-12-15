#include "ex_platform.h"
#include "ex_logger.h"

void
ex::platform::window::create(ex::platform::window::create_info *create_info) {
    m_window_info.title = create_info->title;
    m_pinput = create_info->pinput;

    m_cursor = LoadCursorA(nullptr, IDC_ARROW);    
    m_instance = GetModuleHandle(nullptr);
    // m_window_placement = {};
    // m_window_placement.length = sizeof(m_window_placement);

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
    wc.lpfnWndProc = process_message_setup;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_instance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    //wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hCursor = nullptr;
    wc.hbrBackground = nullptr;
    wc.lpszClassName = m_window_info.title.c_str();
    wc.hIconSm = nullptr;

    m_atom = RegisterClassExA(&wc);

    m_window_info.windowed_width = create_info->width;
    m_window_info.windowed_height = create_info->height;
    m_window_info.fullscreen_width = GetSystemMetrics(SM_CXSCREEN);
    m_window_info.fullscreen_height = GetSystemMetrics(SM_CYSCREEN);
    
    uint32_t window_ex_style = 0;
    uint32_t window_style = 0;
    
    if (create_info->mode == FULLSCREEN) {
        m_window_info.current_width = m_window_info.fullscreen_width;
        m_window_info.current_height = m_window_info.fullscreen_height;
        m_window_info.xpos = 0;
        m_window_info.ypos = 0;
        m_current_mode = create_info->mode;

        window_ex_style = WS_EX_APPWINDOW;
        window_style = WS_POPUP;
        
        DEVMODE screen_settings = {};
        screen_settings.dmSize = sizeof(screen_settings);
        screen_settings.dmPelsWidth = m_window_info.current_width;
        screen_settings.dmPelsHeight = m_window_info.current_height;
        screen_settings.dmBitsPerPel = 32;
        screen_settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        ChangeDisplaySettings(&screen_settings, CDS_FULLSCREEN);
    } else if (create_info->mode == WINDOWED) {
        m_window_info.current_width = m_window_info.windowed_width;
        m_window_info.current_height = m_window_info.windowed_height;
        m_window_info.xpos = (m_window_info.fullscreen_width - m_window_info.current_width) / 2;
        m_window_info.ypos = (m_window_info.fullscreen_height - m_window_info.current_height) / 2;
        m_current_mode = create_info->mode;
        
        window_ex_style = WS_EX_OVERLAPPEDWINDOW;
        window_style = WS_OVERLAPPEDWINDOW;
        
        RECT border_rect = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);
        
        m_window_info.current_width += border_rect.right - border_rect.left;
        m_window_info.current_height += border_rect.bottom - border_rect.top;
        m_window_info.xpos += border_rect.left;
        m_window_info.ypos += border_rect.top;
    }

    m_handle = CreateWindowExA(window_ex_style, MAKEINTATOM(m_atom),
                               m_window_info.title.c_str(), window_style,
                               m_window_info.xpos, m_window_info.ypos,
                               m_window_info.current_width, m_window_info.current_height,
                               nullptr, nullptr, m_instance, this);

    m_hide_cursor = false;
}

void
ex::platform::window::destroy() {
    DestroyWindow(m_handle);
    UnregisterClass(MAKEINTATOM(m_atom), m_instance);
}

void
ex::platform::window::update() {
    MSG msg;
    while (PeekMessage(&msg, m_handle, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (IsIconic(m_handle)) m_current_state = EX_WINDOW_STATE_INACTIVE;
}

void
ex::platform::window::show() {
    ShowWindow(m_handle, SW_SHOW);
    SetForegroundWindow(m_handle);
    SetFocus(m_handle);
    m_current_state = EX_WINDOW_STATE_ACTIVE;
}

void
ex::platform::window::close() {
    if (m_current_mode == FULLSCREEN) ChangeDisplaySettings(NULL, 0);
    m_current_state = EX_WINDOW_STATE_CLOSED;
}

void
ex::platform::window::sleep(uint64_t ms) {
    Sleep(ms);
}

bool
ex::platform::window::closed() {
    return m_current_state == EX_WINDOW_STATE_CLOSED;
}

bool
ex::platform::window::inactive() {
    return m_current_state == EX_WINDOW_STATE_INACTIVE;
}

uint32_t
ex::platform::window::width() {
    return m_window_info.current_width;
}

uint32_t
ex::platform::window::height() {
    return m_window_info.current_height;
}

int8_t
ex::platform::window::get_key(int32_t key_code) {
    return GetAsyncKeyState(key_code);
}

void
ex::platform::window::toggle_fullscreen() {
    LONG window_ex_style = GetWindowLong(m_handle, GWL_EXSTYLE);
    LONG window_style = GetWindowLong(m_handle, GWL_STYLE);
    
    // if (m_current_mode == WINDOWED) {
    //     m_current_mode = FULLSCREEN;
    //     MONITORINFO mi = { sizeof(mi) };
    //     if (GetWindowPlacement(m_handle, &m_window_placement) &&
    //         GetMonitorInfo(MonitorFromWindow(m_handle, MONITOR_DEFAULTTOPRIMARY), &mi)) {
    //         SetWindowLong(m_handle, GWL_STYLE, ((window_style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP));
    //         SetWindowPos(m_handle, HWND_TOP,
    //                      mi.rcMonitor.left, mi.rcMonitor.top,
    //                      mi.rcMonitor.right - mi.rcMonitor.left,
    //                      mi.rcMonitor.bottom - mi.rcMonitor.top,
    //                      SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    //     }
    // } else if (m_current_mode == FULLSCREEN) {
    //     m_current_mode = WINDOWED;
    //     SetWindowLong(m_handle, GWL_STYLE, window_style | WS_OVERLAPPEDWINDOW);
    //     SetWindowPlacement(m_handle, &m_window_placement);
    //     SetWindowPos(m_handle, 0, 0, 0, 0, 0,
    //                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
    //                  SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    // }

    if (m_current_mode == WINDOWED) {            
        window_ex_style = WS_EX_APPWINDOW;
        window_style &= ~WS_OVERLAPPEDWINDOW;
        window_style |= WS_POPUP;
        
        SetWindowLong(m_handle, GWL_EXSTYLE, window_ex_style);
        SetWindowLong(m_handle, GWL_STYLE, window_style);
        
        m_window_info.current_width = m_window_info.fullscreen_width;
        m_window_info.current_height = m_window_info.fullscreen_height;
        m_window_info.xpos = 0;
        m_window_info.ypos = 0;
        m_current_mode = FULLSCREEN;

        SetWindowPos(m_handle, HWND_TOP,
                     m_window_info.xpos,
                     m_window_info.ypos,
                     m_window_info.current_width,
                     m_window_info.current_height,
                     SWP_SHOWWINDOW);
    } else if (m_current_mode == FULLSCREEN) {
        window_ex_style = WS_EX_OVERLAPPEDWINDOW;
        window_style &= ~WS_POPUP;
        window_style |= WS_OVERLAPPEDWINDOW;
        SetWindowLong(m_handle, GWL_EXSTYLE, window_ex_style);
        SetWindowLong(m_handle, GWL_STYLE, window_style);
        
        m_window_info.current_width = m_window_info.windowed_width;
        m_window_info.current_height = m_window_info.windowed_height;
        m_window_info.xpos = (m_window_info.fullscreen_width - m_window_info.current_width) / 2;
        m_window_info.ypos = (m_window_info.fullscreen_height - m_window_info.current_height) / 2;
        m_current_mode = WINDOWED;
        
        RECT border_rect = { 0, 0, 0, 0 };
        AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);
        
        m_window_info.current_width += border_rect.right - border_rect.left;
        m_window_info.current_height += border_rect.bottom - border_rect.top;
        m_window_info.xpos += border_rect.left;
        m_window_info.ypos += border_rect.top;

        SetWindowPos(m_handle, 0,
                     m_window_info.xpos,
                     m_window_info.ypos,
                     m_window_info.current_width,
                     m_window_info.current_height,
                     SWP_SHOWWINDOW);
    }
}

void
ex::platform::window::change_title(std::string title) {
    SetWindowTextA(m_handle, title.c_str());
    m_window_info.title = title;
}

void
ex::platform::window::set_cursor_pos(uint32_t x, uint32_t y) {
    if (m_current_state == EX_WINDOW_STATE_ACTIVE) {
        POINT point;
        point.x = x;
        point.y = y;
    
        ClientToScreen(m_handle, &point);
        SetCursorPos(point.x, point.y);
    }
}

void
ex::platform::window::hide_cursor(bool hide) {
    m_hide_cursor = hide;
}

bool
ex::platform::window::create_vulkan_surface(VkInstance instance,
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
ex::platform::window::process_message_setup(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        auto *create_struct = (CREATESTRUCTA*)lparam;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)create_struct->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)&process_message_redirect);

        return process_message_redirect(hwnd, msg, wparam, lparam);
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

LRESULT
ex::platform::window::process_message_redirect(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto *window = (ex::platform::window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    return window->process_message(hwnd, msg, wparam, lparam);
}

LRESULT
ex::platform::window::process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
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
        m_window_info.current_width = LOWORD(lparam);
        m_window_info.current_height = HIWORD(lparam);
    } break;
    case WM_SETCURSOR: {
        if (m_hide_cursor) SetCursor(0);
        else SetCursor(m_cursor);
    } break;
    case WM_ACTIVATE: {
        if (LOWORD(wparam) == WA_INACTIVE) {
            EXDEBUG("-+WINDOW_INACTIVE+-");
            m_current_state = EX_WINDOW_STATE_INACTIVE;
        } else {
            EXDEBUG("-+WINDOW_ACTIVATED+-");
            m_current_state = EX_WINDOW_STATE_ACTIVE;
        }
    } break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        bool pressed = msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN;
        uint32_t key_code = wparam;
        m_pinput->process_key(key_code, pressed);
    } break;
    case WM_MOUSEMOVE: {
        int32_t x = GET_X_LPARAM(lparam);
        int32_t y = GET_Y_LPARAM(lparam);
        m_pinput->process_mouse_move(x, y);
    } break;
    case WM_MOUSEWHEEL: {
        int32_t z_delta = GET_WHEEL_DELTA_WPARAM(wparam);
        if (z_delta != 0) {
            z_delta= (z_delta < 0) ? -1 : 1;
            m_pinput->process_mouse_wheel(z_delta);
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
        
        m_pinput->process_button(mouse_button, pressed);
    } break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}
