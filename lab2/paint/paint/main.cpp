// To polecenie mówi kompilatorowi (dokładnie linkerowi), żeby traktował nasz program 
// jako aplikację okienkową (Windows), a nie konsolową (Console). 
// Dzięki temu nie wyskakuje nam z tyłu czarne okienko "cmd", tylko od razu ładuje się interfejs.
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wWinMainCRTStartup")

// Główny plik nagłówkowy WinAPI. Zawiera wszystkie podstawowe typy (HWND, LRESULT) i funkcje systemu Windows.
#include <windows.h>
// Dodatkowy plik z makrami, np. GET_X_LPARAM, które ułatwiają wyciąganie pozycji myszy z komunikatów.
#include <windowsx.h>
// Kontener z biblioteki standardowej C++. Będziemy w nim przechowywać listę stworzonych okien-prostokątów.
#include <vector>
// Zawiera funkcje std::min i std::max, których użyjemy do obliczania lewego-górnego rogu prostokąta.
#include <algorithm>
// Zawiera funkcję std::abs (wartość bezwzględna), potrzebną do obliczania szerokości i wysokości (zawsze na plusie).
#include <cmath>
// Biblioteka do obsługi ciągów znaków (std::wstring - string dla znaków szerokich/Unicode, których wymaga Windows).
#include <string>

class PaintApp {
private:
    // Uchwyt instancji - "dowód tożsamości" naszego programu w systemie operacyjnym.
    HINSTANCE m_instance;
    // Uchwyt do naszego głównego okna aplikacji (H-WND = Handle to Window).
    HWND m_main;
    // Pędzel (obiekt GDI), którym wypewnimy tło głównego okna.
    HBRUSH m_bg_brush;
    // Pędzel, którym pokolorujemy nasze dzieci (prostokąty).
    HBRUSH m_rect_brush;
    // Dynamiczna lista (wektor), w której trzymamy uchwyty (HWND) wszystkich narysowanych prostokątów.
    std::vector<HWND> m_rectangles;

    // Flaga (prawda/fałsz) mówiąca, czy aktualnie trzymamy wciśnięty przycisk myszy i rysujemy.
    bool m_is_drawing = false;
    // Struktura POINT przechowuje x i y. Tu zapiszemy punkt, w którym kliknęliśmy myszką (początek rysowania).
    POINT m_start_pt = { 0, 0 };

    // Główna funkcja (serce okna), która reaguje na wszystko, co się dzieje z oknem (kliknięcia, klawiatura).
    LRESULT window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        switch (message) {
            // Komunikat: Wciśnięto Lewy Przycisk Myszy.
        case WM_LBUTTONDOWN:
            // Zaznaczamy, że weszliśmy w tryb rysowania.
            m_is_drawing = true;
            // Wyciągamy dokładne współrzędne X i Y kliknięcia z parametru 'lparam' i zapisujemy jako start.
            m_start_pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };

            // Bardzo ważne: SetCapture każe Windowsowi wysyłać wszystkie ruchy myszy do NASZEGO okna,
            // nawet jeśli użytkownik wyjedzie kursorem poza jego granice. Dzięki temu prostokąt nie "urwie się".
            SetCapture(window);

            // Tworzymy nowe, małe okienko, które będzie naszym prostokątem. Klasa to systemowe "STATIC".
            // Styl WS_CHILD mówi, że jest to dziecko głównego okna, a WS_VISIBLE, że ma być od razu widoczne.
            m_rectangles.push_back(CreateWindowExW(
                0, L"STATIC", nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                m_start_pt.x, m_start_pt.y, 0, 0, // Początkowy X, Y, oraz szerokość 0 i wysokość 0.
                window, nullptr, m_instance, nullptr
            ));
            return 0; // Zwracamy 0, co oznacza, że sami obsłużyliśmy ten komunikat.

