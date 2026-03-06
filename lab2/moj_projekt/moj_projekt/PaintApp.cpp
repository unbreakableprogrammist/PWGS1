#include "PaintApp.h"
#include <algorithm> // Dla std::min
#include <cmath>     // Dla std::abs

// Inicjalizacja statycznego pola klasy (nazwa klasy okna) [cite: 80, 96]
std::wstring const PaintApp::s_class_name{ L"Not_WM_PAINT_Class" };

// --- KONSTRUKTOR ---
PaintApp::PaintApp(HINSTANCE instance)
    : hInstance{ instance }, hwnd{}, is_drawing{ false }, start_point{ 0, 0 }
{
    // Tworzymy pędzle GDI do malowania tła i prostokątów [cite: 359, 374, 385, 390]
    m_bg_brush = CreateSolidBrush(RGB(30, 50, 90));       // Granatowe tło
    m_rect_brush = CreateSolidBrush(RGB(170, 70, 80));    // Czerwonawe prostokąty

    // Rejestrujemy klasę okna w systemie [cite: 97, 180, 183]
    register_class();

    // Definiujemy styl okna: stały rozmiar, brak zmiany wymiarów [cite: 115, 116, 123, 124]
    // WS_CLIPCHILDREN zapobiega migotaniu przy rysowaniu dzieci
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;

    // Obliczamy rozmiar okna tak, aby obszar roboczy miał dokładnie 800x600 [cite: 318, 322]
    RECT size{ 0, 0, 800, 600 };
    AdjustWindowRectEx(&size, style, false, 0);

    // Tworzymy główne okno aplikacji [cite: 113, 118, 120]
    hwnd = CreateWindowExW(
        0,                            // Brak rozszerzonych stylów
        s_class_name.c_str(),         // Nazwa zarejestrowanej klasy
        L"Not WM_PAINT",              // Tytuł okna (wymóg zadania)
        style,                        // Wyliczony styl
        CW_USEDEFAULT, CW_USEDEFAULT, // Domyślna pozycja na ekranie [cite: 125]
        size.right - size.left,       // Całkowita szerokość
        size.bottom - size.top,       // Całkowita wysokość
        nullptr,                      // Brak okna nadrzędnego
        nullptr,                      // Brak menu
        hInstance,                    // Instancja aplikacji [cite: 128]
        this                          // Przekazujemy 'this' do procedury statycznej [cite: 117, 128]
    );
}

// --- DESTRUKTOR ---
PaintApp::~PaintApp() {
    // Zwalniamy zasoby GDI (pędzle), aby uniknąć wycieków pamięci [cite: 20, 847]
    DeleteObject(m_bg_brush);
    DeleteObject(m_rect_brush);
}

// --- REJESTRACJA KLASY ---
bool PaintApp::register_class() {
    WNDCLASSEXW desc{};
    // Sprawdzamy, czy klasa jest już zarejestrowana [cite: 101, 105, 106]
    if (GetClassInfoExW(hInstance, s_class_name.c_str(), &desc) != 0) return true;

    // Wypełniamy strukturę klasy okna [cite: 100, 104, 107]
    desc.cbSize = sizeof(WNDCLASSEXW);
    desc.lpfnWndProc = window_proc_static;     // Statyczny pośrednik [cite: 108]
    desc.hInstance = hInstance;               // Instancja [cite: 109]
    desc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW); // Kursor [cite: 110]
    desc.hbrBackground = m_bg_brush;           // Pędzel tła [cite: 373, 374]
    desc.lpszClassName = s_class_name.c_str(); // Nazwa klasy [cite: 111]

    return RegisterClassExW(&desc) != 0; // Rejestracja [cite: 98, 112]
}

