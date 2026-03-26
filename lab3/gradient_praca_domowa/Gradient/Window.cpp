#include "Window.h"
#include "ColorPicker.h"

COLORREF InterpolateColor(COLORREF c1, COLORREF c2, float t) {
    int r1 = GetRValue(c1), g1 = GetGValue(c1), b1 = GetBValue(c1);
    int r2 = GetRValue(c2), g2 = GetGValue(c2), b2 = GetBValue(c2);

    int r = r1 + static_cast<int>((r2 - r1) * t);
    int g = g1 + static_cast<int>((g2 - g1) * t);
    int b = b1 + static_cast<int>((b2 - b1) * t);

    return RGB(r, g, b);
}

COLORREF GetGradientColor(const std::vector<ColorStop>& stops, float t) {
    if (stops.empty()) return RGB(0, 0, 0);
    if (stops.size() == 1) return stops[0].color;

    // Zabezpieczenie przed wyjściem poza skalę
    if (t <= stops.front().position) return stops.front().color;
    if (t >= stops.back().position) return stops.back().color;

    // Szukamy dwóch punktów, pomiędzy którymi aktualnie jesteśmy
    for (size_t i = 0; i < stops.size() - 1; ++i) {
        if (t >= stops[i].position && t <= stops[i + 1].position) {
            float range = stops[i + 1].position - stops[i].position;
            float local_t = (range > 0.0f) ? (t - stops[i].position) / range : 0.0f;
            return InterpolateColor(stops[i].color, stops[i + 1].color, local_t);
        }
    }
    return RGB(0, 0, 0);
}

void Window::render_canvas_dib(HDC hdc, const RECT& rc) {
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0 || m_stops.size() < 2) return;

    // --- OPTYMALIZACJA: Tablica LUT (Look-Up Table) ---
    // Zamiast liczyć kolor 240 000 razy, liczymy go tylko 1024 razy!
    const int LUT_SIZE = 1024;
    std::vector<DWORD> lut(LUT_SIZE);
    for (int i = 0; i < LUT_SIZE; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(LUT_SIZE - 1);
        COLORREF c = GetGradientColor(m_stops, t);
        // Zapisujemy gotowy format DIB, żeby nie przesuwać bitów później
        lut[i] = (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c);
    }
    // --------------------------------------------------

    std::vector<DWORD> pixels(width * height);

    float dx = static_cast<float>(m_ptEnd.x - m_ptStart.x);
    float dy = static_cast<float>(m_ptEnd.y - m_ptStart.y);
    float lengthSq = dx * dx + dy * dy;
    if (lengthSq < 0.0001f) lengthSq = 1.0f;

    // Optymalizacja 2: Wyciągamy pierwiastek raz, ZANIM wejdziemy do wielkiej pętli
    float maxDist = sqrt(lengthSq);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float px = static_cast<float>(x - m_ptStart.x);
            float py = static_cast<float>(y - m_ptStart.y);
            float t = 0.0f;

            if (m_isRadial) {
                float dist = sqrt(px * px + py * py);
                t = (maxDist > 0.0001f) ? (dist / maxDist) : 0.0f;
            }
            else {
                t = (px * dx + py * dy) / lengthSq;
            }

            t = std::max(0.0f, std::min(t, 1.0f));

            // MAGIA LUT: Pobieramy gotowy kolor ze "ściągawki" błyskawicznie!
            int lutIndex = static_cast<int>(t * (LUT_SIZE - 1));
            pixels[(height - 1 - y) * width + x] = lut[lutIndex];
        }
    }

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(hdc, 0, 0, width, height, 0, 0, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);
}

const std::wstring Window::s_class_name{ L"Gradient" };

