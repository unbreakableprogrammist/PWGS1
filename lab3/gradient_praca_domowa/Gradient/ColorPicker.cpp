#include "ColorPicker.h"
#pragma comment(lib, "comctl32.lib")

HHOOK       ColorPicker::s_mouseHook = nullptr;
ColorPicker* ColorPicker::s_activePicker = nullptr;
const wchar_t* ColorPicker::s_className = L"GE_ColorPicker";

// Konwertuje kolor RGB do modelu HSV.
void ColorPicker::RGBtoHSV(COLORREF rgb, float& h, float& s, float& v) {
    float r = GetRValue(rgb) / 255.0f;
    float g = GetGValue(rgb) / 255.0f;
    float b = GetBValue(rgb) / 255.0f;

    float cmax = std::max({ r, g, b });
    float cmin = std::min({ r, g, b });
    float diff = cmax - cmin;

    v = cmax;
    s = (cmax == 0.0f) ? 0.0f : diff / cmax;

    if (diff == 0.0f)      h = 0.0f;
    else if (cmax == r)    h = fmod((g - b) / diff, 6.0f);
    else if (cmax == g)    h = (b - r) / diff + 2.0f;
    else                   h = (r - g) / diff + 4.0f;

    h *= 60.0f;
    if (h < 0.0f) h += 360.0f;
}

// Konwertuje kolor HSV do modelu RGB.
COLORREF ColorPicker::HSVtoRGB(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - std::abs(fmod(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0, g = 0, b = 0;

    if (h < 60) { r = c; g = x; }
    else if (h < 120) { r = x; g = c; }
    else if (h < 180) { g = c; b = x; }
    else if (h < 240) { g = x; b = c; }
    else if (h < 300) { r = x;         b = c; }
    else { r = c;         b = x; }

    return RGB((r + m) * 255.0f, (g + m) * 255.0f, (b + m) * 255.0f);
}

ColorPicker::ColorPicker(HWND hParent, COLORREF startColor)
    : m_hParent(hParent), m_oldColor(startColor), m_currentColor(startColor),
    m_confirmed(false), m_dragMode(NONE), m_isUpdatingUI(false)
{
    RGBtoHSV(startColor, m_h, m_s, m_v);

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProcStatic;
    wcex.hInstance = GetModuleHandle(nullptr);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszClassName = s_className;
    RegisterClassExW(&wcex);

    // Tworzy okno pickera wyśrodkowane względem okna rodzica.
    int dlgW = 500, dlgH = 420;
    RECT parentRect;
    GetWindowRect(hParent, &parentRect);
    int posX = parentRect.left + (parentRect.right - parentRect.left - dlgW) / 2;
    int posY = parentRect.top + (parentRect.bottom - parentRect.top - dlgH) / 2;

    m_hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, s_className, L"Change Color",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN, 
        posX, posY, dlgW, dlgH, hParent, nullptr, wcex.hInstance, this);
    CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 380, 310, 80, 30, m_hwnd, (HMENU)1, nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 380, 350, 80, 30, m_hwnd, (HMENU)2, nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Reset", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 310, 80, 30, m_hwnd, (HMENU)3, nullptr, nullptr);
    CreateWindowW(L"BUTTON", L"Pick from Screen", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 250, 180, 30, m_hwnd, (HMENU)4, nullptr, nullptr);

    // Tworzy suwaki i pola edycyjne dla kanałów R,G,B,H,S,V.
    INITCOMMONCONTROLSEX icex{ sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icex);

    const wchar_t* labels[] = { L"R", L"G", L"B", L"H", L"S", L"V" };
    int            max_vals[] = { 255, 255, 255, 360, 100, 100 };

    for (int i = 0; i < 6; ++i) {
        int y = 20 + i * 35;
        CreateWindowW(L"STATIC", labels[i], WS_CHILD | WS_VISIBLE, 280, y + 5, 20, 20, m_hwnd, nullptr, nullptr, nullptr);

        m_hSliders[i] = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
            300, y, 100, 30, m_hwnd, nullptr, nullptr, nullptr);
        SendMessage(m_hSliders[i], TBM_SETRANGE, TRUE, MAKELPARAM(0, max_vals[i]));

        m_hEdits[i] = CreateWindowW(L"EDIT", L"0", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_NUMBER,
            410, y + 2, 40, 20, m_hwnd, nullptr, nullptr, nullptr);
    }

    CreateWindowW(L"STATIC", L"HTML hex notation:", WS_CHILD | WS_VISIBLE, 280, 230, 120, 20, m_hwnd, nullptr, nullptr, nullptr);
    m_hHexEdit = CreateWindowW(L"EDIT", L"000000", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_CENTER,
        405, 228, 60, 20, m_hwnd, nullptr, nullptr, nullptr);

    UpdateUI();
}

