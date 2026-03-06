#include <windows.h>
#include "PaintApp.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE /*prevInstance*/, LPWSTR /*command_line*/, int show_command) {
    PaintApp app{ instance };
    return app.run(show_command);
}