Window::Window(HINSTANCE instance)
    : m_hInstance(instance),
    m_hwnd(nullptr),
    m_stops{ { 0.0f, RGB(0, 0, 0) }, { 1.0f, RGB(255, 255, 255) } },
    m_ptStart{ 50, 50 },
    m_ptEnd{ 350, 250 },
    m_draggedCanvasPt(0),
    m_hoveredCanvasPt(0), // <--- DODANE
    m_isRadial(false)
{
    if (!register_class()) return;

    // Tworzenie głównego okna
    m_hwnd = CreateWindowExW(
        0, s_class_name.c_str(), L"Gradient Editor",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        nullptr, nullptr, m_hInstance, this // 'this' przekazujemy do procedury okna
    );

    HMENU hMenu = CreateMenu();

    // Zakładka File
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_OPEN, L"Open CSV\tCtrl+O");
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_SAVE, L"Save CSV\tCtrl+S");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXPORT, L"Export BMP\tCtrl+E");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"File");

    // Zakładka Mode (tę już masz)
    HMENU hModeMenu = CreatePopupMenu();
    AppendMenuW(hModeMenu, MF_STRING, ID_MODE_LINEAR, L"Linear");
    AppendMenuW(hModeMenu, MF_STRING, ID_MODE_RADIAL, L"Radial");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hModeMenu), L"Mode");

    SetMenu(m_hwnd, hMenu);

    // Tworzenie płótna (Canvas)
    m_hCanvas = CreateWindowExW(
        0, L"Canvas Class", nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hwnd, (HMENU)2001, m_hInstance, this
    );

    // Tworzenie paska kontrolnego (Strip)
    m_hStrip = CreateWindowExW(
        0, L"Strip Class", nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hwnd, (HMENU)2002, m_hInstance, this
    );
}

Window::~Window() {}

