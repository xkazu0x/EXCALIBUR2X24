#include "ex_window.h"

void ex::window::create(std::string title, uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    m_title = title;
    
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
    window_class.lpszClassName = m_title.c_str();
    window_class.hIconSm = nullptr;

    m_atom = RegisterClassExA(&window_class);

    int32_t xpos = (GetSystemMetrics(SM_CXSCREEN) - m_width) / 2;
    int32_t ypos = (GetSystemMetrics(SM_CYSCREEN) - m_height) / 2;
    
    uint32_t window_ex_style = WS_EX_OVERLAPPEDWINDOW;
    uint32_t window_style = WS_OVERLAPPEDWINDOW;

    RECT border_rect = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    xpos += border_rect.left;
    ypos += border_rect.top;
    
    m_width += border_rect.right - border_rect.left;
    m_height += border_rect.bottom - border_rect.top;

    m_handle = CreateWindowExA(window_ex_style, MAKEINTATOM(m_atom),
                               m_title.c_str(), window_style,
                               xpos, ypos, m_width, m_height,
                               nullptr, nullptr, m_instance, this);
}

void ex::window::destroy() {
    DestroyWindow(m_handle);
    UnregisterClass(MAKEINTATOM(m_atom), m_instance);
}

void ex::window::update() {
    MSG msg;
    while (PeekMessage(&msg, m_handle, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void ex::window::show() {
    ShowWindow(m_handle, SW_SHOW);
    m_state = EX_WINDOW_STATE_ACTIVE;
}

void ex::window::close() {
    m_state = EX_WINDOW_STATE_CLOSED;
}

bool ex::window::is_active() {
    return m_state == EX_WINDOW_STATE_ACTIVE;
}

LRESULT ex::window::process_message_setup(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_NCCREATE) {
        auto *create_struct = (CREATESTRUCTA*)lparam;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)create_struct->lpCreateParams);
        SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)&process_message_redirect);

        return process_message_redirect(hwnd, msg, wparam, lparam);
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

LRESULT ex::window::process_message_redirect(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    auto *window = (ex::window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
    return window->process_message(hwnd, msg, wparam, lparam);
}

LRESULT ex::window::process_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CLOSE: {
        m_state = EX_WINDOW_STATE_CLOSED;
        return 0;
    } break;
    case WM_SIZE: {
        if (LOWORD(lparam) >= 64 && HIWORD(lparam) >= 64) {
            m_width = LOWORD(lparam);
            m_height = HIWORD(lparam);
        }
    } break;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}
