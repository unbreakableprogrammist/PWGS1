#include <windows.h>
#include <string>
#include <cmath>

// Stała PI potrzebna do obliczania położeń cyfr na okręgu za pomocą funkcji trygonometrycznych.
constexpr double PI = 3.14159265358979323846;

class app_dial {
private:
    // Podstawowe uchwyty aplikacji
    HINSTANCE m_instance; // Kopia instancji naszego programu w pamięci
    HWND m_main;          // Uchwyt (identyfikator) głównego okna programu
    HWND m_display;       // Uchwyt do pola tekstowego (STATIC) na samej górze (wyświetlacz numeru)
    std::wstring m_dialed_number; // Przechowuje ciąg znaków - aktualnie "wykręcony" numer telefonu

    // Zmienne matematyczne obrotu
    double m_base_angle = 0.0; // Główny, obecny kąt obrócenia całej tarczy (w radianach)

    // Zmienne do obsługi przeciągania myszką
    bool m_is_dragging = false;       // Czy lewy przycisk myszy jest aktualnie wciśnięty?
    double m_start_mouse_angle = 0.0; // Kąt, pod którym myszka złapała tarczę
    double m_start_dial_angle = 0.0;  // Kąt tarczy w momencie jej złapania

    // Położenie i rozmiar tarczy
    int m_center_x = 300; // Środek tarczy na osi X
    int m_center_y = 350; // Środek tarczy na osi Y (lekko w dół, by zrobić miejsce na wyświetlacz)
    int m_radius = 150;   // Promień tarczy, na którym układają się kółka z cyframi

    // Zasoby GDI (Pędzle do tła i wypełnień, Pióra do konturów)
    HBRUSH m_bg_brush;    // Pędzel do tła całego okna
    HBRUSH m_hole_brush;  // Pędzel do dziurek (kółek z cyframi)
    HBRUSH m_dial_brush;  // Pędzel do obudowy (tła) samej tarczy
    HPEN m_dial_pen;      // Pióro do rysowania ramek tarczy

    static const std::wstring s_class_name;

