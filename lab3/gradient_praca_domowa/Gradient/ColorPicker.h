#pragma once
#include <windows.h>
#include <commctrl.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <string>

class ColorPicker {
public:
    static bool Show(HWND hParent, COLORREF& inOutColor);

private:
    ColorPicker(HWND hParent, COLORREF startColor);
    ~ColorPicker();

    HWND m_hwnd;
    HWND m_hParent;
    bool m_confirmed;
    bool m_isUpdatingUI; // Flaga zapobiegająca nieskończonym pętlom podczas wpisywania!

    enum DragMode { NONE, HUE_RING, SV_TRIANGLE };
    DragMode m_dragMode;

    // --- STAN KOLORU ---
    COLORREF m_oldColor;
    COLORREF m_currentColor;
    float m_h, m_s, m_v; // Odcień (0-360), Nasycenie (0-1), Jasność (0-1)

    // --- KONTROLKI Z PRAWEJ STRONY ---
    HWND m_hSliders[6]; // R, G, B, H, S, V
    HWND m_hEdits[6];
    HWND m_hHexEdit;

    // --- FUNKCJE POMOCNICZE ---
    static void RGBtoHSV(COLORREF rgb, float& h, float& s, float& v);
    static COLORREF HSVtoRGB(float h, float s, float v);
    void RenderPickerDIB(HDC hdc, const RECT& rc);
    void UpdateUI();

    // --- PROCEDURY I HOOKI ---
    static const wchar_t* s_className;
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static HHOOK s_mouseHook;
    static ColorPicker* s_activePicker;
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
};