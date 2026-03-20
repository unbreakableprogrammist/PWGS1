//#include "Window.h"
//#include <commctrl.h> // Required for Trackbar
//#include <string>
//
//// Tell the compiler to link the Common Controls library
//#pragma comment(lib, "comctl32.lib")
//
//const std::wstring Window::s_class_name{ L"GradientSelectorClass" };
//
//// --- CONSTRUCTOR ---
//Window::Window(HINSTANCE instance) : m_hInstance(instance), m_hwnd(nullptr) {
//    if (!register_class()) return;
//
//    m_hwnd = CreateWindowExW(
//        0, s_class_name.c_str(), L"Gradient Selector - Lab Prep",
//        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
//        650, 450, nullptr, nullptr, m_hInstance, this
//    );
//}
//
//Window::~Window() {}
//
//// --- REGISTER CLASS ---
//bool Window::register_class() {
//    WNDCLASSEXW desc{};
//    if (GetClassInfoExW(m_hInstance, s_class_name.c_str(), &desc) != 0) return true;
//
//    desc.cbSize = sizeof(WNDCLASSEXW);
//    desc.lpfnWndProc = window_proc_static;
//    desc.hInstance = m_hInstance;
//    desc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
//    desc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
//    desc.lpszClassName = s_class_name.c_str();
//
//    return RegisterClassExW(&desc) != 0;
//}
//
//// --- MESSAGE LOOP ---
//int Window::run(int show_command) {
//    if (!m_hwnd) return EXIT_FAILURE;
//    ShowWindow(m_hwnd, show_command);
//
//    MSG msg{};
//    BOOL result;
//    while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
//        if (result == -1) return EXIT_FAILURE;
//        TranslateMessage(&msg);
//        DispatchMessageW(&msg);
//    }
//    return static_cast<int>(msg.wParam);
//}
//
//// --- STATIC WINDOW PROC ---
//LRESULT CALLBACK Window::window_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
//    Window* app = nullptr;
//    if (message == WM_NCCREATE) {
//        auto p = reinterpret_cast<LPCREATESTRUCTW>(lParam);
//        app = static_cast<Window*>(p->lpCreateParams);
//        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
//    }
//    else {
//        app = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
//    }
//
//    if (app != nullptr) return app->window_proc(window, message, wParam, lParam);
//    return DefWindowProcW(window, message, wParam, lParam);
//}
//
//// --- MAIN WINDOW PROC ---
//LRESULT Window::window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
//    switch (message) {
//
//        // --- CREATE INTERFACE ---
//    case WM_CREATE: {
//
//        // 1. GROUPBOX (Left Panel)
//        CreateWindowExW(0, L"BUTTON", L"Gradient Settings", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
//            10, 10, 250, 360, window, (HMENU)200, m_hInstance, nullptr);
//
//        // 2. RADIO BUTTONS (Type Selection)
//        CreateWindowExW(0, L"BUTTON", L"Linear Gradient", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
//            30, 40, 200, 20, window, (HMENU)201, m_hInstance, nullptr);
//
//        CreateWindowExW(0, L"BUTTON", L"Radial Gradient", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
//            30, 70, 200, 20, window, (HMENU)202, m_hInstance, nullptr);
//
//        // Select the first radio button by default
//        CheckRadioButton(window, 201, 202, 201);
//
//        // 3. CHECKBOX (Extra Option)
//        CreateWindowExW(0, L"BUTTON", L"Invert Colors", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
//            30, 110, 200, 20, window, (HMENU)203, m_hInstance, nullptr);
//
//        // 4. SLIDER WITH LABEL
//        CreateWindowExW(0, L"STATIC", L"Effect Strength:", WS_VISIBLE | WS_CHILD,
//            30, 150, 200, 20, window, (HMENU)204, m_hInstance, nullptr);
//
//        HWND hSlider = CreateWindowExW(0, TRACKBAR_CLASS, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | TBS_HORZ,
//            25, 170, 210, 40, window, (HMENU)205, m_hInstance, nullptr);
//
//        // Set slider range 0-100 and default position to 50
//        SendMessageW(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
//        SendMessageW(hSlider, TBM_SETPOS, TRUE, 50);
//
//        // 5. MAIN BUTTON
//        CreateWindowExW(0, L"BUTTON", L"Draw Gradient!", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//            50, 250, 150, 50, window, (HMENU)206, m_hInstance, nullptr);
//
//        // 6. DRAWING AREA (Right Panel)
//        CreateWindowExW(0, L"STATIC", L"Your gradient will appear here...", WS_VISIBLE | WS_CHILD | WS_BORDER | SS_CENTER | SS_CENTERIMAGE,
//            280, 17, 330, 353, window, (HMENU)207, m_hInstance, nullptr);
//
//        return 0;
//    }
//
//                  // --- HANDLE CLICKS ---
//    case WM_COMMAND: {
//        int clicked_id = LOWORD(wParam);
//
//        // If "Draw Gradient!" button is clicked
//        if (clicked_id == 206) {
//
//            // 1. Check which Radio Button is selected
//            bool is_linear = (IsDlgButtonChecked(window, 201) == BST_CHECKED);
//
//            // 2. Check if Checkbox is selected
//            bool invert_colors = (IsDlgButtonChecked(window, 203) == BST_CHECKED);
//
//            // 3. Get slider position
//            HWND hSlider = GetDlgItem(window, 205);
//            LRESULT slider_pos = SendMessageW(hSlider, TBM_GETPOS, 0, 0);
//
//            // 4. Build info text
//            std::wstring info = L"Collected settings:\n\n";
//            info += L"Type: " + std::wstring(is_linear ? L"Linear" : L"Radial") + L"\n";
//            info += L"Invert Colors: " + std::wstring(invert_colors ? L"YES" : L"NO") + L"\n";
//            info += L"Effect Strength: " + std::to_wstring(slider_pos) + L"%\n\n";
//            info += L"Ready to draw!";
//
//            // 5. Show collected data
//            MessageBoxW(window, info.c_str(), L"Interface Data", MB_OK | MB_ICONINFORMATION);
//        }
//        return 0;
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
//
//    return DefWindowProcW(window, message, wParam, lParam);
//}