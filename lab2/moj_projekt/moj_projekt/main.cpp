#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wWinMainCRTStartup")
#include <windows.h>
#include "PaintApp.h"

int WINAPI wWinMain(
    _In_ HINSTANCE instance,
    _In_opt_ HINSTANCE prevInstance,
    _In_ LPWSTR command_line,
    _In_ int show_command)
{
    // Tworzymy aplikację [cite: 205]
    PaintApp app{ instance };
    // Odpalamy główną pętlę [cite: 205]
    return app.run(show_command);
}