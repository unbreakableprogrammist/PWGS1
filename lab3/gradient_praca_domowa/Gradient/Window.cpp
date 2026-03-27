#include "Window.h"
#include "ColorPicker.h"

// interpolacja liniowa -> miesza kolory punktu c1 i c2
COLORREF InterpolateColor(COLORREF c1, COLORREF c2, float t) {
    int r = GetRValue(c1) + static_cast<int>((GetRValue(c2) - GetRValue(c1)) * t);
    int g = GetGValue(c1) + static_cast<int>((GetGValue(c2) - GetGValue(c1)) * t);
    int b = GetBValue(c1) + static_cast<int>((GetBValue(c2) - GetBValue(c1)) * t);
    return RGB(r, g, b);
}

// Wyznacza kolor gradientu dla pozycji `t` na podstawie listy punktów.
COLORREF GetGradientColor(const std::vector<ColorStop>& stops, float t) {
    if (stops.empty()) return RGB(0, 0, 0);
    if (stops.size() == 1) return stops[0].color;
    if (t <= stops.front().position) return stops.front().color;
    if (t >= stops.back().position) return stops.back().color;

    for (size_t i = 0; i < stops.size() - 1; ++i) {
        if (t >= stops[i].position && t <= stops[i + 1].position) {
            float range = stops[i + 1].position - stops[i].position;
            float local_t = (range > 0.0f) ? (t - stops[i].position) / range : 0.0f;
            return InterpolateColor(stops[i].color, stops[i + 1].color, local_t);
        }
    }
    return RGB(0, 0, 0);
}

// Renderuje canvas do bufora i rysuje go na ekranie.
void Window::render_canvas_dib(HDC hdc, const RECT& rc) {
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0 || m_stops.size() < 2) return;

    const int LUT_SIZE = 1024; // optymalizacja 
    std::vector<DWORD> lut(LUT_SIZE);
    for (int i = 0; i < LUT_SIZE; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(LUT_SIZE - 1);
        COLORREF c = GetGradientColor(m_stops, t);
        lut[i] = (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c);
    }

	std::vector<DWORD> pixels(width * height); // wektor pikseli do renderowania


    float dx = static_cast<float>(m_ptEnd.x - m_ptStart.x); 
    float dy = static_cast<float>(m_ptEnd.y - m_ptStart.y);
    float lengthSq = dx * dx + dy * dy;
    if (lengthSq < 0.0001f) lengthSq = 1.0f;
    float maxDist = sqrt(lengthSq);
	// wypelniamy canvas pikselami z lut, obliczajac dla kazdego pikselu jego polozenie wzgledem linii Start-End
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
            pixels[(height - 1 - y) * width + x] = lut[static_cast<int>(t * (LUT_SIZE - 1))];
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

const std::wstring Window::s_class_name{ L"GradientEditor" };

Window::Window(HINSTANCE instance)
    : m_hInstance(instance),
    m_hwnd(nullptr),
    m_hCanvas(nullptr),
    m_hStrip(nullptr),
    m_stops{ { 0.0f, RGB(0, 0, 0) }, { 1.0f, RGB(255, 255, 255) } },
    m_draggedStopIndex(-1),
    m_hoveredStopIndex(-1),
    m_isDragging(false),
    m_ptStart{ 50, 50 },
    m_ptEnd{ 350, 250 },
    m_draggedCanvasPt(0),
    m_hoveredCanvasPt(0),
    m_isRadial(false)
{
    if (!register_class()) return;

    // Ustawia rozmiar okna tak, aby obszar roboczy miał 600x400.
    RECT rc = { 0, 0, 600, 400 };
    AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, TRUE, 0);

    m_hwnd = CreateWindowExW(
        0, s_class_name.c_str(), L"GradientEditor",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        nullptr, nullptr, m_hInstance, this
    );

    if (!m_hwnd) return;

    // Tworzy menu aplikacji.
    HMENU hMenu = CreateMenu();

    HMENU hFileMenu = CreatePopupMenu();
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_OPEN, L"Open CSV\tCtrl+O");
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_SAVE, L"Save CSV\tCtrl+S");
    AppendMenuW(hFileMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hFileMenu, MF_STRING, ID_FILE_EXPORT, L"Export BMP\tCtrl+E");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hFileMenu), L"File");

    HMENU hEditMenu = CreatePopupMenu();
    AppendMenuW(hEditMenu, MF_STRING, ID_RESET_GRADIENT, L"Reset Gradient\tCtrl+R");
    AppendMenuW(hEditMenu, MF_SEPARATOR, 0, nullptr);
    HMENU hModeMenu = CreatePopupMenu();
    AppendMenuW(hModeMenu, MF_STRING, ID_MODE_LINEAR, L"Linear");
    AppendMenuW(hModeMenu, MF_STRING, ID_MODE_RADIAL, L"Radial");
    AppendMenuW(hEditMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hModeMenu), L"Mode");
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hEditMenu), L"Edit");

    HMENU hHelpMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hHelpMenu), L"Help");

    SetMenu(m_hwnd, hMenu);

    m_hCanvas = CreateWindowExW(
        0, L"GE_Canvas", nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hwnd, (HMENU)2001, m_hInstance, this
    );

    m_hStrip = CreateWindowExW(
        0, L"GE_Strip", nullptr,
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_hwnd, (HMENU)2002, m_hInstance, this
    );
}

