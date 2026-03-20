#include "Window.h"
#include <commctrl.h> // Biblioteka niezbędna do działania zaawansowanych kontrolek, takich jak suwak (Trackbar)
#include <string>     // Do łatwego sklejania i konwertowania tekstów (std::wstring)

// MAGIA VISUAL STUDIO: Mówimy kompilatorowi: "Hej, dołącz do mojego programu plik comctl32.lib".
// Bez tego program skompilowałby się, ale funkcja tworząca suwak po prostu by zawiodła.
#pragma comment(lib, "comctl32.lib")

// Nazwa naszej "matrycy" okna (wymagane przez system do rejestracji)
const std::wstring Window::s_class_name{ L"GradientSelectorClass" };

// --- KONSTRUKTOR (Tworzenie okna głównego) ---
Window::Window(HINSTANCE instance) : m_hInstance(instance), m_hwnd(nullptr) {
    // Najpierw rejestrujemy zasady działania okna
    if (!register_class()) return;

    // A potem fizycznie tworzymy okno na ekranie
    m_hwnd = CreateWindowExW(
        0, s_class_name.c_str(),
        L"Gradient Editor - Pro Version", // Tytuł okna (zostawiony po angielsku)
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        650, 470, nullptr, nullptr, m_hInstance, this
    );
}

Window::~Window() {}

// --- REJESTRACJA KLASY ---
bool Window::register_class() {
    WNDCLASSEXW desc{};
    if (GetClassInfoExW(m_hInstance, s_class_name.c_str(), &desc) != 0) return true;

    desc.cbSize = sizeof(WNDCLASSEXW);
    desc.lpfnWndProc = window_proc_static; // Podpinamy pośrednika dla komunikatów
    desc.hInstance = m_hInstance;
    desc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW); // Zwykły kursor

    // ULEPSZENIE: Ustawiamy tło głównego okna na czystą biel (COLOR_WINDOW + 1 to standardowa biel okien Windows)
    desc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    desc.lpszClassName = s_class_name.c_str();

    return RegisterClassExW(&desc) != 0;
}