            // Komunikat: Ruch myszą po ekranie. (Dzięki SetCapture dostajemy to ciągle, dopóki trzymamy LPM).
        case WM_MOUSEMOVE:
            // Jeśli trzymamy wciśnięty przycisk (m_is_drawing) i mamy stworzony przynajmniej jeden prostokąt:
            if (m_is_drawing && !m_rectangles.empty()) {
                // Gdzie teraz jest myszka?
                int cur_x = GET_X_LPARAM(lparam);
                int cur_y = GET_Y_LPARAM(lparam);

                // API Windowsa zawsze rysuje od lewego górnego rogu.
                // Jeśli ciągniemy myszkę w lewo/w górę, musimy dynamicznie zmienić punkt startowy na mniejszy.
                int x = std::min((int)m_start_pt.x, cur_x);
                int y = std::min((int)m_start_pt.y, cur_y);

                // Szerokość i wysokość nie mogą być ujemne, więc bierzemy wartość bezwzględną (std::abs) z różnicy.
                int width = std::abs((int)m_start_pt.x - cur_x);
                int height = std::abs((int)m_start_pt.y - cur_y);

                // SetWindowPos zmienia pozycję i rozmiar aktywnego prostokąta (tego na samym końcu wektora).
                // Flagi na końcu mówią, by nie zmieniać "kolejności na wierzchu" (Z-order) ani nie aktywować okienka.
                SetWindowPos(m_rectangles.back(), nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
            }
            return 0;

            // Komunikat: Puszczono Lewy Przycisk Myszy.
        case WM_LBUTTONUP:
            if (m_is_drawing) {
                // Przestajemy rysować. Prostokąt zostaje "zapisany" (bo i tak leży w wektorze m_rectangles).
                m_is_drawing = false;
                // Zwalniamy myszkę – system wraca do normalności, inne okna mogą ją teraz śledzić.
                ReleaseCapture();
            }
            return 0;

            // Komunikat: Wciśnięto klawisz na klawiaturze.
        case WM_KEYDOWN:
            // W wparam zapisany jest kod wciśniętego klawisza. VK_BACK to Virtual Key dla klawisza Backspace.
            if (wparam == VK_BACK && !m_rectangles.empty()) {
                // DestroyWindow faktycznie zabija okienko w systemie, zwalniając pamięć. Bierzemy ostatnie z wektora.
                DestroyWindow(m_rectangles.back());
                // pop_back usuwa ten uchwyt z naszej wewnętrznej listy w C++.
                m_rectangles.pop_back();

                // Jeśli usunęliśmy w trakcie jego rysowania, musimy też puścić myszkę, żeby się nie zacięła.
                if (m_is_drawing) {
                    m_is_drawing = false;
                    ReleaseCapture();
                }

                // InvalidateRect "brudzi" obszar okna, zmuszając system do jego ponownego narysowania.
                // Ostatni argument TRUE sprawia, że tło jest też czyszczone (odświeżone kolorem tła).
                InvalidateRect(window, nullptr, TRUE);
            }
            return 0;

            // Komunikat wysyłany przez okna typu STATIC (nasze prostokąty) do rodzica przed ich narysowaniem.
        case WM_CTLCOLORSTATIC:
            // Rodzic mówi: "Słuchaj, dziecko, zamiast szarego tła, użyj tego mojego czerwonego pędzla".
            // Rzutujemy HBRUSH na INT_PTR, bo tego oczekuje Windows API w odpowiedzi.
            return reinterpret_cast<INT_PTR>(m_rect_brush);

            // Komunikat: Użytkownik wcisnął X, chce zamknąć okno.
        case WM_CLOSE:
            // Rozkazujemy zniszczenie okna.
            DestroyWindow(window);
            return 0;

            // Komunikat: Okno jest właśnie niszczone.
        case WM_DESTROY:
            // Zamykamy główną pętlę komunikatów i każemy programowi zakończyć działanie z kodem błędu 0 (sukces).
            PostQuitMessage(0);
            return 0;
        }
        // Jeśli nasz program nie był zainteresowany danym komunikatem (np. minimalizacja), odsyłamy go 
        // do domyślnej procedury systemu Windows, która zajmie się tym za nas.
        return DefWindowProcW(window, message, wparam, lparam);
    }

    // WinAPI jest napisane w języku C, który nie zna koncepcji klas i wskaźnika "this" z C++.
    // Ta statyczna metoda działa jak tłumacz: odbiera komunikat od systemu i znajduje, 
    // do którego konkretnego "obiektu" klasy PaintApp należy go przekazać.
    static LRESULT CALLBACK window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        PaintApp* app = nullptr;
        // WM_NCCREATE to pierwszy fizyczny komunikat przy tworzeniu okna.
        if (message == WM_NCCREATE) {
            // Wyciągamy wskaźnik "this" podany w CreateWindowExW...
            auto p = reinterpret_cast<LPCREATESTRUCTW>(lparam);
            app = static_cast<PaintApp*>(p->lpCreateParams);
            // ...i ukrywamy go w wewnętrznych danych samego okna pod etykietą GWLP_USERDATA.
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        else {
            // Dla każdego kolejnego komunikatu po prostu wyciągamy ten schowany wskaźnik "this".
            app = reinterpret_cast<PaintApp*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        }

        // Jeśli udało się znaleźć nasz obiekt, odpalamy jego prawdziwą metodę window_proc.
        if (app != nullptr) {
            return app->window_proc(window, message, wparam, lparam);
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

public:
    // Konstruktor naszej aplikacji. Wywołuje się od razu na starcie z funkcji main.
    PaintApp(HINSTANCE instance) : m_instance(instance), m_main(nullptr) {
        // CreateSolidBrush tworzy obiekt pędzla dla systemu Windows (makro RGB to proporcje Red, Green, Blue).
        m_bg_brush = CreateSolidBrush(RGB(30, 50, 90));       // Granatowe tło
        m_rect_brush = CreateSolidBrush(RGB(170, 70, 80));    // Czerwonawe prostokąty

        std::wstring class_name = L"Not_WM_PAINT_Class";

        // Struktura WNDCLASSEXW to taki schemat (formularz), który mówi Windowsowi, jak ma wyglądać nasza nowa klasa okna.
        WNDCLASSEXW desc = { 0 };
        desc.cbSize = sizeof(WNDCLASSEXW); // Rozmiar tej struktury w bajtach (zawsze wymagane w WinAPI).
        desc.lpfnWndProc = window_proc_static; // Podpinamy naszego statycznego "tłumacza" komunikatów.
        desc.hInstance = m_instance; // Kojarzymy to z naszym plikiem exe.
        desc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW); // Standardowy kursor strzałki.
        desc.hbrBackground = m_bg_brush; // Ustawiamy pędzel dla tła okna. System sam nim pomaluje okno.
        desc.lpszClassName = class_name.c_str(); // Nadajemy klasie nazwę, żeby potem móc jej użyć.

        // Rejestrujemy ten "schemat" w systemie Windows.
        RegisterClassExW(&desc);

        // Ustalamy styl: okno nakładkowe (zwykłe), pasek tytułowy, systemowe menu (ikona X), przycisk minimalizacji.
        // Brak tu WS_THICKFRAME i WS_MAXIMIZEBOX, co oznacza, że blokujemy zmianę rozmiaru!
        // WS_CLIPCHILDREN oznacza, że gdy główne okno się odświeża, omija obszary dzieci (prostokątów), co zapobiega migotaniu.
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;

        // Zależy nam, by czyste pole do rysowania (obszar roboczy) miało dokładnie 800x600 px.
        RECT size = { 0, 0, 800, 600 };
        // Ta funkcja dodaje wymiary ramek i paska tytułu, obliczając, jak duże musi być całe okno, by środek miał 800x600.
        AdjustWindowRectEx(&size, style, false, 0);

        // Tworzymy główne okno i zapisujemy jego uchwyt do zmiennej m_main.
        m_main = CreateWindowExW(
            0, // Dodatkowe (rozszerzone) style - brak.
            class_name.c_str(), L"Not WM_PAINT", // Nazwa klasy (taka jak w schemacie) i Tytuł okna na pasku.
            style, // Ustalony wcześniej styl.
            CW_USEDEFAULT, CW_USEDEFAULT, // Pozycja X i Y na ekranie (pozwalamy systemowi domyślnie je dobrać).
            size.right - size.left, size.bottom - size.top, // Ostateczna obliczona szerokość i wysokość całego okna.
            nullptr, nullptr, m_instance, // Brak rodzica, brak menu, nasza instancja.
            this // TO BARDZO WAŻNE: podajemy wskaźnik "this", żeby w window_proc_static móc go wyciągnąć!
        );
    }

    // Destruktor. Kiedy program się kończy, C++ go wywoła automatycznie.
    ~PaintApp() {
        // Wszystkie obiekty GDI (np. pędzle) należy bezwzględnie niszczyć, w przeciwnym razie nastąpi wyciek pamięci RAM.
        DeleteObject(m_bg_brush);
        DeleteObject(m_rect_brush);
    }

    // Ta funkcja trzyma program przy życiu, dopóki użytkownik go nie zamknie.
    int run(int show_command) {
        if (!m_main) return EXIT_FAILURE; // Zabezpieczenie, gdyby stworzenie okna się nie udało.

        // Pokazujemy okno na ekranie (domyślnie po stworzeniu jest niewidoczne).
        ShowWindow(m_main, show_command);

        MSG msg = { 0 };
        BOOL result = TRUE;
        // Pętla zdarzeń. GetMessageW cały czas czeka, aż system przyśle nam jakiś komunikat (ruch myszą, kliknięcie klawisza).
        // Wyciąga go ze strumienia, a kiedy nadejdzie sygnał WM_QUIT (wywołany w WM_DESTROY), zwróci 0 i przerwie pętlę.
        while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
            if (result == -1) return EXIT_FAILURE; // Krytyczny błąd systemu.
            TranslateMessage(&msg); // Tłumaczy niektóre wciśnięcia klawiszy na ładne znaki tekstowe.
            DispatchMessageW(&msg); // Rozsyła komunikat do właściwej procedury okna (naszego window_proc_static).
        }
        return EXIT_SUCCESS; // Koniec programu.
    }
};

// Punkt startowy każdego okienkowego programu na system Windows. Odpowiednik "int main()".
int WINAPI wWinMain(
    _In_ HINSTANCE instance,            // Nasz numer rejestracyjny aplikacji (wspomniany wcześniej).
    _In_opt_ HINSTANCE prevInstance,    // Zaszłość z 16-bitowego Windowsa (kiedyś do sprawdzania, czy program już działa), teraz zawsze jest NULL.
    _In_ LPWSTR command_line,           // Tekst z wiersza poleceń, z jakim uruchomiono program (jak argc/argv w C++).
    _In_ int show_command)              // Sugestia systemu, jak pokazać okno (np. zmaksymalizowane lub zminimalizowane).
{
    // Tworzymy obiekt naszej aplikacji i przekazujemy instancję ze startu systemu.
    PaintApp app(instance);
    // Odpalamy nieskończoną pętlę i zwracamy do Windowsa informację, jak się program zakończył.
    return app.run(show_command);
}