#pragma once
#include <windows.h>
#include <commdlg.h>   // <--- DODAJ TĘ LINIJKĘ (Common Dialogs)
#include <string>
#include <vector>
#include <algorithm>
#include <fstream> // DODANE: Do obsługi zapisywania plików CSV

#define ID_RESET_GRADIENT 1001
#define ID_MODE_LINEAR    1002
#define ID_MODE_RADIAL    1003
// DODANE IDENTYFIKATORY:
#define ID_FILE_OPEN      1004
#define ID_FILE_SAVE      1005
#define ID_FILE_EXPORT    1006



// Reprezentuje pojedynczy punkt na pasku gradientu
// Reprezentuje pojedynczy punkt na pasku gradientu
struct ColorStop {
    float position; // 0.0f (początek) do 1.0f (koniec)
    COLORREF color; // Kolor RGB

    // DODANE: Uczymy C++ jak sortować nasze punkty (zawsze od najmniejszej pozycji)
    bool operator<(const ColorStop& other) const {
        return position < other.position;
    }
};

class Window {
public:
    Window(HINSTANCE instance);
    ~Window();
    int run(int show_command);
    bool is_initialized() const { return m_hwnd != nullptr; }

private:
    HWND m_hwnd;
    HWND m_hCanvas;
    HWND m_hStrip;
    HINSTANCE m_hInstance;

    std::vector<ColorStop> m_stops;
    int m_draggedStopIndex;
    int m_hoveredStopIndex;
    bool m_isDragging;

    // NOWE ZMIENNE DLA ZADANIA DOMOWEGO (KANWA):
    POINT m_ptStart;       // Pozycja kółka Start
    POINT m_ptEnd;         // Pozycja kółka End
    int m_draggedCanvasPt; // Które kółko ciągniemy? (1 = Start, 2 = End, 0 = żadne)
    int m_hoveredCanvasPt; // DODANE: Które kółko jest podświetlone? (0 = żadne)
    bool m_isRadial;       // NOWE: Czy mamy włączony tryb kołowy?

    // NOWA FUNKCJA DO RYSOWANIA W PAMIĘCI RAM:
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

    // DODANE: Funkcje do plików
    void load_csv();
    void save_csv();
    
    void export_bmp();     // Nasz nowy eksport!
};