Window::~Window() {}

bool Window::register_class() {
    WNDCLASSEXW desc{};
    if (GetClassInfoExW(m_hInstance, s_class_name.c_str(), &desc) != 0) return true;

    HICON hIcon = (HICON)LoadImageW(nullptr, L"icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);

    desc.cbSize = sizeof(WNDCLASSEXW);
    desc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    desc.hInstance = m_hInstance;
    desc.hIcon = hIcon;
    desc.hIconSm = hIcon;
    desc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    desc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    desc.lpfnWndProc = window_proc_static;
    desc.lpszClassName = s_class_name.c_str();
    if (RegisterClassExW(&desc) == 0) return false;

    desc.lpfnWndProc = canvas_proc_static;
    desc.hbrBackground = nullptr;
    desc.lpszClassName = L"GE_Canvas";
    if (RegisterClassExW(&desc) == 0) return false;

    desc.lpfnWndProc = strip_proc_static;
    desc.hbrBackground = nullptr;
    desc.lpszClassName = L"GE_Strip";
    return RegisterClassExW(&desc) != 0;
}

int Window::run(int show_command) {
    if (!m_hwnd) return EXIT_FAILURE;
    ShowWindow(m_hwnd, show_command);

    ACCEL accel[] = {
        { FCONTROL | FVIRTKEY, 'R', ID_RESET_GRADIENT },
        { FCONTROL | FVIRTKEY, 'O', ID_FILE_OPEN },
        { FCONTROL | FVIRTKEY, 'S', ID_FILE_SAVE },
        { FCONTROL | FVIRTKEY, 'E', ID_FILE_EXPORT }
    };
    HACCEL hAccel = CreateAcceleratorTableW(accel, 4);

    MSG msg{};
    BOOL result;
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

// Przekierowuje komunikaty z procedury statycznej do instancji klasy.
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
        // Minimalny rozmiar okna: client area 400x300
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
        RECT rc = { 0, 0, 400, 300 };
        AdjustWindowRectEx(&rc, WS_OVERLAPPEDWINDOW, TRUE, 0);
        mmi->ptMinTrackSize.x = rc.right - rc.left;
        mmi->ptMinTrackSize.y = rc.bottom - rc.top;
        return 0;
    }
    case WM_SIZE: {
        // Układ: [margin][canvas][margin][strip][margin], stały margines 5px
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        const int margin = 5;
        const int strip_height = 35;
        int canvas_height = height - 3 * margin - strip_height;

        if (m_hCanvas) MoveWindow(m_hCanvas, margin, margin, width - 2 * margin, canvas_height, TRUE);
        if (m_hStrip)  MoveWindow(m_hStrip, margin, 2 * margin + canvas_height, width - 2 * margin, strip_height, TRUE);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_RESET_GRADIENT: reset_gradient();                                                 break;
        case ID_MODE_LINEAR:    m_isRadial = false; InvalidateRect(m_hCanvas, nullptr, FALSE);    break;
        case ID_MODE_RADIAL:    m_isRadial = true;  InvalidateRect(m_hCanvas, nullptr, FALSE);    break;
        case ID_FILE_OPEN:      load_csv();                                                       break;
        case ID_FILE_SAVE:      save_csv();                                                       break;
        case ID_FILE_EXPORT:    export_bmp();                                                     break;
        }
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
    if (message == WM_NCCREATE) {
        auto p = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        app = static_cast<Window*>(p->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else {
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
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(window, &ps);
        RECT rc;
        GetClientRect(window, &rc);

        render_canvas_dib(hdc, rc);

        // Rysowanie uchwytów Start/End z efektem hover (podwójna obramówka biało-czarna)
        HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HGDIOBJ oldBrush = SelectObject(hdc, nullBrush);

        auto draw_handle = [&](POINT pt, int pt_id) {
            bool hovered = (pt_id == m_hoveredCanvasPt);
            HPEN whitePen = CreatePen(PS_SOLID, hovered ? 5 : 3, RGB(255, 255, 255));
            HGDIOBJ oldPen = SelectObject(hdc, whitePen);
            Ellipse(hdc, pt.x - 7, pt.y - 7, pt.x + 7, pt.y + 7);

            HPEN innerPen = CreatePen(PS_SOLID, 2, hovered ? RGB(255, 50, 50) : RGB(0, 0, 0));
            SelectObject(hdc, innerPen);
            Ellipse(hdc, pt.x - 5, pt.y - 5, pt.x + 5, pt.y + 5);

            SelectObject(hdc, oldPen);
            DeleteObject(whitePen);
            DeleteObject(innerPen);
            };

        draw_handle(m_ptStart, 1);
        draw_handle(m_ptEnd, 2);

        SelectObject(hdc, oldBrush);
        EndPaint(window, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = (short)LOWORD(lParam);
        int y = (short)HIWORD(lParam);

        int dx1 = x - m_ptStart.x, dy1 = y - m_ptStart.y;
        if (dx1 * dx1 + dy1 * dy1 <= 100) {
            m_draggedCanvasPt = 1;
            SetCapture(window);
            return 0;
        }
        int dx2 = x - m_ptEnd.x, dy2 = y - m_ptEnd.y;
        if (dx2 * dx2 + dy2 * dy2 <= 100) {
            m_draggedCanvasPt = 2;
            SetCapture(window);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = (short)LOWORD(lParam);
        int y = (short)HIWORD(lParam);

        // Aktualizacja stanu hover
        int hover = 0;
        int dx1 = x - m_ptStart.x, dy1 = y - m_ptStart.y;
        int dx2 = x - m_ptEnd.x, dy2 = y - m_ptEnd.y;
        if (dx1 * dx1 + dy1 * dy1 <= 100) hover = 1;
        else if (dx2 * dx2 + dy2 * dy2 <= 100) hover = 2;

        if (hover != m_hoveredCanvasPt) {
            m_hoveredCanvasPt = hover;
            InvalidateRect(window, nullptr, FALSE);
        }

        // Przesuwanie aktywnego uchwytu
        if (m_draggedCanvasPt != 0) {
            if (m_draggedCanvasPt == 1) { m_ptStart.x = x; m_ptStart.y = y; }
            else { m_ptEnd.x = x; m_ptEnd.y = y; }
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP:
        m_draggedCanvasPt = 0;
        ReleaseCapture();
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

// Zwraca indeks color stopu znajdującego się pod pozycją x, lub -1
int Window::hit_test_strip(int x, int width) {
    const int track_margin = 10;
    int track_width = width - 2 * track_margin;
    const int handle_radius = 6;

    for (int i = static_cast<int>(m_stops.size()) - 1; i >= 0; --i) {
        int stop_x = track_margin + static_cast<int>(m_stops[i].position * track_width);
        if (abs(x - stop_x) <= handle_radius) return i;
    }
    return -1;
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

        // Podwójne buforowanie — rysujemy na memDC, potem BitBlt na ekran
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ oldBitmap = SelectObject(memDC, memBitmap);

        HBRUSH bgBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        FillRect(memDC, &rc, bgBrush);
        DeleteObject(bgBrush);

        const int track_margin = 10;
        const int offset_y = 5;
        int strip_h = height / 2;
        int track_width = width - 2 * track_margin;

        // Pasek gradientu uwzględniający wszystkie color stops
        if (m_stops.size() >= 2 && track_width > 0) {
            for (int i = 0; i < track_width; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(track_width);
                COLORREF c = GetGradientColor(m_stops, t);
                HPEN pen = CreatePen(PS_SOLID, 1, c);
                HGDIOBJ oldPen = SelectObject(memDC, pen);
                MoveToEx(memDC, track_margin + i, offset_y, nullptr);
                LineTo(memDC, track_margin + i, offset_y + strip_h);
                SelectObject(memDC, oldPen);
                DeleteObject(pen);
            }
        }

        // Uchwyty color stopów z efektem hover (grubsza obramówka)
        for (size_t i = 0; i < m_stops.size(); ++i) {
            int x = track_margin + static_cast<int>(m_stops[i].position * track_width);
            int hw = 4;
            bool hovered = (static_cast<int>(i) == m_hoveredStopIndex);

            HBRUSH hBrush = CreateSolidBrush(m_stops[i].color);
            HPEN   hPen = CreatePen(PS_SOLID, hovered ? 2 : 1, RGB(0, 0, 0));
            HGDIOBJ oldBrush = SelectObject(memDC, hBrush);
            HGDIOBJ oldPen = SelectObject(memDC, hPen);

            Rectangle(memDC, x - hw, offset_y - 2, x + hw, offset_y + strip_h + 2);

            SelectObject(memDC, oldPen);
            SelectObject(memDC, oldBrush);
            DeleteObject(hPen);
            DeleteObject(hBrush);
        }

        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBitmap);
        DeleteObject(memBitmap);
        DeleteDC(memDC);
        EndPaint(window, &ps);
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        // Dodanie nowego color stopu w miejscu podwójnego kliknięcia
        int x = (short)LOWORD(lParam);
        RECT rc; GetClientRect(window, &rc);
        int track_width = (rc.right - rc.left) - 20;

        if (track_width > 0) {
            float pos = std::max(0.0f, std::min(static_cast<float>(x - 10) / track_width, 1.0f));
            m_stops.push_back({ pos, GetGradientColor(m_stops, pos) });
            std::sort(m_stops.begin(), m_stops.end());
            InvalidateRect(m_hCanvas, nullptr, FALSE);
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }

    case WM_RBUTTONDOWN: {
        // Usunięcie color stopu prawym przyciskiem (minimum 2 muszą pozostać)
        if (m_stops.size() > 2) {
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

    case WM_LBUTTONDOWN: {
        RECT rc; GetClientRect(window, &rc);
        m_draggedStopIndex = hit_test_strip((short)LOWORD(lParam), rc.right - rc.left);
        if (m_draggedStopIndex != -1) {
            m_isDragging = false;
            SetCapture(window);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int x = (short)LOWORD(lParam);
        RECT rc; GetClientRect(window, &rc);
        int width = rc.right - rc.left;

        int hovered = hit_test_strip(x, width);
        if (hovered != m_hoveredStopIndex) {
            m_hoveredStopIndex = hovered;
            InvalidateRect(window, nullptr, FALSE);
        }

        if (m_draggedStopIndex != -1 && (wParam & MK_LBUTTON)) {
            m_isDragging = true;
            const int track_margin = 10;
            int track_width = width - 2 * track_margin;

            float new_pos = std::max(0.0f, std::min(static_cast<float>(x - track_margin) / track_width, 1.0f));
            m_stops[m_draggedStopIndex].position = new_pos;
            std::sort(m_stops.begin(), m_stops.end());

            // Po posortowaniu szukamy indeksu stopu najbliższego nowej pozycji
            float minDist = 1e9f;
            for (int i = 0; i < static_cast<int>(m_stops.size()); ++i) {
                float d = fabs(m_stops[i].position - new_pos);
                if (d < minDist) { minDist = d; m_draggedStopIndex = i; }
            }

            InvalidateRect(m_hCanvas, nullptr, FALSE);
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        ReleaseCapture();

        if (m_draggedStopIndex != -1 && !m_isDragging) {
            // Pojedyncze kliknięcie bez przeciągania — otwieramy color picker
            COLORREF kolor = m_stops[m_draggedStopIndex].color;
            if (ColorPicker::Show(window, kolor)) {
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

void Window::save_csv() {
    OPENFILENAMEW ofn{};
    wchar_t szFile[260] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.lpstrDefExt = L"csv";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn)) {
        std::ofstream file(szFile);
        for (const auto& stop : m_stops) {
            char hex[8];
            snprintf(hex, sizeof(hex), "#%02X%02X%02X",
                GetRValue(stop.color), GetGValue(stop.color), GetBValue(stop.color));
            file << stop.position << "," << hex << "\n";
        }
    }
}

void Window::load_csv() {
    OPENFILENAMEW ofn{};
    wchar_t szFile[260] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::ifstream file(szFile);
        std::string line;
        std::vector<ColorStop> new_stops;

        while (std::getline(file, line)) {
            size_t comma = line.find(',');
            if (comma == std::string::npos) continue;
            try {
                float pos = std::stof(line.substr(0, comma));
                std::string hex = line.substr(comma + 1);
                if (hex.size() >= 7 && hex[0] == '#') {
                    int r, g, b;
                    if (sscanf_s(hex.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3)
                        new_stops.push_back({ pos, RGB(r, g, b) });
                }
            }
            catch (...) {}
        }

        if (new_stops.size() >= 2) {
            m_stops = new_stops;
            std::sort(m_stops.begin(), m_stops.end());
            InvalidateRect(m_hCanvas, nullptr, FALSE);
            InvalidateRect(m_hStrip, nullptr, FALSE);
        }
        else {
            MessageBoxW(m_hwnd, L"Plik CSV jest uszkodzony lub zawiera mniej niż 2 punkty.", L"Błąd", MB_ICONERROR);
        }
    }
}

void Window::reset_gradient() {
    m_stops = { { 0.0f, RGB(0, 0, 0) }, { 1.0f, RGB(255, 255, 255) } };
    if (m_hCanvas) InvalidateRect(m_hCanvas, nullptr, FALSE);
    if (m_hStrip)  InvalidateRect(m_hStrip, nullptr, FALSE);
}

void Window::export_bmp() {
    OPENFILENAMEW ofn{};
    wchar_t szFile[260] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"BMP Files\0*.bmp\0All Files\0*.*\0";
    ofn.lpstrDefExt = L"bmp";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn)) return;

    RECT rc;
    GetClientRect(m_hCanvas, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) return;

    BITMAPFILEHEADER bfh{};
    BITMAPINFOHEADER bih{};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 32;
    bih.biCompression = BI_RGB;
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + width * height * 4;

    // Generujemy piksele tą samą logiką co render_canvas_dib
    std::vector<DWORD> pixels(width * height);
    float dx = static_cast<float>(m_ptEnd.x - m_ptStart.x);
    float dy = static_cast<float>(m_ptEnd.y - m_ptStart.y);
    float lengthSq = dx * dx + dy * dy;
    if (lengthSq < 0.0001f) lengthSq = 1.0f;
    float maxDist = sqrt(lengthSq);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float px = static_cast<float>(x - m_ptStart.x);
            float py = static_cast<float>(y - m_ptStart.y);
            float t = m_isRadial
                ? std::max(0.0f, std::min(sqrt(px * px + py * py) / maxDist, 1.0f))
                : std::max(0.0f, std::min((px * dx + py * dy) / lengthSq, 1.0f));

            COLORREF c = GetGradientColor(m_stops, t);
            pixels[(height - 1 - y) * width + x] = (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c);
        }
    }

    std::ofstream file(szFile, std::ios::binary);
    if (file) {
        file.write(reinterpret_cast<const char*>(&bfh), sizeof(bfh));
        file.write(reinterpret_cast<const char*>(&bih), sizeof(bih));
        file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size() * sizeof(DWORD));
    }
}