    // --- PROCEDURA STATYCZNA (Most pomiędzy Windowsem a klasą C++) ---
    static LRESULT CALLBACK window_proc_static(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        app_dial* app = nullptr;
        if (message == WM_NCCREATE) {
            // W momencie pierwszego tworzenia okna, "wszywamy" wskaźnik do naszej klasy (this)
            // w pamięć wewnętrzną okna za pomocą SetWindowLongPtrW [cite: 145-147]
            auto p = reinterpret_cast<LPCREATESTRUCTW>(lparam);
            app = static_cast<app_dial*>(p->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        else {
            // Dla każdego kolejnego komunikatu wyciągamy ten wskaźnik z okna [cite: 151-154]
            app = reinterpret_cast<app_dial*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        }

        // Jeśli udało się wydobyć wskaźnik, przekazujemy komunikat do niestatycznej metody window_proc [cite: 155-158]
        if (app != nullptr) {
            return app->window_proc(window, message, wparam, lparam);
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

    // --- GŁÓWNA PROCEDURA OKNA (Logika biznesowa i renderowanie) ---
    LRESULT window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        switch (message) {

        case WM_ERASEBKGND:
            // KLUCZOWE DLA PODWÓJNEGO BUFOROWANIA!
            // Zwracając 1 oszukujemy Windowsa, że tło zostało już zamazane.
            // Dzięki temu system nie maluje go na biało/szaro i likwidujemy efekt "migania" (flickeringu).
            return 1;

        case WM_PAINT: {
            // Sekcja zajmująca się ręcznym rysowaniem całej grafiki GDI 
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(window, &ps); // Pobiera główny kontekst urządzenia (HDC - na ekran) [cite: 772-773, 806-807]

            RECT rc;
            GetClientRect(window, &rc); // Pobiera rozmiary wewnątrz naszego okna 

            // --- POCZĄTEK PODWÓJNEGO BUFOROWANIA ---
            // Tworzymy wirtualne "płótno" (mem_dc) w pamięci RAM, kopiując właściwości HDC ekranu
            HDC mem_dc = CreateCompatibleDC(hdc);
            // Tworzymy wirtualną bitmapę (obraz) o rozmiarach naszego okna
            HBITMAP mem_bitmap = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
            // Podpinamy bitmapę pod nasze wirtualne płótno, żebyśmy mogli po nim rysować [cite: 811-812]
            HBITMAP old_bitmap = (HBITMAP)SelectObject(mem_dc, mem_bitmap);

            // Najpierw ręcznie malujemy całe wirtualne płótno na kolor tła (zamiast WM_ERASEBKGND)
            FillRect(mem_dc, &rc, m_bg_brush); // [cite: 901]

            // -- Rysowanie wyglądu tarczy --

            // 1. Metalowy wskaźnik na górze ("haczyk", czyli blokada tarczy)
            HPEN hook_pen = CreatePen(PS_SOLID, 8, RGB(220, 220, 225)); // Grube na 8px, szare pióro
            HPEN old_pen = (HPEN)SelectObject(mem_dc, hook_pen); // [cite: 808-812]
            MoveToEx(mem_dc, m_center_x, m_center_y - m_radius - 60, nullptr); // Skok na samą górę ponad tarczą
            LineTo(mem_dc, m_center_x, m_center_y - m_radius - 10); // Rysowanie linii prostej w dół
            SelectObject(mem_dc, old_pen);
            DeleteObject(hook_pen); // Od razu usuwamy niepotrzebne już pióro by nie było wycieków pamięci! [cite: 837-838]

            // 2. Główna obudowa tarczy (Największe koło)
            SelectObject(mem_dc, m_dial_brush);
            SelectObject(mem_dc, m_dial_pen);
            int outer_r = m_radius + 40; // Promień zewnętrzny to promień dziurek + 40px
            // Rysuje elipsę ograniczoną kwadratem (środek +/- promień)
            Ellipse(mem_dc, m_center_x - outer_r, m_center_y - outer_r, m_center_x + outer_r, m_center_y + outer_r);

            // 3. Wewnętrzny, nieruchomy środek tarczy
            int inner_r = m_radius - 40; // Mniejszy okrąg w środku
            SelectObject(mem_dc, m_bg_brush);
            Ellipse(mem_dc, m_center_x - inner_r, m_center_y - inner_r, m_center_x + inner_r, m_center_y + inner_r);

            // Przygotowanie czcionki do cyfr
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            HFONT oldFont = (HFONT)SelectObject(mem_dc, hFont);

            SelectObject(mem_dc, m_hole_brush); // Pędzel dla "dziurek" z cyframi
            SelectObject(mem_dc, m_dial_pen);
            SetBkMode(mem_dc, TRANSPARENT); // Ustawiamy przezroczyste tło pod samą czcionką tekstu [cite: 379-380]
            SetTextColor(mem_dc, RGB(255, 255, 255)); // Kolor czcionki - biały

            // 4. Rysowanie 10 dziurek z cyframi na okręgu
            for (int i = 0; i < 10; ++i) {
                // Równanie parametryczne okręgu z przesunięciem o m_base_angle (kręcenie)
                // Kąt to i * (pełne koło 2*PI / 10 części) + nasz obrót
                double angle = m_base_angle + (i * 2 * PI) / 10.0;

                // Obliczamy środek każdej dziurki
                int center_x = m_center_x + (int)(m_radius * cos(angle));
                int center_y = m_center_y + (int)(m_radius * sin(angle));

                // Pozycja górnego-lewego rogu do narysowania okręgu
                int x = center_x - 20;
                int y = center_y - 20;

                // Rysujemy dziurkę z cyfrą
                Ellipse(mem_dc, x, y, x + 40, y + 40);

                // Konwersja int na ciąg znaków (np. 0, 1, 2)
                std::wstring text = std::to_wstring(i);
                RECT text_rc = { x, y, x + 40, y + 40 };
                // Rysujemy wyśrodkowany tekst idealnie wewnątrz prostokąta text_rc [cite: 903]
                DrawTextW(mem_dc, text.c_str(), -1, &text_rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            }

            SelectObject(mem_dc, oldFont);

            // --- KONIEC PODWÓJNEGO BUFOROWANIA ---
            // "Wklejamy" naszą przygotowaną w całości bitmapę z RAM-u na ekran monitora.
            // Ponieważ robimy to jako jedną wielką paczkę pikseli (BitBlt), nie ma żadnego migania!
            BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, mem_dc, 0, 0, SRCCOPY);

            // Niezbędne sprzątanie po GDI, zapobiegające totalnym wyciekom pamięci! [cite: 837-840]
            SelectObject(mem_dc, old_bitmap);
            DeleteObject(mem_bitmap);
            DeleteDC(mem_dc);

            EndPaint(window, &ps); // Koniec malowania ekranu [cite: 772-773, 834]
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            // Koloruje zewnętrzne, statyczne okienko tekstowe z wybranym numerem na górze programu [cite: 379-380]
            HDC hdc = reinterpret_cast<HDC>(wparam);
            HWND hCtl = (HWND)lparam;

            SetBkMode(hdc, TRANSPARENT); // Usunięcie brzydkiego szarego kwadratu spod tekstu

            // Jeśli to nasz wyświetlacz "Wybierz numer...", kolorujemy na jasny oranż i oddajemy ciemne tło okna
            if (hCtl == m_display) {
                SetTextColor(hdc, RGB(247, 93, 60));
                return reinterpret_cast<INT_PTR>(m_bg_brush);
            }

            SetTextColor(hdc, RGB(255, 255, 255));
            return reinterpret_cast<INT_PTR>(m_hole_brush); // [cite: 403]
        }

        case WM_LBUTTONDOWN: {
            // Moment złapania tarczy myszką
            int mouse_x = (short)LOWORD(lparam);
            int mouse_y = (short)HIWORD(lparam);

            // Obliczamy matematyczny wektor kąta pomiędzy środkiem tarczy a naszą myszką za pomocą Arc Tangens
            m_start_mouse_angle = atan2(mouse_y - m_center_y, mouse_x - m_center_x);
            // Zapisujemy pod jakim kątem znajdowała się tarcza, by dodać do niej późniejsze przesunięcie
            m_start_dial_angle = m_base_angle;

            m_is_dragging = true; // Zaczynamy ciągnięcie
            SetCapture(window);   // Zatrzymuje mysz w oknie, żeby gwałtowny ruch "nie zgubił" kliknięcia
            return 0;
        }

        case WM_MOUSEMOVE: {
            // Ruch tarczy (Ciągnięcie)
            if (m_is_dragging) {
                int mouse_x = (short)LOWORD(lparam);
                int mouse_y = (short)HIWORD(lparam);

                // Nowy, obecny kąt myszki
                double current_mouse_angle = atan2(mouse_y - m_center_y, mouse_x - m_center_x);
                // Obliczenie wektora "różnicy", czyli o ile przekręciliśmy myszkę
                double diff = current_mouse_angle - m_start_mouse_angle;

                // TZW. GIMBAL LOCK FIX: Funkcja atan2 ma punkt przeskoku między Pi a -Pi (lewa storna osi X).
                // Te dwie pętle naprawiają ten przeskok, żeby tarcza nie "wariowała" przy zrobieniu pełnego koła.
                while (diff > PI)  diff -= 2 * PI;
                while (diff < -PI) diff += 2 * PI;

                // Zapisujemy nowy kąt całej tarczy
                m_base_angle = m_start_dial_angle + diff;

                // Wymusza wywołanie funkcji WM_PAINT. 
                // UWAGA: Trzeci argument FALSE to wyłączenie wywoływania WM_ERASEBKGND.
                // Dzięki temu działa nasze podwójne buforowanie i nic nie klatkuje.
                InvalidateRect(window, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            // Zwalniamy tarczę
            if (m_is_dragging) {
                m_is_dragging = false;
                ReleaseCapture();

                // Sprawdzamy czy dociągnęliśmy tarczę (czy numer najechał na "haczyk")
                for (int i = 0; i < 10; ++i) {
                    double angle = m_base_angle + (i * 2 * PI) / 10.0;

                    // Normalizuje dowolny kąt do zakresu -PI do PI (np. wycina zbedne pełne obroty)
                    double normalized_angle = atan2(sin(angle), cos(angle));

                    // Idealnie na 12 godzinie, czyli punkt startowy haczyka (W informatyce oś Y rośnie w dół, więc -PI/2)
                    double top_angle = -PI / 2.0;

                    // Jeśli kąt cyfry (normalized_angle) jest blisko haczyka (margines błędu +/- 0.3 radiana ok. 17 stopni)
                    if (abs(normalized_angle - top_angle) < 0.3) {
                        m_dialed_number += std::to_wstring(i); // Dodaj string wykręconego numeru (np. '7')
                        SetWindowTextW(m_display, m_dialed_number.c_str()); // Wypisz nową treść na ekran
                        break; // Numer dopasowany, nie ma sensu sprawdzać pozostałych cyfr
                    }
                }
            }
            return 0;
        }

        case WM_DESTROY: {
            // Bezwzględny obowiązek! Przed wyjściem niszczymy wszystkie zarezerwowane wcześniej w systemie zasoby graficzne! [cite: 20-21, 847]
            DeleteObject(m_bg_brush);
            DeleteObject(m_hole_brush);
            DeleteObject(m_dial_brush);
            DeleteObject(m_dial_pen);
            PostQuitMessage(EXIT_SUCCESS); // Ostrzega pętle GetMessage, że trzeba zakończyć działanie [cite: 165-166, 176]
            return 0;
        }
        }
        return DefWindowProcW(window, message, wparam, lparam); // [cite: 160]
    }

    bool register_class() {
        // Rejestrowanie formatki okna w systemie przed jego stworzeniem [cite: 97-101]
        WNDCLASSEXW desc{};
        if (GetClassInfoExW(m_instance, s_class_name.c_str(), &desc) != 0) return true; // [cite: 105-106]

        desc.cbSize = sizeof(WNDCLASSEXW);
        desc.lpfnWndProc = window_proc_static; // Wskazanie na naszą "mostkową" statyczną funkcje [cite: 108]
        desc.hInstance = m_instance; // [cite: 109]
        desc.hCursor = LoadCursorW(nullptr, L"IDC_ARROW"); // Podstawowy kursor myszy [cite: 110]
        desc.hbrBackground = m_bg_brush; // Automatycznie ustawiło by tło, ale zignorowaliśmy to zwracając 1 w WM_ERASEBKGND [cite: 111]
        desc.lpszClassName = s_class_name.c_str(); // [cite: 111]

        return RegisterClassExW(&desc) != 0; // [cite: 112]
    }

    HWND create_window() {
        // Tworzenie głównego okna programu
        HWND window = CreateWindowExW(
            0,
            s_class_name.c_str(), L"Retro Tarcza",
            WS_OVERLAPPEDWINDOW, // Typowe okno Windows z ramką [cite: 115, 123-124]
            CW_USEDEFAULT, 0, 600, 650, // Wymiary (X: domyślny, Y:0, szer:600, wys:650)
            nullptr, nullptr, m_instance, this // Zmienna 'this' idzie jako ostatni parametr prosto do WM_NCCREATE! [cite: 117, 128]
        );

        // Wyciągamy standardową czcionkę okien z systemu
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        // Tworzymy napis na samej górze (Nasza tablica z numerem telefonu)
        m_display = CreateWindowExW(
            0, L"STATIC", L"Przeciagnij cyfre na sama gore...",
            WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
            100, 50, 400, 40,
            window, nullptr, m_instance, nullptr // [cite: 352-355]
        );
        SendMessage(m_display, WM_SETFONT, (WPARAM)hFont, TRUE); // Aplikuje pobraną wcześniej czcionkę

        return window;
    }

public:
    app_dial(HINSTANCE instance) : m_instance{ instance }, m_main{} {
        // Konstruktor: Tutaj jednorazowo alokujemy pędzle z wybranymi RGB [cite: 388-390, 863]
        m_bg_brush = CreateSolidBrush(RGB(40, 44, 52));
        m_hole_brush = CreateSolidBrush(RGB(247, 93, 60));
        m_dial_brush = CreateSolidBrush(RGB(65, 70, 80));
        m_dial_pen = CreatePen(PS_SOLID, 3, RGB(100, 105, 115));

        register_class(); // [cite: 183]
        m_main = create_window(); // [cite: 187]
    }

    int run(int show_command) {
        ShowWindow(m_main, show_command); // Wymusza pokazanie się okna na pulpicie [cite: 191]
        MSG msg{};
        BOOL result = TRUE;
        // Pętla GetMessage - Serce programu przechwytujące ruchy myszą, klawiaturą itp. [cite: 194-201]
        while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
            if (result == -1) return EXIT_FAILURE;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return EXIT_SUCCESS; // Zamykamy program [cite: 202]
    }
};

const std::wstring app_dial::s_class_name{ L"DialAppClass" }; // Inicjalizacja stałej z nazwą okna [cite: 96]

// Główny punkt wejścia systemu Windows (zamiennik int main() dla API) [cite: 54-56]
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE /*prevInstance*/, LPWSTR /*command_line*/, int show_command) {
    app_dial app{ instance }; // Tworzymy klasę i ruszamy! [cite: 205]
    return app.run(show_command);
}