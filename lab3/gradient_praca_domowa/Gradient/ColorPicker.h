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
    bool m_isUpdatingUI;

    enum DragMode { NONE, HUE_RING, SV_TRIANGLE };
    DragMode m_dragMode;

    // Aktualny i początkowy kolor w modelu RGB + składowe HSV.
    COLORREF m_oldColor;
    COLORREF m_currentColor;
    float m_h, m_s, m_v;

    // Kontrolki edycyjne dla kanałów R,G,B,H,S,V i pola HEX.
    HWND m_hSliders[6];
    HWND m_hEdits[6];
    HWND m_hHexEdit;

    // Funkcje pomocnicze konwersji i odświeżania UI.
    static void RGBtoHSV(COLORREF rgb, float& h, float& s, float& v);
    static COLORREF HSVtoRGB(float h, float s, float v);
    void RenderPickerDIB(HDC hdc, const RECT& rc);
    void UpdateUI();

    // Procedury okna i hook myszy dla pipety ekranowej.
    static const wchar_t* s_className;
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static HHOOK s_mouseHook;
    static ColorPicker* s_activePicker;
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
};