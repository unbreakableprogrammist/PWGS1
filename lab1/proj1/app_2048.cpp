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
		CW_USEDEFAULT, 0, // x, y - pozycja okna na ekranie, CW_USEDEFAULT - system sam wybierze domyślną pozycję
		CW_USEDEFAULT, 0, // nWidth, nHeight - rozmiar okna, CW_USEDEFAULT - system sam wybierze domyślny rozmiar
		nullptr, nullptr, m_instance, this); // hWndParent - uchwyt do okna nadrzędnego (jeśli jest), nullptr - brak nadrzędnego okna; hMenu - uchwyt do menu (jeśli jest), nullptr - brak menu; hInstance - uchwyt do instancji aplikacji; lpParam - dodatkowe dane przekazywane do funkcji obsługi zdarzeń (jeśli są), this - przekazujemy wskaźnik do bieżącego obiektu klasy app_2048, który będzie dostępny w funkcji obsługi zdarzeń, dzięki czemu będziemy mogli odwoływać się do danych i metod tego obiektu w tej funkcji
}

LRESULT CALLBACK app_2048::window_proc_static( // LRESULT CALLBACK - windows zwraca nam result za kazdym razem jak uzytkownik cos zrobi
	HWND window, // uchwyt do okna, czyli identyfikator okna, ktory jest przekazywany przez system operacyjny do funkcji obslugi zdarzen, gdy wystapi zdarzenie zwiazane z tym oknem
	UINT message, // kod zdarzenia, czyli liczba calkowita reprezentujaca rodzaj zdarzenia, np klikniecie myszka, nacisniecie klawisza, itp
	WPARAM wparam, // zmienne na argumety szczegolowe dotyczace zdarzenia, np kod klawisza, pozycja kursora, itp
	LPARAM lparam)
{
	app_2048* app = nullptr; // tworzymy wskaznik do obiektu klasy app_2048, ktory bedzie przechowywal informacje o tym, ktory obiekt klasy app_2048 jest zwiazany z danym oknem gry, czyli ktory obiekt jest odpowiedzialny za obsluge zdarzen zwiazanych z tym oknem gry
	// Ten komunikat przychodzi tylko raz, w momencie "narodzin" okna
	if (message == WM_NCCREATE)
	{
		// Wyciągamy nasz wskaźnik "this" ukryty w lparam przez CreateWindowExW
		CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
		app = static_cast<app_2048*>(create->lpCreateParams);

		// "Przyklejamy" ten wskaźnik do naszego okna na stałe
		SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
	}
	else 
	{
		// dla innego po prostu odczytujemy to co "przykleiliśmy" wcześniej do okna (przy tworzeniu okna)
		app = reinterpret_cast<app_2048*>(GetWindowLongPtrW(window, GWLP_USERDATA));
	}
	//Jeśli udało się znaleźć nasz obiekt, przekazujemy mu komunikat
	if (app != nullptr)
	{
		return app->window_proc(window, message, wparam, lparam);
	}

	// 5. Jeśli nie (np. przed przypisaniem wskaźnika), niech Windows zajmie się tym sam
	return DefWindowProcW(window, message, wparam, lparam);
}

LRESULT app_2048::window_proc(
	HWND window, UINT message, WPARAM wparam, LPARAM lparam // message typ, lparam , wparam. zalezy od zdarzenia ktore robi co
)
{
	switch (message) {
		// dodanie obslugi klawiszy:
	case WM_KEYDOWN:  // wciśnięcie klawisza
		switch (wparam) {  // wparam to kod klawisza
		case VK_UP:    // strzałka w górę
			SetWindowTextW(window, L"Wcisnąłeś górę!"); /// Zmiana tytul okna
			return 0;
		case VK_DOWN:
			SetWindowTextW(window, L"Wcisnąłeś dół!");
			return 0;
		case VK_LEFT:
			return 0;
		case VK_RIGHT:
			return 0;
		}
		return 0;
		// lewy przycisk myszy
	case WM_LBUTTONDOWN: {
		MessageBoxW(window, L"Kliknąłeś!", L"Tytuł", MB_OK); // MessageBox
		MessageBeep(MB_OK); // dzwiek
		int x = LOWORD(lparam); // LOWORD mlodsze bity (0 - 32)
		int y = HIWORD(lparam); // starsze (33 - 64)
		HDC hdc = GetDC(window);        // uchwyt do "płótna" okna
		Ellipse(hdc, x - 10, y - 10, x + 10, y + 10);  // kółko w miejscu kliknięcia
		ReleaseDC(window, hdc);         // zawsze zwalniaj!
		return 0;
	}

	case WM_CLOSE: // uzytkownik kliknal X
		DestroyWindow(window);
		return 0;
	case WM_DESTROY: // przychodzi po DestroyWindow
		if (window == m_main) // czy window to m_main
			PostQuitMessage(EXIT_SUCCESS);
		return 0;
	}
	return DefWindowProcW(window, message, wparam, lparam); // oddanie kontroli Windowsowi
}

// konstuktor klasy, 
app_2048::app_2048(HINSTANCE instance) : m_instance{ instance }, m_main{}
{
	register_class(); // rejestruje klase okna - jak ma wygladac i zachowywac sie okno
	m_main = create_window(); // tworzy okno i zapisuje jego uchwyt
}

int app_2048::run(int show_command) { // glowna petla aplikacji
	ShowWindow(m_main, show_command); // Pokazuje okno
	MSG msg{};
	BOOL result = TRUE;
	while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) { // czeka na kolejne zdarzenie i zapisuje je do msg, result = 0 <=> PostQuitMessage()
		if (result == -1)
			return EXIT_FAILURE;

		TranslateMessage(&msg); // czyta klawisze
		DispatchMessageW(&msg); // wysyla zdarzenie do window_proc
	}
	return EXIT_SUCCESS;
}