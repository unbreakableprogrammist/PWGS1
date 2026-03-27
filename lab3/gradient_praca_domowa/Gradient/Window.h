#pragma once
#include <windows.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

// identyfikatory komend po to zeby : if (akcja == ID_FILE_SAVE) save_csv();
#define ID_RESET_GRADIENT 1001 // Edit -> Reset Gradient / CTRL + R
#define ID_MODE_LINEAR    1002 // Edit -> Mode -> Linear
#define ID_MODE_RADIAL    1003 // Edit -> Mode -> Radial 
#define ID_FILE_OPEN      1004 // File -> Open CSV / CTRL + O
#define ID_FILE_SAVE      1005 // File -> Save CSV / CTRL + S
#define ID_FILE_EXPORT    1006 // File -> Export BMP 




struct ColorStop {
    float position; // gdzie lezy kolor 0.0 - maks lewo , 1.0 - maks prawo 
    COLORREF color; // przetrzymuje jaki to kolor 

    bool operator<(const ColorStop& other) const { // wlasny komparator 
        return position < other.position;
    }
};

class Window {
public:
    Window(HINSTANCE instance);
    ~Window();

    // Uruchamia główną pętlę komunikatów aplikacji.
    int run(int show_command);

    // Informacja, czy okno główne zostało poprawnie utworzone.
    bool is_initialized() const { return m_hwnd != nullptr; }

private:
    HWND m_hwnd; // uchwyt do głównego okna 
    HWND m_hCanvas; // uchwyt do obszaru canvas 
    HWND m_hStrip; // uchwyt do paska na dole 
    HINSTANCE m_hInstance;

    std::vector<ColorStop> m_stops;
    int m_draggedStopIndex;
    int m_hoveredStopIndex;
    bool m_isDragging;

    // Punkty definiujące kierunek/środek gradientu na canvasie.
    POINT m_ptStart;
    POINT m_ptEnd;
    int m_draggedCanvasPt;
    int m_hoveredCanvasPt;
    bool m_isRadial;

    // Renderuje gradient do bufora DIB i wyświetla go na canvasie.
    void render_canvas_dib(HDC hdc, const RECT& rc);

    static const std::wstring s_class_name;

    bool register_class();
    static LRESULT CALLBACK window_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK canvas_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK strip_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT canvas_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT strip_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    void reset_gradient();
    COLORREF get_interpolated_color(float t);
    int hit_test_strip(int x, int width);

    // Operacje wejścia/wyjścia: zapis/odczyt CSV i eksport BMP.
    void load_csv();
    void save_csv();

    void export_bmp();
};