ColorPicker::~ColorPicker() {
    // Zwalnia hook pipety, jeśli okno jest niszczone w trakcie próbkowania.
    if (s_mouseHook && s_activePicker == this) {
        UnhookWindowsHookEx(s_mouseHook);
        s_mouseHook = nullptr;
        s_activePicker = nullptr;
    }
    if (m_hwnd) DestroyWindow(m_hwnd);
}

// Synchronizuje stan kontrolek z aktualnym kolorem.
void ColorPicker::UpdateUI() {
    if (!m_hwnd || m_isUpdatingUI) return;
    m_isUpdatingUI = true;

    int r = GetRValue(m_currentColor), g = GetGValue(m_currentColor), b = GetBValue(m_currentColor);
    int vals[] = { r, g, b,
        static_cast<int>(m_h),
        static_cast<int>(m_s * 100.0f),
        static_cast<int>(m_v * 100.0f) };

    HWND hFocus = GetFocus();
    for (int i = 0; i < 6; ++i) {
        SendMessage(m_hSliders[i], TBM_SETPOS, TRUE, vals[i]);
        if (m_hEdits[i] != hFocus)
            SetWindowTextW(m_hEdits[i], std::to_wstring(vals[i]).c_str());
    }

    if (m_hHexEdit != hFocus) {
        wchar_t hex[16];
        swprintf(hex, 16, L"%02x%02x%02x", r, g, b);
        SetWindowTextW(m_hHexEdit, hex);
    }

    m_isUpdatingUI = false;
}

// Wyświetla modal color picker i zwraca true jeśli użytkownik zatwierdził wybór
bool ColorPicker::Show(HWND hParent, COLORREF& inOutColor) {
    ColorPicker picker(hParent, inOutColor);
    EnableWindow(hParent, FALSE);
    ShowWindow(picker.m_hwnd, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessage(picker.m_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!IsWindow(picker.m_hwnd)) break;
    }

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);

    if (picker.m_confirmed) {
        inOutColor = picker.m_currentColor;
        return true;
    }
    return false;
}

