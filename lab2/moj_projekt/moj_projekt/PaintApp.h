#pragma once
#include <windows.h>  // Główne narzędzia systemu Windows (typy HWND, HINSTANCE, funkcje okienkowe).
#include <windowsx.h> // Dodatkowe makra (np. GET_X_LPARAM), które ułatwią nam wyciąganie pozycji myszki.
#include <vector>     // Potrzebne do trzymania naszej "listy" narysowanych prostokątów.
#include <string>     // Potrzebne do przechowywania nazwy klasy okna jako tekstu.

class PaintApp {
private:
	HINSTANCE hInstance; // Uchwyt do instancji aplikacji, potrzebny do rejestracji klasy okna.
	HWND hwnd;          // Uchwyt do głównego okna aplikacji.

	HBRUSH m_bg_brush; // uchwyt do pedzla do tla , aby nie bylo bialego tła podczas rysowania prostokątów
	HBRUSH m_rect_brush; // uchwyt do pedzla do rysowania prostokątów

	// std::vector będzie przechowywał uchwyty (HWND) wszystkich prostokątów (które de facto są małymi okienkami).
	std::vector<HWND> m_rectangles;

	bool is_drawing; // flaga ktora mowi czy uzytkownik wlasnie rysuje prostokat (czy przycisk myszy jest wcisniety)

	POINT start_point; // punkt początkowy rysowania prostokąta (gdzie użytkownik kliknął myszką)

	// Nazwa naszej klasy okna (wymóg systemu Windows, by każde okno miało zarejestrowaną nazwę).
	// Jest "static", bo nazwa klasy jest wspólna dla wszystkich ewentualnych okien tego typu.
	static std::wstring const s_class_name;

	bool register_class(); // metoda do rejestracji klasy okna, która będzie używana do tworzenia głównego okna i prostokątów.

	// do tej metody windows bedzie przekazywal nam co sie dzieje z oknem (np. klikniecie myszka, ruch myszki, zamkniecie okna itp.)
	// static po to ze C nie zna obiektowowosci i wtedy traktuje window proc static jak zwykla funkcje a my z niej bedziemy przekazywac wszystko do window proc 
	static LRESULT window_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam); 
	LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam); // to jest metoda do obslugi zdarzen okna, bedzie wywolywana przez window_proc_static
public:
	PaintApp(HINSTANCE instance); // konstruktor, który przyjmuje uchwyt do instancji aplikacji i inicjalizuje nasze zmienne
	~PaintApp(); // destruktor, który zwalnia zasoby (np. pędzle) i niszczy wszystkie prostokąty (okienka) przed zamknięciem aplikacji.

	// Funkcja uruchamiająca nieskończoną "pętlę komunikatów", która utrzymuje program przy życiu
	int run(int show_command);
};

