#include "Window.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Window app(hInstance);

    if (!app.is_initialized()) {
        return -1;
    }

    return app.run(nCmdShow);
}