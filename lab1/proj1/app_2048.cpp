#include "app_2048.h"
#include<stdexcept> // biblioteka do obslugi wyjatkow, czyli sytuacji gdy cos idzie nie tak, np nie udaje sie zarejestrowac klasy okna, nie udaje sie stworzyc okna, itp

// pole class_name jest statyczne, wiec musi byc zainicjalizowane poza definicja klasy, czyli tutaj w pliku app_2048.cpp
// jeszcze zanim stworzymy obiekt tej klasy 
std::wstring const app_2048::s_class_name{ L"2048 Window" };

bool app_2048::register_class() { // implementujemy klase 
	// struktura WNDCLASSEXW jest struktura danych, ktora zawiera informacje o klasie okna, czyli rodzaju okna, jego wygladzie, sposobie obslugi zdarzen, itp
	WNDCLASSEXW desc{}; // desc - nazwa zmiennej , {} - inicjalizuemy ja wartosciami domyslnymi 
	// GetClassInfoExW - pobierz informacje o klasie
	// m_instance - szukaj w obrebie tej instancji aplikacji, czyli tego procesu, ktory jest uruchamiany
	// s_class_name.c_str() - szukaj klasy o nazwie s_class_name (="2048 Window"), czyli tej klasy, ktora chcemy zarejestrowac
	// &desc - zapisz informacje o tej klasie w strukturze desc, czyli w tej zmiennej, ktora jest przekazywana jako argument do funkcji GetClassInfoExW
	// Funkcja GetClassInfoExW zwraca wartość 0 (czyli fałsz/błąd), jeśli nie znajdzie takiej klasy. Zatem warunek != 0 oznacza: "Znaleziono klasę! Ona już tu jest!
	// to oznacza ze nie ma co wypelniach formularza i zwracamy true
	if (GetClassInfoExW(m_instance, s_class_name.c_str(), &desc) != 0) return true; 

	desc = { .cbSize = sizeof(WNDCLASSEXW), // cbSize - dokladny rozmiar naszego formularza
		.lpfnWndProc = window_proc_static, // lpfnWndProc - wskaznik do funkcji obslugi zdarzen, czyli tej funkcji, ktora bedzie wywolywana przez system operacyjny, gdy wystapi zdarzenie zwiazane z oknem gry, np klikniecie myszka, nacisniecie klawisza, itp
		.hInstance = m_instance, // hInstance - uchwyt do instancji aplikacji, czyli identyfikator procesu, ktory jest przekazywany przez system operacyjny do funkcji wWinMain, gdy aplikacja jest uruchamiana
		.hCursor = LoadCursorW(nullptr, IDC_ARROW), // hCursor - uchwyt do kursora myszy, czyli tej strzalki, ktora jest wyswietlana na ekranie, gdy poruszamy myszka, itp. LoadCursorW - funkcja do ladowania kursora, nullptr - oznacza ze chcemy zaladowac kursor systemowy, IDC_ARROW - oznacza ze chcemy zaladowac kursor w ksztalcie strzalki
		.lpszClassName = s_class_name.c_str() // lpszClassName - nazwa klasy okna, czyli tej klasy, ktora chcemy zarejestrowac
	};
	return RegisterClassExW(&desc) != 0; // RegisterClassExW - funkcja do rejestracji klasy okna, &desc - struktura z informacjami o klasie okna, ktora chcemy zarejestrowac. Funkcja ta zwraca wartosc 0 (czyli falsz/blad), jesli nie udalo sie zarejestrowac klasy. Zatem warunek != 0 oznacza: "Klasa została zarejestrowana pomyślnie!"
}

HWND app_2048::create_window() { // nadpisujemy funkcje create_window, ktora bedzie odpowiedzialna za tworzenie okna gry, HWND - uchwyt do okna
	return CreateWindowExW(
		0, // dwExStyle - np przezroczyste okno, itp, 0 - brak dodatkowych stylów
		s_class_name.c_str(), // lpClassName - nazwa klasy okna, czyli tej klasy, ktora chcemy stworzyc
		L"2048", // lpWindowName - tytul okna, czyli tekst wyswietlany na pasku tytulu okna
		WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_BORDER | WS_MINIMIZEBOX, // dwStyle - styl okna, WS_OVERLAPPEDWINDOW - standardowy styl okna z ramką, paskiem tytułu, WS_SYSMENU - okno ma menu systemowe (ikona w lewym górnym rogu), WS_CAPTION - okno ma pasek tytułu, WS_BORDER - okno ma ramkę, WS_MINIMIZEBOX - okno ma przycisk minimalizacji
		CW_USEDEFAULT, CW_USEDEFAULT, // x, y - pozycja okna na ekranie, CW_USEDEFAULT - system sam wybierze domyślną pozycję
		CW_USEDEFAULT, CW_USEDEFAULT, // nWidth, nHeight - rozmiar okna, CW_USEDEFAULT - system sam wybierze domyślny rozmiar
		nullptr, nullptr, m_instance, this); // hWndParent - uchwyt do okna nadrzędnego (jeśli jest), nullptr - brak nadrzędnego okna; hMenu - uchwyt do menu (jeśli jest), nullptr - brak menu; hInstance - uchwyt do instancji aplikacji; lpParam - dodatkowe dane przekazywane do funkcji obsługi zdarzeń (jeśli są), this - przekazujemy wskaźnik do bieżącego obiektu klasy app_2048, który będzie dostępny w funkcji obsługi zdarzeń, dzięki czemu będziemy mogli odwoływać się do danych i metod tego obiektu w tej funkcji
}