#include "Window.h" // Dołączamy nasz schemat, żeby móc implementować metody.

// Inicjalizujemy naszą statyczną nazwę klasy. 
// Literka 'L' przed tekstem oznacza, że to tekst Unicode (2 bajty na literę). Chroni to przed chińskimi znaczkami.
const std::wstring Window::s_class_name{ L"MojSuperSzkielet" };

// --- KONSTRUKTOR ---
// Lista inicjalizacyjna (po dwukropku): od razu zapisujemy przekazaną instancję do m_hInstance i zerujemy m_hwnd.
Window::Window(HINSTANCE instance) : m_hInstance(instance), m_hwnd(nullptr) {

    // Próbujemy zarejestrować "matrycę" okna w systemie.
    if (!register_class()) {
        return; // Jeśli rejestracja się nie uda, przerywamy konstruktor (m_hwnd pozostanie nullptr).
    }

    // Fizyczne polecenie dla Windowsa: "Stwórz okno!" (Wersja 'W' = Unicode).
    m_hwnd = CreateWindowExW(
        0,                           // Dodatkowe, rozszerzone style okna (0 = brak).
        s_class_name.c_str(),        // Podajemy nazwę matrycy (zmieniamy std::wstring na surowy tekst z C przez .c_str()).
        L"Szkielet na Laby",         // Tytuł, który pojawi się na górnym pasku okna.
        WS_OVERLAPPEDWINDOW,         // Styl standardowy: okno ma pasek, krzyżyk do zamykania i można zmieniać jego rozmiar.
        CW_USEDEFAULT, CW_USEDEFAULT,// Pozycja początkowa okna na ekranie (X i Y) - niech Windows sam zdecyduje.
        600, 400,                    // Szerokość (600) i wysokość (400) okna w pikselach.
        nullptr,                     // Uchwyt okna-rodzica (nullptr, bo to jest główne, samodzielne okno).
        nullptr,                     // Uchwyt do górnego menu (nullptr, bo nie robimy klasycznego menu Plik/Edycja itp.).
        m_hInstance,                 // Informujemy Windowsa, który program jest właścicielem tego okna.
        this                         // BARDZO WAŻNE: Przekazujemy wskaźnik 'this' (na samych siebie) jako ukryty parametr dla okna.
    );
}

// --- DESTRUKTOR ---
Window::~Window() {
    // Na razie pusto. Tutaj usuwa się stworzone grafiki, np. pędzle i czcionki.
}

// --- REJESTRACJA KLASY OKNA ---
bool Window::register_class() {
    WNDCLASSEXW desc{}; // Tworzymy pustą strukturę opisującą wygląd i zachowanie okna. Wypełniamy ją zerami ({}).

    // Sprawdzamy, czy Windows już przypadkiem nie zna tej klasy (np. przy tworzeniu drugiego okna tego samego typu).
    if (GetClassInfoExW(m_hInstance, s_class_name.c_str(), &desc) != 0) return true; // Jeśli zna, kończymy z sukcesem.

    desc.cbSize = sizeof(WNDCLASSEXW);                   // Mówimy systemowi, jak duża w pamięci jest ta struktura.
    desc.lpfnWndProc = window_proc_static;               // Podpinamy naszego STATYCZNEGO pośrednika do obsługi zdarzeń.
    desc.hInstance = m_hInstance;                        // Podpinamy uchwyt naszej aplikacji.
    desc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW); // Ładujemy standardowy kursor myszki (biała strzałka).
    desc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);     // Ustawiamy domyślny kolor tła okna (zazwyczaj biały/szary).
    desc.lpszClassName = s_class_name.c_str();           // Nadajemy tej definicji naszą nazwę ("MojSuperSzkielet").

    // Rejestrujemy wypełnioną strukturę w systemie. Jeśli zwróci 0, to znaczy że był błąd (zwracamy false).
    return RegisterClassExW(&desc) != 0;
}

// --- PĘTLA KOMUNIKATÓW (Nasz systemowy "recepcjonista") ---
int Window::run(int show_command) {
    if (!m_hwnd) return EXIT_FAILURE; // Zabezpieczenie: jeśli okno nie istnieje, wychodzimy z błędem.

    // Pokazujemy okno na ekranie (używając komendy przekazanej od systemu, np. pokaż normalnie).
    ShowWindow(m_hwnd, show_command);

    MSG msg{};    // Pusta struktura, do której Windows będzie nam wrzucał informacje o kliknięciach.
    BOOL result;  // Zmienna przechowująca wynik funkcji GetMessage.

    // GetMessageW czeka w nieskończoność na ruch myszką lub kliknięcie. Zwraca 0 tylko, gdy dostanie sygnał zamknięcia programu (WM_QUIT).
    while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
        if (result == -1) return EXIT_FAILURE; // Jeśli GetMessage zwróci -1, nastąpił krytyczny błąd systemu, przerywamy.

        TranslateMessage(&msg); // Jeśli wciśnięto klawisz, tłumaczy go na czytelny znak (np. Shift + a = 'A').
        DispatchMessageW(&msg); // Wysyła ten przetłumaczony komunikat bezpośrednio do naszej funkcji window_proc_static!
    }
    // Zwracamy kod wyjścia (przekazany w sygnale WM_QUIT).
    return static_cast<int>(msg.wParam);
}

