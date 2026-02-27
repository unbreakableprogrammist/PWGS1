#pragma once // robi cos takiego ze raz includuje tylko biblioteki, np windows.h zaladuje raz nawet jestli uzyjemy app_2048 w paru miejscach 
#include <windows.h>
#include<string>
using namespace std;

class app_2048 {
private:
	bool register_class(); // deklarujemy funckje 
	// ponizej deklarujemy zmienna s_class_name
	// static - jest to zmienna statyczna, czyli taka ktora jest wspolna dla wszystkich obiektow tej klasy, a nie dla kazdego obiektu osobno
	// wstring - jest to typ danych reprezentujacy lancuch znakow szerokich (wide string), czyli taki ktory moze przechowywac znaki z wiekszej ilosci jezykow, np polskie znaki diakrytyczne
	//const - oznacza ze ta zmienna jest stala, czyli jej wartosc nie moze byc zmieniona po jej zainicjalizowaniu
	static wstring const s_class_name;
	// ogolna funkcja ktora wywluje windows gdy jest jakies zdarzenie zwiazane z oknem, np klikniecie myszka, nacisniecie klawisza, itp
	static LRESULT CALLBACK window_proc_static(
		HWND window, // uchwyt do okna, czyli identyfikator okna, ktory jest przekazywany przez system operacyjny do funkcji obslugi zdarzen, gdy wystapi zdarzenie zwiazane z tym oknem
		UINT message, // kod zdarzenia, czyli liczba calkowita reprezentujaca rodzaj zdarzenia, np klikniecie myszka, nacisniecie klawisza, itp
		WPARAM wparam, // dodatkowe informacje o zdarzeniu, ktore sa przekazywane jako argumenty do funkcji obslugi zdarzen, np kod klawisza, pozycja kursora, itp
		LPARAM lparam // dodatkowe informacje o zdarzeniu, ktore sa przekazywane jako argumenty do funkcji obslugi zdarzen, np kod klawisza, pozycja kursora, itp
		);
	HWND create_window(); // deklarujemy funkcje create_window, ktora bedzie odpowiedzialna za tworzenie okna gry
	HINSTANCE m_instance; // deklarujemy zmienna m_instance, ktora bedzie przechowywac uchwyt do instancji aplikacji, czyli identyfikator procesu, ktory jest przekazywany przez system operacyjny do funkcji wWinMain, gdy aplikacja jest uruchamiana
	HWND m_main; // deklarujemy zmienna m_main, ktora bedzie przechowywac uchwyt do glownego okna gry, czyli identyfikator okna, ktory jest zwracany przez funkcje create_window, gdy okno jest tworzone
public:
	app_2048(HINSTANCE instance); // deklarujemy konstruktor klasy app_2048, ktory bedzie odpowiedzialny za inicjalizacje aplikacji, czyli rejestracje klasy okna, tworzenie glownego okna gry, itp
	int run(int show_command); // deklarujemy funkcje run, ktora bedzie odpowiedzialna za uruchomienie glownej petli zdarzen gry, czyli odbieranie i obsluge zdarzen zwiazanych z oknem gry, itp
};

