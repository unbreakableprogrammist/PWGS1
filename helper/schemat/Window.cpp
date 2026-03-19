#include "Window.h"

// --- KONSTRUKTOR ---
Window::Window(HINSTANCE hInstance) : m_hInstance(hInstance), m_hwnd(NULL) {
    const char* CLASS_NAME = "KalkulatorWindow";

    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = StaticWndProc; // Podpinamy statycznego pośrednika
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassA(&wc);

    // Ostatni parametr to "this" - przekazujemy wskaźnik na samego siebie do okna!
    m_hwnd = CreateWindowExA(
        0, CLASS_NAME, "Obiektowy Kalkulator", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 500,
        NULL, NULL, hInstance, this
    );

    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    }
}

// --- DESTRUKTOR ---
Window::~Window() {
    // Sprzątanie (jeśli potrzebne)
}

// --- PĘTLA KOMUNIKATÓW ---
bool Window::ProcessMessages() {
    MSG msg;
    // Używamy PeekMessage zamiast GetMessage - to lepsze podejście w aplikacjach obiektowych/grach
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            return false; // Zamknij pętlę
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true; // Pętla kręci się dalej
}

// --- STATYCZNY POŚREDNIK ---
LRESULT CALLBACK Window::StaticWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Window* pThis = NULL;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (Window*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

        // NOWOŚĆ: Ręcznie przypisujemy uchwyt okna od razu, gdy Windows nam go wyśle po raz pierwszy!
        pThis->m_hwnd = hwnd;
    }
    else {
        pThis = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// --- WŁAŚCIWA PROCEDURA OKNA ---
LRESULT Window::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

        // Tutaj w przyszłości dodamy WM_CREATE (dodawanie przycisków) 
        // i WM_COMMAND (klikanie przycisków kalkulatora)
    }
    return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}