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

    // Определение зоны курсора на границе
    enum class HitZone {
        None,
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        Move
    };

    HitZone GetHitZone(POINT pt) const;
    bool IsPointInSelection(POINT pt) const;

    HWND m_hwnd;
    HDC m_hdcMem;
    HBITMAP m_hbmMem;
    RECT m_windowRect;
    RECT m_selection;
    RECT m_originalSelection;  // Для сохранения исходного размера при ресайзе
    POINT m_startPoint;
    POINT m_endPoint;
    POINT m_dragOffset;
    HitZone m_hitZone;         // Текущая зона захвата
    bool m_isDragging;
    bool m_isMoving;
    bool m_isResizing;         // Флаг ресайза
    bool m_selectionComplete;
    bool m_isActive;
    bool m_selectionConfirmed;

    static constexpr int DARKEN_ALPHA = 180;
    static constexpr int MIN_SELECTION_SIZE = 10;
    static constexpr int HIT_MARGIN = 8;  // Зона захвата границы в пикселях
};