#include "Window.h" // Dołączamy definicję naszej klasy Window, żeby kompilator wiedział, co to jest.

// Główna funkcja programu okienkowego (odpowiednik int main() w konsoli).
// WINAPI - to instrukcja dla kompilatora, jak ma przekazywać parametry do funkcji (tzw. konwencja wywołania).
// hInstance - unikalny identyfikator (uchwyt) naszego uruchomionego programu nadany przez Windows.
// hPrevInstance - relikt przeszłości (zawsze NULL w dzisiejszych systemach).
// lpCmdLine - argumenty z wiersza poleceń (gdyby ktoś uruchomił program z konsoli z jakimiś dopiskami).
// nCmdShow - informacja od systemu, czy okno ma startować zminimalizowane, zmaksymalizowane, czy normalnie.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    // Tworzymy fizyczny obiekt naszej klasy o nazwie 'app'. 
    // Przekazujemy mu 'hInstance', żeby klasa wiedziała, do jakiego programu należy.
    // W tym momencie wywołuje się Konstruktor klasy Window!
    Window app(hInstance);

    // Sprawdzamy, czy okno faktycznie się stworzyło (czy uchwyt nie jest pusty).
    if (!app.is_initialized()) {
        // Jeśli okno nie powstało (np. błąd pamięci), zwracamy -1 i natychmiast kończymy program.
        return -1;
    }

    // Jeśli okno powstało, uruchamiamy naszą "ukrytą" pętlę komunikatów.
    // Funkcja run() zablokuje się w tym miejscu i będzie działać tak długo, aż zamkniemy okno.
    // Gdy okno się zamknie, run() zwróci kod wyjścia, a program się zakończy.
    return app.run(nCmdShow);
}