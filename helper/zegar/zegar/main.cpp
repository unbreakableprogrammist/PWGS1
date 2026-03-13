#include <windows.h>

#include <string>

#include <cmath>



constexpr double PI = 3.14159265358979323846;



class app_dial {

private:

    HINSTANCE m_instance;

    HWND m_main;

    HWND m_children[10];

    HWND m_display;

    std::wstring m_dialed_number;



    double m_base_angle = 0.0;



    bool m_is_dragging = false;

    double m_start_mouse_angle = 0.0;

    double m_start_dial_angle = 0.0;



    int m_center_x = 300;

    int m_center_y = 350;

    int m_radius = 150;



    HBRUSH m_bg_brush;

    HBRUSH m_hole_brush;



    static const std::wstring s_class_name;



    static LRESULT CALLBACK window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {

        app_dial* app = nullptr;

        if (message == WM_NCCREATE) {

            auto p = reinterpret_cast<LPCREATESTRUCTW>(lparam);

            app = static_cast<app_dial*>(p->lpCreateParams);

            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app)); // [cite: 146-147]

        }

        else {

            app = reinterpret_cast<app_dial*>(GetWindowLongPtrW(window, GWLP_USERDATA)); // [cite: 153-154]

        }



        if (app != nullptr) {

            return app->window_proc(window, message, wparam, lparam); // [cite: 157-158]

        }

        return DefWindowProcW(window, message, wparam, lparam); // [cite: 160]

    }



    // ROZWIĄZANIE PROBLEMU MIGANIA: Grupowe przesuwanie okienek

    void update_children_positions() {

        // Zaczynamy transakcję przesuwania 10 elementów

        HDWP hdwp = BeginDeferWindowPos(10);



        for (int i = 0; i < 10; ++i) {

            double angle = m_base_angle + (i * 2 * PI) / 10.0;

            int x = m_center_x + (int)(m_radius * cos(angle)) - 20;

            int y = m_center_y + (int)(m_radius * sin(angle)) - 20;



            // Zamiast SetWindowPos[cite: 407], dodajemy nowy układ do kolejki

            hdwp = DeferWindowPos(hdwp, m_children[i], nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        }



        // System przesuwa wszystko NARAZ i odrysowuje tło bez migania

        EndDeferWindowPos(hdwp);

    }



    LRESULT window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {

        switch (message) {

        case WM_CTLCOLORSTATIC: {

            HDC hdc = reinterpret_cast<HDC>(wparam);

            HWND hCtl = (HWND)lparam;



            SetBkMode(hdc, TRANSPARENT);



            if (hCtl == m_display) {

                SetTextColor(hdc, RGB(247, 93, 60));

                return reinterpret_cast<INT_PTR>(m_bg_brush);

            }



            SetTextColor(hdc, RGB(255, 255, 255));

            return reinterpret_cast<INT_PTR>(m_hole_brush); // [cite: 403]

        }



        case WM_LBUTTONDOWN: {

            int mouse_x = (short)LOWORD(lparam);

            int mouse_y = (short)HIWORD(lparam);



            m_start_mouse_angle = atan2(mouse_y - m_center_y, mouse_x - m_center_x);

            m_start_dial_angle = m_base_angle;



            m_is_dragging = true;

            SetCapture(window);

            return 0;

        }



        case WM_MOUSEMOVE: {

            if (m_is_dragging) {

                int mouse_x = (short)LOWORD(lparam);

                int mouse_y = (short)HIWORD(lparam);



                double current_mouse_angle = atan2(mouse_y - m_center_y, mouse_x - m_center_x);

                double diff = current_mouse_angle - m_start_mouse_angle;



                while (diff > PI)  diff -= 2 * PI;

                while (diff < -PI) diff += 2 * PI;



                m_base_angle = m_start_dial_angle + diff;



                // USUNĄŁEM InvalidateRect! update_children_positions samo załatwi tło!

                update_children_positions();

            }

            return 0;

        }



        case WM_LBUTTONUP: {

            if (m_is_dragging) {

                m_is_dragging = false;

                ReleaseCapture();



                for (int i = 0; i < 10; ++i) {

                    double angle = m_base_angle + (i * 2 * PI) / 10.0;

                    double normalized_angle = atan2(sin(angle), cos(angle));

                    double top_angle = -PI / 2.0;



                    if (abs(normalized_angle - top_angle) < 0.3) {

                        m_dialed_number += std::to_wstring(i);

                        SetWindowTextW(m_display, m_dialed_number.c_str());

                        break;

                    }

                }

            }

            return 0;

        }



        case WM_DESTROY: {

            // Zwalnianie zasobów GDI! [cite: 20-21, 847]

            DeleteObject(m_bg_brush);

            DeleteObject(m_hole_brush);

            PostQuitMessage(EXIT_SUCCESS); // [cite: 176]

            return 0;

        }

        }

        return DefWindowProcW(window, message, wparam, lparam); // [cite: 178]

    }



    bool register_class() {

        WNDCLASSEXW desc{};

        if (GetClassInfoExW(m_instance, s_class_name.c_str(), &desc) != 0) return true; // [cite: 105-106]



        desc.cbSize = sizeof(WNDCLASSEXW);

        desc.lpfnWndProc = window_proc_static; // [cite: 108]

        desc.hInstance = m_instance; // [cite: 109]

        desc.hCursor = LoadCursorW(nullptr, L"IDC_ARROW"); // [cite: 110]

        desc.hbrBackground = m_bg_brush;

        desc.lpszClassName = s_class_name.c_str(); // [cite: 111]



        return RegisterClassExW(&desc) != 0; // [cite: 112]

    }



    HWND create_window() {

        HWND window = CreateWindowExW(

            0,

            s_class_name.c_str(), L"Retro Tarcza",

            WS_OVERLAPPEDWINDOW,

            CW_USEDEFAULT, 0, 600, 650,

            nullptr, nullptr, m_instance, this // [cite: 128]

        );



        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);



        m_display = CreateWindowExW(

            0, L"STATIC", L"Przeciagnij cyfre na sama gore...",

            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,

            100, 50, 400, 40,

            window, nullptr, m_instance, nullptr // [cite: 352-355]

        );

        SendMessage(m_display, WM_SETFONT, (WPARAM)hFont, TRUE);



        for (int i = 0; i < 10; ++i) {

            std::wstring text = std::to_wstring(i);



            m_children[i] = CreateWindowExW(

                0, L"STATIC", text.c_str(),

                WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,

                0, 0, 40, 40,

                window, nullptr, m_instance, nullptr

            );



            SendMessage(m_children[i], WM_SETFONT, (WPARAM)hFont, TRUE);



            HRGN hRgn = CreateEllipticRgn(0, 0, 40, 40);

            SetWindowRgn(m_children[i], hRgn, TRUE);

        }



        update_children_positions();



        return window;

    }



public:

    app_dial(HINSTANCE instance) : m_instance{ instance }, m_main{} {

        // [cite: 388-390, 863]

        m_bg_brush = CreateSolidBrush(RGB(40, 44, 52));

        m_hole_brush = CreateSolidBrush(RGB(247, 93, 60));



        register_class(); // [cite: 183]

        m_main = create_window(); // [cite: 187]

    }



    int run(int show_command) {

        ShowWindow(m_main, show_command); // [cite: 191]

        MSG msg{};

        BOOL result = TRUE;

        while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) { // [cite: 194]

            if (result == -1) return EXIT_FAILURE; // [cite: 196-199]

            TranslateMessage(&msg); // [cite: 200]

            DispatchMessageW(&msg); // [cite: 201]

        }

        return EXIT_SUCCESS; // [cite: 202]

    }

};



const std::wstring app_dial::s_class_name{ L"DialAppClass" }; // [cite: 96]



int WINAPI wWinMain(HINSTANCE instance, HINSTANCE /*prevInstance*/, LPWSTR /*command_line*/, int show_command) {

    app_dial app{ instance }; // [cite: 205]

    return app.run(show_command);

}