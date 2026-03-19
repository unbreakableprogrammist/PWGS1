#include "Window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Tworzymy obiekt naszej aplikacji/okna
    Window myApp(hInstance);

    // Jeśli okno nie powstało, przerywamy
    if (!myApp.IsInitialized()) {
        return -1;
    }

    // Pętla główna - działa tak długo, aż ProcessMessages zwróci false
    while (myApp.ProcessMessages()) {
        // Tutaj w przyszłości można dodać np. logikę gry, 
        // ale dla kalkulatora wystarczy samo przetwarzanie komunikatów.
    }

    return 0;
}