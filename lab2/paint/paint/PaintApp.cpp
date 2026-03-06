//#include "PaintApp.h"
//#include <algorithm>
//#include <cmath>
//
//std::wstring const PaintApp::s_class_name{ L"Not_WM_PAINT_Class" };
//
//PaintApp::PaintApp(HINSTANCE instance)
//    : m_instance{ instance }, m_main{}, m_is_drawing{ false }, m_start_pt{ 0, 0 }
//{
//    m_bg_brush = CreateSolidBrush(RGB(30, 50, 90));
//    m_rect_brush = CreateSolidBrush(RGB(170, 70, 80));
//
//    register_class();
//    m_main = create_main_window();
//}
//
//PaintApp::~PaintApp() {
//    DeleteObject(m_bg_brush);
//    DeleteObject(m_rect_brush);
//}
//
//bool PaintApp::register_class() {
//    WNDCLASSEXW desc{};
//    if (GetClassInfoExW(m_instance, s_class_name.c_str(), &desc) != 0) return true;
//
//    desc = {
//        .cbSize = sizeof(WNDCLASSEXW),
//        .lpfnWndProc = window_proc_static,
//        .hInstance = m_instance,
//        .hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW),
//        .hbrBackground = m_bg_brush,
//        .lpszClassName = s_class_name.c_str()
//    };
//    return RegisterClassExW(&desc) != 0;
//}
//
//HWND PaintApp::create_main_window() {
//    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
//
//    RECT size{ 0, 0, 800, 600 };
//    AdjustWindowRectEx(&size, style, false, 0);
//
//    return CreateWindowExW(
//        0, s_class_name.c_str(), L"Not WM_PAINT",
//        style,
//        CW_USEDEFAULT, CW_USEDEFAULT,
//        size.right - size.left, size.bottom - size.top,
//        nullptr, nullptr, m_instance, this
//    );
//}
//
//LRESULT CALLBACK PaintApp::window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
//    PaintApp* app = nullptr;
//    if (message == WM_NCCREATE) {
//        auto p = reinterpret_cast<LPCREATESTRUCTW>(lparam);
//        app = static_cast<PaintApp*>(p->lpCreateParams);
//        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
//    }
//    else {
//        app = reinterpret_cast<PaintApp*>(GetWindowLongPtrW(window, GWLP_USERDATA));
//    }
//
//    if (app != nullptr) {
//        return app->window_proc(window, message, wparam, lparam);
//    }
//    return DefWindowProcW(window, message, wparam, lparam);
//}
//
//LRESULT PaintApp::window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
//    switch (message) {
//    case WM_LBUTTONDOWN: {
//        m_is_drawing = true;
//        m_start_pt.x = GET_X_LPARAM(lparam);
//        m_start_pt.y = GET_Y_LPARAM(lparam);
//
//        SetCapture(window);
//
//        HWND new_rect = CreateWindowExW(
//            0, L"STATIC", nullptr,
//            WS_CHILD | WS_VISIBLE,
//            m_start_pt.x, m_start_pt.y, 0, 0,
//            window, nullptr, m_instance, nullptr
//        );
//        m_rectangles.push_back(new_rect);
//        return 0;
//    }
//
//    case WM_MOUSEMOVE: {
//        if (m_is_drawing && !m_rectangles.empty()) {
//            int cur_x = GET_X_LPARAM(lparam);
//            int cur_y = GET_Y_LPARAM(lparam);
//
//            int x = std::min((int)m_start_pt.x, cur_x);
//            int y = std::min((int)m_start_pt.y, cur_y);
//            int width = std::abs((int)m_start_pt.x - cur_x);
//            int height = std::abs((int)m_start_pt.y - cur_y);
//
//            HWND active_rect = m_rectangles.back();
//            SetWindowPos(active_rect, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
//        }
//        return 0;
//    }
//
//    case WM_LBUTTONUP: {
//        if (m_is_drawing) {
//            m_is_drawing = false;
//            ReleaseCapture();
//        }
//        return 0;
//    }
//
//    case WM_KEYDOWN: {
//        if (wparam == VK_BACK) {
//            if (!m_rectangles.empty()) {
//                HWND last_rect = m_rectangles.back();
//                DestroyWindow(last_rect);
//                m_rectangles.pop_back();
//
//                if (m_is_drawing) {
//                    m_is_drawing = false;
//                    ReleaseCapture();
//                }
//
//                InvalidateRect(window, nullptr, TRUE);
//            }
//        }
//        return 0;
//    }
//
//    case WM_CTLCOLORSTATIC: {
//        return reinterpret_cast<INT_PTR>(m_rect_brush);
//    }
//
//    case WM_CLOSE:
//        DestroyWindow(window);
//        return 0;
//
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        return 0;
//    }
//    return DefWindowProcW(window, message, wparam, lparam);
//}
//
//int PaintApp::run(int show_command) {
//    ShowWindow(m_main, show_command);
//    MSG msg{};
//    BOOL result = TRUE;
//    while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
//        if (result == -1) return EXIT_FAILURE;
//        TranslateMessage(&msg);
//        DispatchMessageW(&msg);
//    }
//    return EXIT_SUCCESS;
//}