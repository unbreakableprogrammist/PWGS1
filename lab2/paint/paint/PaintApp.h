#pragma once
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <vector>

class PaintApp {
private:
    HINSTANCE m_instance; // intsancja aplikacji 
    HWND m_main;

    HBRUSH m_bg_brush;
    HBRUSH m_rect_brush;

    std::vector<HWND> m_rectangles;
    bool m_is_drawing;
    POINT m_start_pt;

    static std::wstring const s_class_name;

    bool register_class();
    HWND create_main_window();

    static LRESULT CALLBACK window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

public:
    PaintApp(HINSTANCE instance);
    ~PaintApp();
    int run(int show_command);
};