// --- STATYCZNY POŚREDNIK (Pomost między starym C a nowym C++) ---
// Ta funkcja jest wywoływana przez samego Windowsa, gdy coś się dzieje.
LRESULT CALLBACK Window::window_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    Window* app = nullptr; // Tworzymy pusty wskaźnik na nasz obiekt.

    // Gdy okno jest w trakcie tworzenia (jeszcze zanim na dobre pojawi się na ekranie).
    if (message == WM_NCCREATE) {
        // lParam w tym konkretnym momencie przechowuje paczkę danych startowych.
        // Wyciągamy z tej paczki parametr 'lpCreateParams', który w CreateWindowExW ustawiliśmy na 'this'.
        auto p = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        app = static_cast<Window*>(p->lpCreateParams); // Przypisujemy wskaźnik 'this' do naszej zmiennej app.

        // Zapisujemy ten wskaźnik 'app' w specjalnej, ukrytej pamięci tego konkretnego okna (GWLP_USERDATA).
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else {
        // Przy każdym innym kliknięciu/ruchu myszką okno już istnieje. 
        // Po prostu odczytujemy wcześniej zapisany wskaźnik z ukrytej pamięci okna.
        app = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    // Jeśli udało się zdobyć wskaźnik na nasz fizyczny obiekt klasy...
    if (app != nullptr) {
        // ...przekierowujemy wywołanie do właściwej, nienależącej do 'static' funkcji w tym obiekcie!
        return app->window_proc(window, message, wParam, lParam);
    }

    // Jeśli z jakiegoś powodu wskaźnika nie ma, niech Windows zajmie się komunikatem sam (domyślna obsługa).
    return DefWindowProcW(window, message, wParam, lParam);
}

// --- WŁAŚCIWY MÓZG OKNA ---
// Tu programujemy zachowanie aplikacji. Mamy pełny dostęp do zmiennych klasy (jak m_hwnd).
LRESULT Window::window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) { // Sprawdzamy, jaki rodzaj komunikatu nam przysłano.

        // Komunikat WM_CREATE przychodzi DOKŁADNIE RAZ, tuż po tym, jak okno się stworzy.
    case WM_CREATE: {

        // Tworzymy przycisk (dziecko naszego głównego okna).
        CreateWindowExW(
            0,                     // Brak rozszerzonych stylów.
            L"BUTTON",             // Mówimy systemowi: "Chcę kontrolkę typu przycisk".
            L"Kliknij mnie",       // Tekst na przycisku.
            WS_VISIBLE | WS_CHILD, // WS_VISIBLE (bądź od razu widoczny), WS_CHILD (przyklej się wewnątrz głównego okna).
            50, 50,                // Odstęp 50px od lewej, 50px od góry.
            150, 40,               // Przycisk ma mieć 150px szerokości i 40px wysokości.
            window,                // Główne okno jest "rodzicem" tego przycisku.
            (HMENU)101,            // Nadajemy przyciskowi ID = 101. Używamy tego potem do sprawdzania, co kliknięto.
            m_hInstance,           // Informujemy, z jakiej aplikacji pochodzi przycisk.
            nullptr                // Brak dodatkowych danych kreacji.
        );

        // Zwracamy 0, informując Windows: "Zrozumiałem komunikat WM_CREATE i zrobiłem co trzeba".
        return 0;
    }

                  // Komunikat wysyłany, gdy klikniesz krzyżyk lub wciśniesz Alt+F4.
    case WM_CLOSE:
        DestroyWindow(window); // Nakazuje zniszczenie fizycznego okna (co wygeneruje komunikat WM_DESTROY).
        return 0;

        // Komunikat wysyłany, gdy okno jest właśnie usuwane z pamięci.
    case WM_DESTROY:
        PostQuitMessage(0); // Wysyła sygnał WM_QUIT z kodem 0 do naszej pętli GetMessage, przerywając ją na zawsze.
        return 0;
    }

    // Każdy inny komunikat (np. ruch myszką po pustym polu, wciśnięcie obcego klawisza) 
    // odsyłamy do domyślnej procedury Windowsa, żeby system obsłużył to po swojemu.
    return DefWindowProcW(window, message, wParam, lParam);
}