bool Window::register_class() {
    WNDCLASSEXW desc{};
    if (GetClassInfoExW(m_hInstance, s_class_name.c_str(), &desc) != 0) return true;

    // --- DODANE: Ładowanie ikony prosto z pliku ---
    HICON hIcon = (HICON)LoadImageW(nullptr, L"icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);

    // Rejestracja głównego okna
    desc.cbSize = sizeof(WNDCLASSEXW);
    desc.style = CS_HREDRAW | CS_VREDRAW;
    desc.hInstance = m_hInstance;
    desc.hIcon = hIcon;       // <--- PRZYPISANIE DUŻEJ IKONY
    desc.hIconSm = hIcon;     // <--- PRZYPISANIE MAŁEJ IKONY (na pasku zadań)
    desc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    desc.lpfnWndProc = window_proc_static;
    desc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    desc.lpszClassName = s_class_name.c_str();
    if (RegisterClassExW(&desc) == 0) return false;

    // Rejestracja klasy dla Canvas
    desc.lpfnWndProc = canvas_proc_static;
    desc.hbrBackground = nullptr;
    desc.lpszClassName = L"Canvas Class";
    if (RegisterClassExW(&desc) == 0) return false;

    // Rejestracja klasy dla Strip
    desc.lpfnWndProc = strip_proc_static;
    desc.hbrBackground = nullptr;
    desc.lpszClassName = L"Strip Class";

    return RegisterClassExW(&desc) != 0;
}

int Window::run(int show_command) {
    if (!m_hwnd) return EXIT_FAILURE;
    ShowWindow(m_hwnd, show_command);

    // Zaktualizowana tablica skrótów klawiszowych (dodano Ctrl+O, S i E)
    ACCEL accel[] = {
        { FCONTROL | FVIRTKEY, 'R', ID_RESET_GRADIENT },
        { FCONTROL | FVIRTKEY, 'O', ID_FILE_OPEN },
        { FCONTROL | FVIRTKEY, 'S', ID_FILE_SAVE },
        { FCONTROL | FVIRTKEY, 'E', ID_FILE_EXPORT }
    };
    HACCEL hAccel = CreateAcceleratorTableW(accel, 4); // Mamy teraz 4 skróty!

    MSG msg{};
    BOOL result;
    // Główna pęta komunikatów
    while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
        if (result == -1) return EXIT_FAILURE;
        if (!TranslateAcceleratorW(m_hwnd, hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    DestroyAcceleratorTable(hAccel);
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK Window::window_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    Window* app = nullptr;
    if (message == WM_NCCREATE) {
        auto p = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        app = static_cast<Window*>(p->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else {
        app = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    if (app) return app->window_proc(window, message, wParam, lParam);
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT Window::window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_GETMINMAXINFO: {
        // Blokada zmniejszania okna poniżej 400x300
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        RECT rc = { 0, 0, 400, 300 };
        AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, FALSE, 0);
        mmi->ptMinTrackSize.x = rc.right - rc.left;
        mmi->ptMinTrackSize.y = rc.bottom - rc.top;
        return 0;
    }
    case WM_SIZE: {
        // Układanie Canvas i Strip przy zmianie rozmiaru okna
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        int margin = 5;

        int strip_height = 35;
        int canvas_height = height - (3 * margin) - strip_height;

        if (m_hCanvas) MoveWindow(m_hCanvas, margin, margin, width - (2 * margin), canvas_height, TRUE);
        if (m_hStrip) MoveWindow(m_hStrip, margin, margin + canvas_height, width - (2 * margin), strip_height, TRUE);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1; // Zapobiega migotaniu tła
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_RESET_GRADIENT) reset_gradient();
        else if (LOWORD(wParam) == ID_MODE_LINEAR) { m_isRadial = false; InvalidateRect(m_hCanvas, nullptr, FALSE); }
        else if (LOWORD(wParam) == ID_MODE_RADIAL) { m_isRadial = true;  InvalidateRect(m_hCanvas, nullptr, FALSE); }
        // DODANE:
        else if (LOWORD(wParam) == ID_FILE_OPEN) load_csv();
        else if (LOWORD(wParam) == ID_FILE_SAVE) save_csv();
        else if (LOWORD(wParam) == ID_FILE_EXPORT) export_bmp();
        return 0;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK Window::canvas_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    Window* app = nullptr;
    if (message == WM_NCCREATE) { // to ogolnie przypisuje wskaznik do obiektu dla stworzonego okna canvas
        auto p = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        app = static_cast<Window*>(p->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else
    {
        app = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }
    if (app) return app->canvas_proc(window, message, wParam, lParam);
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK Window::strip_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    Window* app = nullptr;
    if (message == WM_NCCREATE) {
        auto p = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        app = static_cast<Window*>(p->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else {
        app = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }
    if (app) return app->strip_proc(window, message, wParam, lParam);
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT Window::canvas_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        return 1; // Brak migotania tła

        // --- 1. RYSOWANIE KANWY ---
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(window, &ps);
        RECT rc;
        GetClientRect(window, &rc);

        render_canvas_dib(hdc, rc);

        HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HGDIOBJ oldBrush = SelectObject(hdc, nullBrush);

        // ZMODYFIKOWANA FUNKCJA: Rysuje grubsze i kolorowe kółko, jeśli najedziemy na nie myszką!
        auto draw_handle = [&](POINT pt, int pt_id) {
            bool isHovered = (pt_id == m_hoveredCanvasPt);

            // Zewnętrzne kółko (grubsza biała ramka przy najechaniu)
            HPEN whitePen = CreatePen(PS_SOLID, isHovered ? 5 : 3, RGB(255, 255, 255));
            HGDIOBJ oldPen = SelectObject(hdc, whitePen);
            Ellipse(hdc, pt.x - 7, pt.y - 7, pt.x + 7, pt.y + 7);

            // Wewnętrzne kółko (zmienia kolor na np. czerwony po najechaniu)
            COLORREF innerColor = isHovered ? RGB(255, 50, 50) : RGB(0, 0, 0);
            HPEN blackPen = CreatePen(PS_SOLID, 2, innerColor);
            SelectObject(hdc, blackPen);
            Ellipse(hdc, pt.x - 5, pt.y - 5, pt.x + 5, pt.y + 5);

            SelectObject(hdc, oldPen);
            DeleteObject(whitePen);
            DeleteObject(blackPen);
            };

        // Rysujemy punkty podając im ich numery (1 = Start, 2 = End)
        draw_handle(m_ptStart, 1);
        draw_handle(m_ptEnd, 2);

        SelectObject(hdc, oldBrush);
        EndPaint(window, &ps);
        return 0;
    }

                 // --- 2. ŁAPANIE KÓŁKA MYSZKĄ ---
    case WM_LBUTTONDOWN: {
        int x = (short)LOWORD(lParam);
        int y = (short)HIWORD(lParam);

        // Sprawdzamy czy kliknęliśmy w punkt START (używamy małego promienia np. 10px)
        int dx1 = x - m_ptStart.x;
        int dy1 = y - m_ptStart.y;
        if (dx1 * dx1 + dy1 * dy1 <= 100) {
            m_draggedCanvasPt = 1; // 1 = ciągniemy Start
            SetCapture(window);    // Blokujemy myszkę dla naszego okna
            return 0;
        }

        // Sprawdzamy czy kliknęliśmy w punkt END
        int dx2 = x - m_ptEnd.x;
        int dy2 = y - m_ptEnd.y;
        if (dx2 * dx2 + dy2 * dy2 <= 100) {
            m_draggedCanvasPt = 2; // 2 = ciągniemy End
            SetCapture(window);
            return 0;
        }
        return 0;
    }

                       // --- 3. PRZESUWANIE MYSZKĄ ---
    case WM_MOUSEMOVE: {
        int x = (short)LOWORD(lParam);
        int y = (short)HIWORD(lParam);

        // --- 1. SPRAWDZAMY HOVER (Efekt najechania) ---
        int hover = 0;
        int dx1 = x - m_ptStart.x; int dy1 = y - m_ptStart.y;
        int dx2 = x - m_ptEnd.x;   int dy2 = y - m_ptEnd.y;

        if (dx1 * dx1 + dy1 * dy1 <= 100) hover = 1;
        else if (dx2 * dx2 + dy2 * dy2 <= 100) hover = 2;

        if (hover != m_hoveredCanvasPt) {
            m_hoveredCanvasPt = hover;
            InvalidateRect(window, nullptr, FALSE); // Wymuś odrysowanie, bo myszka weszła/wyszła z kółka!
        }

        // --- 2. PRZESUWANIE KÓŁKA (Jeśli trzymamy przycisk) ---
        if (m_draggedCanvasPt != 0) {
            if (m_draggedCanvasPt == 1) {
                m_ptStart.x = x;
                m_ptStart.y = y;
            }
            else if (m_draggedCanvasPt == 2) {
                m_ptEnd.x = x;
                m_ptEnd.y = y;
            }
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }

                     // --- 4. PUSZCZANIE KÓŁKA ---
    case WM_LBUTTONUP: {
        if (m_draggedCanvasPt != 0) {
            m_draggedCanvasPt = 0; // Przestajemy ciągnąć
            ReleaseCapture();      // Uwalniamy myszkę
        }
        return 0;
    }
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

// --- SPRAWDZANIE KLIKNIĘCIA W PASEK ---
int Window::hit_test_strip(int x, int width) {
    int track_margin = 10;
    int track_width = width - (2 * track_margin);
    int handle_width = 6; // margines błędu przy klikaniu (w pikselach)

    // Szukamy od końca, żeby móc kliknąć w ten uchwyt, który narysował się na wierzchu
    for (int i = static_cast<int>(m_stops.size()) - 1; i >= 0; --i) {
        int stop_x = track_margin + static_cast<int>(m_stops[i].position * track_width);
        if (abs(x - stop_x) <= handle_width) return i;
    }
    return -1; // Nie trafiono w żaden uchwyt
}

LRESULT Window::strip_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(window, &ps);
        RECT rc;
        GetClientRect(window, &rc);

        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;

        // --- 1. START PODWÓJNEGO BUFOROWANIA ---
        HDC memDC = CreateCompatibleDC(hdc); // Tworzymy wirtualne płótno w pamięci
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height); // Tworzymy "kartkę papieru"
        HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap); // Kładziemy kartkę na płótnie

        // 2. Najpierw malujemy tło paska na standardowy systemowy kolor (RYSUJEMY PO memDC!)
        HBRUSH bgBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);

        int strip_h = height / 2;
        int offset_y = 5;
        int track_margin = 10;
        int track_width = width - (2 * track_margin);

        // 3. Rysujemy cienki pasek z gradientem
        if (m_stops.size() >= 2 && track_width > 0) {
            COLORREF startColor = m_stops[0].color;
            COLORREF endColor = m_stops[m_stops.size() - 1].color;

            for (int i = 0; i < track_width; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(track_width);
                COLORREF blendedColor = InterpolateColor(startColor, endColor, t);

                HPEN pen = CreatePen(PS_SOLID, 1, blendedColor);
                HGDIOBJ oldPen = SelectObject(memDC, pen);

                int x = track_margin + i;
                MoveToEx(memDC, x, offset_y, nullptr);
                LineTo(memDC, x, offset_y + strip_h);

                SelectObject(memDC, oldPen);
                DeleteObject(pen);
            }
        }

        // 4. Rysowanie uchwytów (prostokątów)
        for (size_t i = 0; i < m_stops.size(); ++i) {
            int x = track_margin + static_cast<int>(m_stops[i].position * track_width);
            int hw = 4;
            int left = x - hw;
            int right = x + hw;
            int top = offset_y - 2;
            int bottom = offset_y + strip_h + 2;

            HBRUSH handleBrush = CreateSolidBrush(m_stops[i].color);
            // Pogrubiamy ramkę dla efektu HOVER
            HPEN handlePen = CreatePen(PS_SOLID, (i == m_hoveredStopIndex) ? 2 : 1, RGB(0, 0, 0));

            HGDIOBJ oldBrush = SelectObject(memDC, handleBrush);
            HGDIOBJ oldPen = SelectObject(memDC, handlePen);

            Rectangle(memDC, left, top, right, bottom);

            SelectObject(memDC, oldPen);
            SelectObject(memDC, oldBrush);
            DeleteObject(handlePen);
            DeleteObject(handleBrush);
        }

        // --- 5. FINISZ: PRZEKLEJAMY GOTOWY OBRAZEK NA EKRAN ---
        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

        // --- 6. SPRZĄTANIE PAMIĘCI ---
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);

        EndPaint(window, &ps);
        return 0;
    }
                 // --- 1. DODAWANIE PUNKTU (DWUKLIK) ---
    case WM_LBUTTONDBLCLK: {
        int x = (short)LOWORD(lParam);
        RECT rc; GetClientRect(window, &rc);
        int track_margin = 10;
        int track_width = (rc.right - rc.left) - (2 * track_margin);

        if (track_width > 0) {
            // Przeliczamy pozycję X z ekranu na wartość 0.0 - 1.0
            float pos = static_cast<float>(x - track_margin) / static_cast<float>(track_width);
            pos = std::max(0.0f, std::min(pos, 1.0f)); // Blokada w zakresie

            // Pobieramy kolor, jaki aktualnie jest w tym miejscu
            COLORREF newColor = GetGradientColor(m_stops, pos);

            // Dodajemy nowy punkt i sortujemy listę!
            m_stops.push_back({ pos, newColor });
            std::sort(m_stops.begin(), m_stops.end());

            // Przerysowujemy pasek i płótno
            InvalidateRect(m_hCanvas, nullptr, FALSE);
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }

                         // --- 2. USUWANIE PUNKTU (PRAWY KLIK) ---
    case WM_RBUTTONDOWN: {
        if (m_stops.size() > 2) { // Muszą zostać minimum 2 punkty!
            RECT rc; GetClientRect(window, &rc);
            int idx = hit_test_strip((short)LOWORD(lParam), rc.right - rc.left);

            if (idx != -1) {
                m_stops.erase(m_stops.begin() + idx);
                InvalidateRect(m_hCanvas, nullptr, FALSE);
                InvalidateRect(window, nullptr, FALSE);
            }
        }
        return 0;
    }

                       // --- 3. ŁAPANIE PUNKTU LUB KLIKNIĘCIE (LEWY KLIK) ---
    case WM_LBUTTONDOWN: {
        RECT rc; GetClientRect(window, &rc);
        m_draggedStopIndex = hit_test_strip((short)LOWORD(lParam), rc.right - rc.left);

        if (m_draggedStopIndex != -1) {
            m_isDragging = false; // Zakładamy, że to może być tylko kliknięcie
            SetCapture(window);   // Kradniemy myszkę
        }
        return 0;
    }

                       // --- 4. PRZESUWANIE PUNKTU ---
    case WM_MOUSEMOVE: {
        int x = (short)LOWORD(lParam);
        RECT rc; GetClientRect(window, &rc);
        int width = rc.right - rc.left;

        // Efekt HOVER (podświetlenie)
        int hovered = hit_test_strip(x, width);
        if (hovered != m_hoveredStopIndex) {
            m_hoveredStopIndex = hovered;
            InvalidateRect(window, nullptr, FALSE); // Wymusza odrysowanie, gdy myszka wchodzi/wychodzi
        }

        // Jeśli trzymamy wciśnięty przycisk i przeciągamy
        if (m_draggedStopIndex != -1 && (wParam & MK_LBUTTON)) {
            m_isDragging = true; // Oznaczamy, że to przeciąganie, a nie zwykłe kliknięcie!

            int track_margin = 10;
            int track_width = width - (2 * track_margin);

            float new_pos = static_cast<float>(x - track_margin) / static_cast<float>(track_width);
            m_stops[m_draggedStopIndex].position = std::max(0.0f, std::min(new_pos, 1.0f));

            // Sortujemy punkty, bo użytkownik mógł przesunąć jeden punkt za drugi
            std::sort(m_stops.begin(), m_stops.end());

            // Ponieważ lista się posortowała, nasz punkt mógł zmienić indeks (np. z [0] na [1]). Musimy go znaleźć!
            for (int i = 0; i < m_stops.size(); i++) {
                if (hit_test_strip(x, width) == i) { m_draggedStopIndex = i; break; }
            }

            InvalidateRect(m_hCanvas, nullptr, FALSE);
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }

                     // --- 5. PUSZCZANIE PUNKTU (ZMIANA KOLORU) ---
    // --- 5. PUSZCZANIE PUNKTU (ZMIANA KOLORU) ---
    case WM_LBUTTONUP: {
        ReleaseCapture();

        if (m_draggedStopIndex != -1 && !m_isDragging) {

            // --- NASZ NOWY, WŁASNY COLOR PICKER! ---
            COLORREF kolor = m_stops[m_draggedStopIndex].color;

            if (ColorPicker::Show(window, kolor)) {
                // Jeśli funkcja zwróciła 'true' (kliknięto OK), aktualizujemy kolor!
                m_stops[m_draggedStopIndex].color = kolor;
                InvalidateRect(m_hCanvas, nullptr, FALSE);
                InvalidateRect(window, nullptr, FALSE);
            }
        }

        m_draggedStopIndex = -1;
        m_isDragging = false;
        return 0;
    }
    }
    return DefWindowProcW(window, message, wParam, lParam);
    
}

// --- ZAPIS DO CSV ---
void Window::save_csv() {
    OPENFILENAMEW ofn{};
    wchar_t szFile[260] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"csv";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    // Pokazuje okienko "Zapisz jako"
    if (GetSaveFileNameW(&ofn)) {
        std::ofstream file(szFile);
        for (const auto& stop : m_stops) {
            // Wyciągamy kolory
            int r = GetRValue(stop.color);
            int g = GetGValue(stop.color);
            int b = GetBValue(stop.color);

            // Formatujemy jako tekst HEX (np. #FF0000)
            char hexColor[8];
            snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X", r, g, b);

            // Zapisujemy linię w formacie z zadania domowego
            file << stop.position << "," << hexColor << "\n";
        }
    }
}

// --- ODCZYT Z CSV ---
void Window::load_csv() {
    OPENFILENAMEW ofn{};
    wchar_t szFile[260] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Pokazuje okienko "Otwórz plik"
    if (GetOpenFileNameW(&ofn)) {
        std::ifstream file(szFile);
        std::string line;
        std::vector<ColorStop> new_stops;

        while (std::getline(file, line)) {
            size_t comma_pos = line.find(',');
            if (comma_pos != std::string::npos) {
                try {
                    // Rozdzielamy na pozycję i kolor
                    float pos = std::stof(line.substr(0, comma_pos));
                    std::string hexStr = line.substr(comma_pos + 1);

                    if (hexStr.length() >= 7 && hexStr[0] == '#') {
                        int r, g, b;
                        if (sscanf_s(hexStr.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3) {
                            new_stops.push_back({ pos, RGB(r, g, b) });
                        }
                    }
                }
                catch (...) {
                    // Ignorujemy zepsute linijki
                }
            }
        }

        // Zabezpieczenie: ładujemy tylko, jeśli udało się przeczytać minimum 2 punkty
        if (new_stops.size() >= 2) {
            m_stops = new_stops;
            std::sort(m_stops.begin(), m_stops.end()); // Ustawiamy je w poprawnej kolejności
            InvalidateRect(m_hCanvas, nullptr, FALSE);
            InvalidateRect(m_hStrip, nullptr, FALSE);
        }
        else {
            MessageBoxW(m_hwnd, L"Plik CSV jest uszkodzony!", L"Błąd", MB_ICONERROR);
        }
    }
}

// --- RESET GRADIENTU (Ctrl+R) ---
void Window::reset_gradient() {
    m_stops.clear();
    m_stops.push_back({ 0.0f, RGB(0, 0, 0) });
    m_stops.push_back({ 1.0f, RGB(255, 255, 255) });
    if (m_hCanvas) InvalidateRect(m_hCanvas, nullptr, FALSE);
    if (m_hStrip) InvalidateRect(m_hStrip, nullptr, FALSE);
}

// --- EKSPORT DO BMP (Ctrl+E) ---
void Window::export_bmp() {
    OPENFILENAMEW ofn{};
    wchar_t szFile[260] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"BMP Files\0*.bmp\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"bmp";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn)) {
        // Pobieramy rozmiar naszego płótna
        RECT rc;
        GetClientRect(m_hCanvas, &rc);
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;
        if (width <= 0 || height <= 0) return;

        // Tworzymy nagłówki dla formatu BMP (nieskompresowany 32-bit RGB)
        BITMAPFILEHEADER bfh{};
        BITMAPINFOHEADER bih{};

        bih.biSize = sizeof(BITMAPINFOHEADER);
        bih.biWidth = width;
        bih.biHeight = height; // Wysokość dodatnia = renderujemy od dołu do góry
        bih.biPlanes = 1;
        bih.biBitCount = 32;   // 4 bajty na piksel (idealnie wyrównane w pamięci)
        bih.biCompression = BI_RGB;

        bfh.bfType = 0x4D42; // Sygnatura "BM"
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bfh.bfSize = bfh.bfOffBits + (width * height * 4);

        // Generujemy w pamięci tablicę pikseli użwając naszej logiki gradientu
        std::vector<DWORD> pixels(width * height);

        float dx = static_cast<float>(m_ptEnd.x - m_ptStart.x);
        float dy = static_cast<float>(m_ptEnd.y - m_ptStart.y);
        float lengthSq = dx * dx + dy * dy;
        if (lengthSq < 0.0001f) lengthSq = 1.0f;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float px = static_cast<float>(x - m_ptStart.x);
                float py = static_cast<float>(y - m_ptStart.y);
                float t = 0.0f;

                if (m_isRadial) {
                    float dist = sqrt(px * px + py * py);
                    float maxDist = sqrt(lengthSq);
                    t = (maxDist > 0.0001f) ? (dist / maxDist) : 0.0f;
                }
                else {
                    t = (px * dx + py * dy) / lengthSq;
                }
                t = std::max(0.0f, std::min(t, 1.0f));

                COLORREF c = GetGradientColor(m_stops, t);
                // BMP oczekuje kanałów w formacie 0x00RRGGBB (w WinAPI)
                DWORD dibColor = (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c);
                pixels[(height - 1 - y) * width + x] = dibColor;
            }
        }

        // Zapisujemy nagłówki i tablicę prosto do pliku binarnego
        std::ofstream file(szFile, std::ios::binary);
        if (file) {
            file.write(reinterpret_cast<const char*>(&bfh), sizeof(bfh));
            file.write(reinterpret_cast<const char*>(&bih), sizeof(bih));
            file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size() * sizeof(DWORD));
        }
    }
}