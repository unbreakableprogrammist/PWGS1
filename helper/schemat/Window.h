#pragma once
#include <windows.h>

class Window {
public:
    Window(HINSTANCE hInstance); // Konstruktor - tutaj rejestrujemy i tworzymy okno
    ~Window();                   // Destruktor

    bool ProcessMessages();      // Nasza pętla komunikatów
    bool IsInitialized() const { return m_hwnd != NULL; } // Sprawdza, czy okno działa

private:
    HWND m_hwnd;           // Uchwyt naszego okna
    HINSTANCE m_hInstance; // Uchwyt aplikacji

    // 1. Statyczny pośrednik dla Windowsa (nie ma dostępu do zmiennych klasy)
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // 2. Prawdziwa funkcja okna (ma pełny dostęp do klasy, bo nie jest statyczna)
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};