LRESULT CALLBACK ColorPicker::WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ColorPicker* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        auto pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = static_cast<ColorPicker*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    }
    else {
        pThis = reinterpret_cast<ColorPicker*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    if (pThis) return pThis->WndProc(hwnd, msg, wParam, lParam);
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT ColorPicker::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_HSCROLL: {
        // Zmiana wartości przez suwak — aktualizuje kolor i HSV
        HWND hSlider = (HWND)lParam;
        for (int i = 0; i < 6; ++i) {
            if (hSlider != m_hSliders[i]) continue;
            int val = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
            if (i < 3) {
                int r = (i == 0) ? val : GetRValue(m_currentColor);
                int g = (i == 1) ? val : GetGValue(m_currentColor);
                int b = (i == 2) ? val : GetBValue(m_currentColor);
                m_currentColor = RGB(r, g, b);
                RGBtoHSV(m_currentColor, m_h, m_s, m_v);
            }
            else {
                if (i == 3) m_h = (float)val;
                if (i == 4) m_s = val / 100.0f;
                if (i == 5) m_v = val / 100.0f;
                m_currentColor = HSVtoRGB(m_h, m_s, m_v);
            }
            UpdateUI();
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
        return 0;
    }

    case WM_COMMAND: {
        // Obsługa pól edycyjnych (EN_CHANGE) i przycisków
        if (HIWORD(wParam) == EN_CHANGE && !m_isUpdatingUI) {
            HWND hEdit = (HWND)lParam;
            wchar_t buf[16];
            GetWindowTextW(hEdit, buf, 16);

            if (hEdit == m_hHexEdit && wcslen(buf) >= 6) {
                int r, g, b;
                if (swscanf_s(buf, L"%02x%02x%02x", &r, &g, &b) == 3) {
                    m_currentColor = RGB(r, g, b);
                    RGBtoHSV(m_currentColor, m_h, m_s, m_v);
                    UpdateUI();
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            else {
                for (int i = 0; i < 6; ++i) {
                    if (hEdit != m_hEdits[i] || wcslen(buf) == 0) continue;
                    try {
                        int val = std::stoi(buf);
                        if (i < 3) {
                            val = std::max(0, std::min(val, 255));
                            int r = (i == 0) ? val : GetRValue(m_currentColor);
                            int g = (i == 1) ? val : GetGValue(m_currentColor);
                            int b = (i == 2) ? val : GetBValue(m_currentColor);
                            m_currentColor = RGB(r, g, b);
                            RGBtoHSV(m_currentColor, m_h, m_s, m_v);
                        }
                        else {
                            if (i == 3) m_h = (float)std::max(0, std::min(val, 360));
                            if (i == 4) m_s = std::max(0, std::min(val, 100)) / 100.0f;
                            if (i == 5) m_v = std::max(0, std::min(val, 100)) / 100.0f;
                            m_currentColor = HSVtoRGB(m_h, m_s, m_v);
                        }
                        UpdateUI();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    catch (...) {}
                    break;
                }
            }
            return 0;
        }

        switch (LOWORD(wParam)) {
        case 1: m_confirmed = true;  DestroyWindow(hwnd); break; // OK
        case 2: m_confirmed = false; DestroyWindow(hwnd); break; // Cancel
        case 3: // Reset — powrót do koloru startowego
            m_currentColor = m_oldColor;
            RGBtoHSV(m_oldColor, m_h, m_s, m_v);
            UpdateUI();
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case 4: // Pick from Screen — instalacja globalnego hooka myszy
            if (!s_mouseHook) {
                s_activePicker = this;
                s_mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(nullptr), 0);
            }
            break;
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = (short)LOWORD(lParam);
        int y = (short)HIWORD(lParam);
        float dx = x - 150.0f, dy = y - 150.0f;
        float dist = sqrt(dx * dx + dy * dy);

        if (dist >= 90.0f && dist <= 120.0f) {
            m_dragMode = HUE_RING;
            SetCapture(hwnd);
            SetFocus(hwnd);
        }
        else if (dist < 90.0f) {
            m_dragMode = SV_TRIANGLE;
            SetCapture(hwnd);
            SetFocus(hwnd);
        }
        // fall-through do WM_MOUSEMOVE żeby drag zaczął działać natychmiast
    }
    case WM_MOUSEMOVE: {
        if (m_dragMode == NONE || !(wParam & MK_LBUTTON)) return 0;

        int x = (short)LOWORD(lParam);
        int y = (short)HIWORD(lParam);
        float dx = x - 150.0f, dy = y - 150.0f;

        if (m_dragMode == HUE_RING) {
            // Kąt myszy → wartość Hue
            float angle = atan2(-dy, dx) * 180.0f / 3.14159f;
            if (angle < 0) angle += 360.0f;
            m_h = angle;
        }
        else {
            // Barycentric mapping punktu myszy na S i V w obrębie trójkąta
            float ar = -m_h * 3.14159f / 180.0f;
            float R = 85.0f;
            float p1x = 150.0f + R * cos(ar), p1y = 150.0f + R * sin(ar);
            float p2x = 150.0f + R * cos(ar + 2.09439f), p2y = 150.0f + R * sin(ar + 2.09439f);
            float p3x = 150.0f + R * cos(ar - 2.09439f), p3y = 150.0f + R * sin(ar - 2.09439f);

            float denom = (p2y - p3y) * (p1x - p3x) + (p3x - p2x) * (p1y - p3y);
            float w1 = ((p2y - p3y) * (x - p3x) + (p3x - p2x) * (y - p3y)) / denom;
            float w2 = ((p3y - p1y) * (x - p3x) + (p1x - p3x) * (y - p3y)) / denom;
            float w3 = 1.0f - w1 - w2;

            float v = std::max(0.0f, std::min(w1 + w3, 1.0f));
            float s = std::max(0.0f, std::min((v > 0.0001f) ? w1 / v : 0.0f, 1.0f));
            m_s = s; m_v = v;
        }

        m_currentColor = HSVtoRGB(m_h, m_s, m_v);
        UpdateUI();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONUP:
        m_dragMode = NONE;
        ReleaseCapture();
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);

        RenderPickerDIB(hdc, rc);

        // Markery na pierścieniu Hue i w trójkącie S/V (biała + czarna obramówka)
        HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HGDIOBJ oldBrush = SelectObject(hdc, nullBrush);

        int cx = 150, cy = 150;
        float ar = -m_h * 3.14159f / 180.0f;

        HPEN blackPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        HPEN whitePen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));

        // Marker na pierścieniu Hue (promień 105 = środek między 90 a 120)
        int hx = cx + (int)(105 * cos(ar));
        int hy = cy + (int)(105 * sin(ar));
        SelectObject(hdc, blackPen);
        Ellipse(hdc, hx - 5, hy - 5, hx + 5, hy + 5);
        SelectObject(hdc, whitePen);
        Ellipse(hdc, hx - 3, hy - 3, hx + 3, hy + 3);

        // Marker w trójkącie S/V wyznaczony przez wagi barycentryczne
        float R = 85.0f;
        float p1x = cx + R * cos(ar), p1y = cy + R * sin(ar);
        float p2x = cx + R * cos(ar + 2.09439f), p2y = cy + R * sin(ar + 2.09439f);
        float p3x = cx + R * cos(ar - 2.09439f), p3y = cy + R * sin(ar - 2.09439f);
        float w1 = m_s * m_v, w3 = m_v - w1, w2 = 1.0f - m_v;
        int svx = (int)(w1 * p1x + w2 * p2x + w3 * p3x);
        int svy = (int)(w1 * p1y + w2 * p2y + w3 * p3y);

        SelectObject(hdc, blackPen);
        Ellipse(hdc, svx - 5, svy - 5, svx + 5, svy + 5);
        SelectObject(hdc, whitePen);
        Ellipse(hdc, svx - 3, svy - 3, svx + 3, svy + 3);

        SelectObject(hdc, oldBrush);
        DeleteObject(blackPen);
        DeleteObject(whitePen);

        // Paski porównania Current / Old
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, 20, 310, L"Current:", 8);
        TextOutW(hdc, 20, 340, L"Old:", 4);

        HBRUSH currBrush = CreateSolidBrush(m_currentColor);
        HBRUSH oldBr = CreateSolidBrush(m_oldColor);
        RECT rCurr = { 80, 310, 250, 330 };
        RECT rOld = { 80, 340, 250, 360 };
        FillRect(hdc, &rCurr, currBrush);
        FillRect(hdc, &rOld, oldBr);
        DeleteObject(currBrush);
        DeleteObject(oldBr);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        // Zwalniamy hook jeśli użytkownik zamknął okno w trakcie pipety
        if (s_mouseHook && s_activePicker == this) {
            UnhookWindowsHookEx(s_mouseHook);
            s_mouseHook = nullptr;
            s_activePicker = nullptr;
        }
        m_hwnd = nullptr;
        return 0;

    case WM_CLOSE:
        m_confirmed = false;
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Renderuje koło Hue (pierścień) i trójkąt S/V przez SetDIBitsToDevice
void ColorPicker::RenderPickerDIB(HDC hdc, const RECT& rc) {
    int width = rc.right - rc.left, height = rc.bottom - rc.top;
    if (width <= 0 || height <= 0) return;

    std::vector<DWORD> pixels(width * height);

    const int   cx = 150, cy = 150;
    const float r_outer = 120.0f, r_inner = 90.0f, r_tri = r_inner - 5.0f;
    float ar = -m_h * 3.14159f / 180.0f;

    float p1x = cx + r_tri * cos(ar), p1y = cy + r_tri * sin(ar);
    float p2x = cx + r_tri * cos(ar + 2.09439f), p2y = cy + r_tri * sin(ar + 2.09439f);
    float p3x = cx + r_tri * cos(ar - 2.09439f), p3y = cy + r_tri * sin(ar - 2.09439f);

    DWORD bg = GetSysColor(COLOR_BTNFACE);
    DWORD bgDib = (GetRValue(bg) << 16) | (GetGValue(bg) << 8) | GetBValue(bg);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float dx = x - cx, dy = y - cy;
            float dist = sqrt(dx * dx + dy * dy);
            DWORD color = bgDib;

            if (dist >= r_inner && dist <= r_outer) {
                // Pierścień Hue: kąt piksela → kolor HSV(hue, 1, 1)
                float a = atan2(-dy, dx) * 180.0f / 3.14159f;
                if (a < 0) a += 360.0f;
                COLORREF c = HSVtoRGB(a, 1.0f, 1.0f);
                color = (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c);
            }
            else if (dist < r_inner) {
                // Trójkąt S/V: wagi barycentryczne → kolor
                float denom = (p2y - p3y) * (p1x - p3x) + (p3x - p2x) * (p1y - p3y);
                float w1 = ((p2y - p3y) * (x - p3x) + (p3x - p2x) * (y - p3y)) / denom;
                float w2 = ((p3y - p1y) * (x - p3x) + (p1x - p3x) * (y - p3y)) / denom;
                float w3 = 1.0f - w1 - w2;
                if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                    float v = w1 + w3;
                    float s = (v > 0.0001f) ? w1 / v : 0.0f;
                    COLORREF c = HSVtoRGB(m_h, s, v);
                    color = (GetRValue(c) << 16) | (GetGValue(c) << 8) | GetBValue(c);
                }
            }

            pixels[(height - 1 - y) * width + x] = color;
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

// Hook globalny myszy (WH_MOUSE_LL) — przechwytuje kolor z ekranu po każdym WM_MOUSEMOVE,
// deinstaluje się i zatwierdza kolor po kliknięciu lewym przyciskiem
LRESULT CALLBACK ColorPicker::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && s_activePicker) {
        if (wParam == WM_MOUSEMOVE) {
            MSLLHOOKSTRUCT* ms = (MSLLHOOKSTRUCT*)lParam;
            HDC screenDC = GetDC(nullptr);
            COLORREF c = GetPixel(screenDC, ms->pt.x, ms->pt.y);
            ReleaseDC(nullptr, screenDC);

            s_activePicker->m_currentColor = c;
            RGBtoHSV(c, s_activePicker->m_h, s_activePicker->m_s, s_activePicker->m_v);
            s_activePicker->UpdateUI();
            InvalidateRect(s_activePicker->m_hwnd, nullptr, FALSE);
        }
        else if (wParam == WM_LBUTTONDOWN) {
            UnhookWindowsHookEx(s_mouseHook);
            s_mouseHook = nullptr;
            s_activePicker = nullptr;
            return 1; // blokujemy kliknięcie żeby nie trafiło do okna pod kursorem
        }
    }
    return CallNextHookEx(s_mouseHook, nCode, wParam, lParam);
}