// --- PĘTLA KOMUNIKATÓW ---
int Window::run(int show_command) {
    if (!m_hwnd) return EXIT_FAILURE;
    ShowWindow(m_hwnd, show_command); // Pokazujemy okno na ekranie

    MSG msg{};
    BOOL result;
    // Czekamy na ruch myszką, kliknięcie klawiatury itd.
    while ((result = GetMessageW(&msg, nullptr, 0, 0)) != 0) {
        if (result == -1) return EXIT_FAILURE;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

// --- STATYCZNY POŚREDNIK (Most między systemem a naszą klasą C++) ---
LRESULT CALLBACK Window::window_proc_static(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    Window* app = nullptr;
    // Zapisujemy i odczytujemy ukryty wskaźnik 'this', żeby wiedzieć, do którego obiektu klasy wysłać kliknięcie
    if (message == WM_NCCREATE) {
        auto p = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        app = static_cast<Window*>(p->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    else {
        app = reinterpret_cast<Window*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    // Odsyłamy komunikat do właściwej procedury obiektu
    if (app != nullptr) return app->window_proc(window, message, wParam, lParam);
    return DefWindowProcW(window, message, wParam, lParam);
}

// --- GŁÓWNA PROCEDURA OKNA (Tutaj piszemy logikę interfejsu!) ---
LRESULT Window::window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {

        // --- TWORZENIE ELEMENTÓW (Komunikat wywoływany raz przy starcie) ---
    case WM_CREATE: {

        // 1. RAMKA GRUPUJĄCA (Dla estetyki, styl BS_GROUPBOX)
        CreateWindowExW(0, L"BUTTON", L"Gradient Settings", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
            10, 10, 250, 400, window, (HMENU)200, m_hInstance, nullptr);

        // 2. PRZYCISKI OPCJI - RADIO BUTTONS (Wybór jedna z wielu)
        CreateWindowExW(0, L"BUTTON", L"Linear Gradient", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            30, 40, 200, 20, window, (HMENU)201, m_hInstance, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Radial Gradient", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            30, 70, 200, 20, window, (HMENU)202, m_hInstance, nullptr);

        // Nakazujemy systemowi "kliknąć" i zaznaczyć pierwszy przycisk Liniowy (ID 201) jako domyślny
        CheckRadioButton(window, 201, 202, 201);

        // 3. POLE ZAZNACZENIA - CHECKBOX (Opcja włącz/wyłącz)
        CreateWindowExW(0, L"BUTTON", L"Invert Colors", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            30, 110, 200, 20, window, (HMENU)203, m_hInstance, nullptr);

        // 4. SUWAK I JEGO PODPIS
        CreateWindowExW(0, L"STATIC", L"Effect Strength:", WS_VISIBLE | WS_CHILD,
            30, 150, 200, 20, window, (HMENU)204, m_hInstance, nullptr);

        HWND hSlider = CreateWindowExW(0, TRACKBAR_CLASS, L"", WS_VISIBLE | WS_CHILD | TBS_AUTOTICKS | TBS_HORZ,
            25, 170, 210, 40, window, (HMENU)205, m_hInstance, nullptr);

        // Ustawiamy parametry suwaka (Od 0 do 100, pozycja startowa: 50)
        SendMessageW(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        SendMessageW(hSlider, TBM_SETPOS, TRUE, 50);

        // 5. ULEPSZENIE: POLE DO WPISYWANIA TEKSTU (Klasa EDIT)
        CreateWindowExW(0, L"STATIC", L"Angle:", WS_VISIBLE | WS_CHILD,
            30, 230, 50, 20, window, (HMENU)208, m_hInstance, nullptr);

        // WS_EX_CLIENTEDGE - specjalny rozszerzony styl dający wklęsłą ramkę (jak u Twojego kolegi)
        // ES_NUMBER - automatycznie blokuje możliwość wpisania czegokolwiek poza cyframi
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"90", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER,
            80, 228, 50, 22, window, (HMENU)209, m_hInstance, nullptr);

        // 6. GŁÓWNY PRZYCISK ZATWIERDZAJĄCY
        CreateWindowExW(0, L"BUTTON", L"Draw Gradient!", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            50, 280, 150, 50, window, (HMENU)206, m_hInstance, nullptr);

        // 7. MIEJSCE NA RYSUNEK (Zwykłe pole tekstowe ze stylami centrującymi tekst i dodającymi ramkę)
        CreateWindowExW(0, L"STATIC", L"Your gradient will appear here...", WS_VISIBLE | WS_CHILD | WS_BORDER | SS_CENTER | SS_CENTERIMAGE,
            280, 17, 330, 393, window, (HMENU)207, m_hInstance, nullptr);

        // 8. ULEPSZENIE: ŁADNA CZCIONKA DLA WSZYSTKICH KONTROLEK
        // EnumChildWindows to potężna funkcja, która znajduje wszystkie elementy stworzone w tym oknie
        // i dla każdego z nich wykonuje linijkę kodu zdefiniowaną wewnątrz (tutaj: zmiana czcionki na systemową).
        EnumChildWindows(window, [](HWND hwnd, LPARAM lParam) -> BOOL {
            SendMessageW(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
            return TRUE;
            }, (LPARAM)GetStockObject(DEFAULT_GUI_FONT));

        return 0; // Tworzenie interfejsu zakończone
    }

                  // --- ULEPSZENIE: PRZEZROCZYSTE TŁA NAPISÓW ---
                  // Ten komunikat jest wysyłany przez Windows, gdy system chce narysować zwykły tekst (STATIC).
                  // Gdybyśmy tego nie zrobili, napisy miałyby szare prostokąty pod spodem (bo tło naszego okna jest białe).
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;           // hdc to "płótno" na którym system zaraz narysuje tekst
        SetBkMode(hdc, TRANSPARENT);     // Mówimy systemowi: "Nie używaj żadnego koloru tła pod literami!"
        return (LRESULT)GetStockObject(WHITE_BRUSH); // Rysuj tło jako białe (zgodnie z naszym głównym oknem)
    }

                          // --- REAKCJE NA KLIKNIĘCIA ---
    case WM_COMMAND: {
        int clicked_id = LOWORD(wParam); // Odczytujemy, ID elementu z którym wejściowo w interakcję

        // Sprawdzamy, czy kliknięto przycisk "Draw Gradient!" (ID 206)
        if (clicked_id == 206) {

            // 1. ODCZYT: Radio Buttons i Checkbox
            // IsDlgButtonChecked pyta dany element czy jest zaznaczony. Zwraca flagę BST_CHECKED jeśli tak.
            bool is_linear = (IsDlgButtonChecked(window, 201) == BST_CHECKED);
            bool invert_colors = (IsDlgButtonChecked(window, 203) == BST_CHECKED);

            // 2. ODCZYT: Suwak (Trackbar)
            HWND hSlider = GetDlgItem(window, 205); // Najpierw znajdujemy fizyczny uchwyt suwaka po jego ID
            LRESULT slider_pos = SendMessageW(hSlider, TBM_GETPOS, 0, 0); // Potem pytamy go o pozycję

            // 3. ODCZYT: Pole tekstowe (Edit)
            wchar_t angle_text[10]{}; // Tworzymy mały bufor znakowy na odpowiedź z pola (max 10 cyfr)
            GetDlgItemTextW(window, 209, angle_text, 10); // Pobieramy tekst z kontrolki o ID 209 i wrzucamy do bufora

            // 4. PRZETWARZANIE DANYCH: Sklejanie informacji w jeden ładny tekst do wyświetlenia
            std::wstring info = L"Collected settings:\n\n";
            info += L"Type: " + std::wstring(is_linear ? L"Linear" : L"Radial") + L"\n";
            info += L"Invert Colors: " + std::wstring(invert_colors ? L"YES" : L"NO") + L"\n";
            info += L"Effect Strength: " + std::to_wstring(slider_pos) + L"%\n";
            info += L"Angle: " + std::wstring(angle_text) + L" degrees\n\n";
            info += L"Ready to draw!";

            // 5. WYNIK: Pokaż okienko informacyjne (tylko z przyciskiem OK i ikonką i)
            MessageBoxW(window, info.c_str(), L"Interface Data", MB_OK | MB_ICONINFORMATION);
        }
        return 0; // Komunikat obsłużony
    }

                   // --- STANDARDOWE ZAMYKANIE APLIKACJI ---
    case WM_CLOSE: // Reakcja na kliknięcie "X" lub Alt+F4
        DestroyWindow(window); // Niszczy fizyczne okno na ekranie
        return 0;

    case WM_DESTROY: // Ostatnie tchnienie okna
        PostQuitMessage(0); // Wysyła sygnał zakończenia do funkcji run(), co pozwala wyjść z programu
        return 0;
    }

    // Każdy inny, domyślny komunikat (np. ruch myszą w pustym miejscu) odsyłamy do Windowsa
    return DefWindowProcW(window, message, wParam, lParam);
}