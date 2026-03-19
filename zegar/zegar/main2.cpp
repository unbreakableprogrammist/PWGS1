//#include <windows.h>
//#include <string>
//
//class app_spring {
//private:
//    HINSTANCE m_instance;
//    HWND m_main;
//    HWND m_children[10];
//
//    // Przechowujemy dokładne pozycje okienek, aby ruch sprężyny był płynny
//    double m_x[10];
//    double m_y[10];
//
//    HBRUSH m_bg_brush;
//    HBRUSH m_hole_brush;
//
//    static const std::wstring s_class_name;
//
//    static LRESULT CALLBACK window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
//        app_spring* app = nullptr;
//        if (message == WM_NCCREATE) {
//            auto p = reinterpret_cast<LPCREATESTRUCTW>(lparam);
//            app = static_cast<app_spring*>(p->lpCreateParams);
//            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app)); // [cite: 146-147]
//        }
//        else {
//            app = reinterpret_cast<app_spring*>(GetWindowLongPtrW(window, GWLP_USERDATA)); // [cite: 153-154]
//        }
//
//        if (app != nullptr) {
//            return app->window_proc(window, message, wparam, lparam); // [cite: 157-158]
//        }
//        return DefWindowProcW(window, message, wparam, lparam); // [cite: 160]
//    }
//
//    LRESULT window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
//        switch (message) {
//        case WM_CTLCOLORSTATIC: {
//            // Kolorowanie kontrolek STATIC [cite: 379-380, 401]
//            HDC hdc = reinterpret_cast<HDC>(wparam);
//            SetBkMode(hdc, TRANSPARENT);
//            SetTextColor(hdc, RGB(255, 255, 255));
//            return reinterpret_cast<INT_PTR>(m_hole_brush); // [cite: 403]
//        }
//
//        case WM_TIMER: {
//            // Obsługa timera odpowiadająca za animację 
//            POINT pt;
//            GetCursorPos(&pt);
//            ScreenToClient(window, &pt);
//
//            // 1. Pierwsze okienko podąża za kursosem (0.1 to sztywność sprężyny)
//            // Odejmujemy 20, aby kursor był na środku okienka 40x40
//            m_x[0] += (pt.x - 20 - m_x[0]) * 0.1;
//            m_y[0] += (pt.y - 20 - m_y[0]) * 0.1;
//            SetWindowPos(m_children[0], nullptr, (int)m_x[0], (int)m_y[0], 0, 0, SWP_NOSIZE | SWP_NOZORDER); // [cite: 406-407, 477-478]
//
//            // 2. Pozostałe okienka podążają za swoim poprzednikiem
//            for (int i = 1; i < 10; ++i) {
//                m_x[i] += (m_x[i - 1] - m_x[i]) * 0.3; // 0.3 to sztywność dla ogona (szybsza reakcja)
//                m_y[i] += (m_y[i - 1] - m_y[i]) * 0.3;
//
//                SetWindowPos(m_children[i], nullptr, (int)m_x[i], (int)m_y[i], 0, 0, SWP_NOSIZE | SWP_NOZORDER); // [cite: 477-478]
//            }
//            return 0;
//        }
//
//        case WM_DESTROY: {
//            // Zwolnienie zasobów GDI [cite: 20-21, 847]
//            DeleteObject(m_bg_brush);
//            DeleteObject(m_hole_brush);
//            PostQuitMessage(EXIT_SUCCESS); // [cite: 176]
//            return 0;
//        }
//        }
//        return DefWindowProcW(window, message, wparam, lparam); // [cite: 178]
//    }
//
//    bool register_class() {
//        WNDCLASSEXW desc{};
//        if (GetClassInfoExW(m_instance, s_class_name.c_str(), &desc) != 0) return true; // [cite: 105-106]
//
//        desc.cbSize = sizeof(WNDCLASSEXW);
//        desc.lpfnWndProc = window_proc_static; // [cite: 108]
//        desc.hInstance = m_instance; // [cite: 109]
//        desc.hCursor = LoadCursorW(nullptr, L"IDC_ARROW"); // [cite: 110]
//        desc.hbrBackground = m_bg_brush;
//        desc.lpszClassName = s_class_name.c_str(); // [cite: 111]
//
//        return RegisterClassExW(&desc) != 0; // [cite: 112]
//    }
//
//    HWND create_window() {
//        // ZMIANA TUTAJ: Dodajemy flagi WS_EX_COMPOSITED oraz WS_CLIPCHILDREN
//        HWND window = CreateWindowExW(
//            WS_EX_COMPOSITED, // Włącza systemowe podwójne buforowanie dla całego okna
//            s_class_name.c_str(), L"Sprezynka",
//            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, // WS_CLIPCHILDREN zapobiega zamalowywaniu dzieci przez tło
//            CW_USEDEFAULT, 0, 800, 600,
//            nullptr, nullptr, m_instance, this // [cite: 120-128]
//        );
//
//        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
//
//        for (int i = 0; i < 10; ++i) {
//            // Inicjalizacja pozycji na środku (tymczasowo)
//            m_x[i] = 400.0;
//            m_y[i] = 300.0;
//
//            std::wstring text = std::to_wstring(i);
//
//            m_children[i] = CreateWindowExW(
//                0, L"STATIC", text.c_str(),
//                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, // [cite: 346]
//                (int)m_x[i], (int)m_y[i], 40, 40,
//                window, nullptr, m_instance, nullptr // [cite: 352-355]
//            );
//
//            SendMessage(m_children[i], WM_SETFONT, (WPARAM)hFont, TRUE);
//        }
//
//        // Uruchamiamy timer z identyfikatorem 1, odświeżanie co 16ms 
//        SetTimer(window, 1, 16, nullptr);
//
//        return window;
//    }
//
//public:
//    app_spring(HINSTANCE instance) : m_instance{ instance }, m_main{} {
//        // [cite: 388-390, 863]
//        m_bg_brush = CreateSolidBrush(RGB(40, 44, 52));
//        m_hole_brush = CreateSolidBrush(RGB(247, 93, 60));
//
//        register_class(); // [cite: 183]
//        m_main = create_window(); // [cite: 187]
//    }
//
//    int run(int show_command) {
//        ShowWindow(m_main, show_command); // [cite: 191]
//        MSG msg{};
//        BOOL result = TRUE;
//        while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) { // [cite: 194]
//            if (result == -1) return EXIT_FAILURE; // [cite: 196-199]
//            TranslateMessage(&msg); // [cite: 200]
//            DispatchMessageW(&msg); // [cite: 201]
//        }
//        return EXIT_SUCCESS; // [cite: 202]
//    }
//};
//
//const std::wstring app_spring::s_class_name{ L"SpringAppClass" }; // [cite: 96]
//
//int WINAPI wWinMain(HINSTANCE instance, HINSTANCE /*prevInstance*/, LPWSTR /*command_line*/, int show_command) {
//    app_spring app{ instance }; // [cite: 205]
//    return app.run(show_command);
//}