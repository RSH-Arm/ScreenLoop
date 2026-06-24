#pragma once
#include <windows.h>
#include <string>

class OverlayWindow {
public:
    OverlayWindow();
    ~OverlayWindow();

    bool Create();
    void Destroy();
    void Show();
    void Hide();

    RECT GetSelection() const { return m_selection; }
    bool IsSelectionComplete() const { return m_selectionComplete; }
    void Reset();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void UpdateSelection(POINT pt);
    void DrawOverlay();
    void DrawSelectionInfo(HDC hdc);

    HWND m_hwnd;
    HDC m_hdcMem;
    HBITMAP m_hbmMem;
    RECT m_windowRect;
    RECT m_selection;
    POINT m_startPoint;
    POINT m_endPoint;
    bool m_isDragging;
    bool m_selectionComplete;
    bool m_isActive;
    bool m_selectionConfirmed;

    static constexpr int DARKEN_ALPHA = 180; // 0-255, где 180 = ~70% затемнения
    static constexpr int MIN_SELECTION_SIZE = 10;
};