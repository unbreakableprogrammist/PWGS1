#pragma once // Instrukcja dla kompilatora: "wczytaj ten plik tylko raz, nawet jak ktoś go dołączy wielokrotnie".
#include <windows.h> // Podstawowa i najważniejsza biblioteka ze wszystkimi funkcjami Windowsa.
#include <string>    // Biblioteka do obsługi nowoczesnych napisów tekstowych (np. std::wstring).

class Window {
public: // Sekcja publiczna - to mogą wywoływać inni (np. nasz main.cpp).

    // Konstruktor. Wywołuje się automatycznie przy tworzeniu obiektu. Pobiera uchwyt aplikacji.
    Window(HINSTANCE instance);

    // Destruktor. Wywołuje się, gdy obiekt jest niszczony (na samym końcu programu).
    ~Window();

    // Funkcja odpalająca pętlę komunikatów. Pobiera parametr decydujący o tym, jak pokazać okno.
    int run(int show_command);

    // Zwraca prawdę (true), jeśli m_hwnd nie jest puste (czyli okno poprawnie fizycznie powstało w systemie).
    bool is_initialized() const { return m_hwnd != nullptr; }

private: // Sekcja prywatna - dostępne tylko wewnątrz metod tej klasy.

    HWND m_hwnd;           // HWND (Handle to Window) - unikalny ID naszego fizycznego okna na ekranie.
    HINSTANCE m_hInstance; // Zapisany ID naszego programu (przekazany z WinMain).

    // Zmienna statyczna, wspólna dla całej klasy. Przechowuje nazwę naszej rejestrowanej "matrycy" okna.
    static const std::wstring s_class_name;

    // Wewnętrzna funkcja pomocnicza. Rejestruje naszą klasę okna w systemie przed jego stworzeniem.
    bool register_class();

    // Statyczny pośrednik. Musi być 'static', bo Windows napisano w C i nie rozumie obiektów z C++. 
    // Ta funkcja służy tylko do odebrania komunikatu od Windowsa i przekazania go do właściwego obiektu.
    static LRESULT CALLBACK window_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    // Prawdziwy "mózg" okna. Tu trafiają przetworzone komunikaty i tu decydujemy, co ma się stać na ekranie.
    LRESULT window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
};