// --- PROCEDURA STATYCZNA (POŚREDNIK) ---
LRESULT CALLBACK PaintApp::window_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    PaintApp* app = nullptr;

    // Przy tworzeniu okna wyciągamy wskaźnik 'this' [cite: 132, 143, 145]
    if (message == WM_NCCREATE) {
        auto p = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        app = static_cast<PaintApp*>(p->lpCreateParams);
        // Zapisujemy wskaźnik w danych użytkownika okna [cite: 133, 146, 147]
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else {
        // Dla innych komunikatów pobieramy zapisany wskaźnik [cite: 134, 151, 154]
        app = reinterpret_cast<PaintApp*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    // Przekierowujemy do właściwej metody obiektu [cite: 131, 157]
    if (app != nullptr) {
        return app->window_proc(window, message, wParam, lParam);
    }
    return DefWindowProcW(window, message, wParam, lParam); // Domyślna obsługa [cite: 135, 160]
}

// --- OBSŁUGA ZDARZEŃ ---
LRESULT PaintApp::window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN: {
        is_drawing = true; // Rozpoczęcie rysowania
        start_point.x = GET_X_LPARAM(lParam); // Punkt startowy X
        start_point.y = GET_Y_LPARAM(lParam); // Punkt startowy Y

        SetCapture(window); // Przechwytywanie myszy poza oknem

        // Tworzymy prostokąt jako okno typu STATIC (zamiast rysowania GDI) [cite: 331, 334, 344]
        HWND new_rect = CreateWindowExW(
            0, L"STATIC", nullptr,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, // Styl dziecka [cite: 346]
            start_point.x, start_point.y, 0, 0,    // Pozycja startowa, rozmiar 0
            window, nullptr, hInstance, nullptr
        );
        m_rectangles.push_back(new_rect); // Zapisujemy uchwyt w pamięci
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (is_drawing && !m_rectangles.empty()) {
            int cur_x = GET_X_LPARAM(lParam);
            int cur_y = GET_Y_LPARAM(lParam);

            // Obliczamy lewy górny róg (min) i wymiary (abs) dla rysowania w każdą stronę
            int x = std::min((int)start_point.x, cur_x);
            int y = std::min((int)start_point.y, cur_y);
            int width = std::abs((int)start_point.x - cur_x);
            int height = std::abs((int)start_point.y - cur_y);

            // Aktualizujemy rozmiar aktywnego okna-prostokąta [cite: 407, 477, 478]
            SetWindowPos(m_rectangles.back(), nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        if (is_drawing) {
            is_drawing = false; // Koniec rysowania
            ReleaseCapture();   // Zwolnienie myszy
        }
        return 0;
    }

    case WM_KEYDOWN: {
        if (wParam == VK_BACK && !m_rectangles.empty()) {
            // Usuwanie ostatniego zapisanego prostokąta
            DestroyWindow(m_rectangles.back());
            m_rectangles.pop_back();

            if (is_drawing) {
                is_drawing = false;
                ReleaseCapture();
            }
            // Wymuszenie odświeżenia tła (true = wyczyść) [cite: 771]
            InvalidateRect(window, nullptr, TRUE);
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        // Ustawiamy kolor tła dla prostokątów (STATIC) [cite: 334, 380, 394, 403]
        return reinterpret_cast<INT_PTR>(m_rect_brush);
    }

    case WM_CLOSE: // [cite: 171]
        DestroyWindow(window); // [cite: 172]
        return 0;

    case WM_DESTROY: // [cite: 174]
        PostQuitMessage(0); // Koniec pętli komunikatów [cite: 165, 176]
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

// --- PĘTLA KOMUNIKATÓW --- [cite: 63, 188]
int PaintApp::run(int show_command) {
    ShowWindow(hwnd, show_command); // Pokaż okno [cite: 191]

    MSG msg{};
    BOOL result = TRUE;
    // Pobieranie komunikatów z kolejki [cite: 192, 194]
    while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
        if (result == -1) return EXIT_FAILURE; // Błąd pętli [cite: 196, 199]
        TranslateMessage(&msg); // Obsługa klawiatury [cite: 200]
        DispatchMessageW(&msg); // Przesłanie do procedury okna [cite: 201]
    }
    return EXIT_SUCCESS; // [cite: 202]
}