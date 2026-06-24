#include "OverlayWindow.h"
#include <windowsx.h>
#include <iostream>
#include <sstream>
#include <vector>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

OverlayWindow::OverlayWindow()
    : m_hwnd(nullptr)
    , m_isDragging(false)
    , m_selectionComplete(false)
    , m_isActive(false)
    , m_selectionConfirmed(false)
    , m_hdcMem(nullptr)
    , m_hbmMem(nullptr) {
    ZeroMemory(&m_selection, sizeof(RECT));
    ZeroMemory(&m_startPoint, sizeof(POINT));
    ZeroMemory(&m_endPoint, sizeof(POINT));
}

OverlayWindow::~OverlayWindow() {
    Destroy();
}

bool OverlayWindow::Create() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_CROSS);
    wc.lpszClassName = L"OverlayWindowClass";

    if (!RegisterClassEx(&wc)) {
        DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            std::cerr << "[Overlay] Failed to register class, error: " << error << "\n";
            return false;
        }
    }

    int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    int screenLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screenTop = GetSystemMetrics(SM_YVIRTUALSCREEN);

    std::cout << "[Overlay] Screen: " << screenLeft << "," << screenTop
        << " " << screenWidth << "x" << screenHeight << "\n";

    m_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"OverlayWindowClass",
        L"Overlay",
        WS_POPUP,
        screenLeft, screenTop, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!m_hwnd) {
        DWORD error = GetLastError();
        std::cerr << "[Overlay] Failed to create window, error: " << error << "\n";
        return false;
    }

    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Устанавливаем размеры окна
    m_windowRect.left = screenLeft;
    m_windowRect.top = screenTop;
    m_windowRect.right = screenLeft + screenWidth;
    m_windowRect.bottom = screenTop + screenHeight;

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    SetCapture(m_hwnd);
    m_isActive = true;
    m_selectionComplete = false;
    m_selectionConfirmed = false;

    // Первоначальная отрисовка
    DrawOverlay();

    std::cout << "[Overlay] Created successfully\n";
    return true;
}

void OverlayWindow::Destroy() {
    if (m_hwnd) {
        if (m_isActive) {
            ReleaseCapture();
        }
        if (m_hdcMem) {
            DeleteDC(m_hdcMem);
            m_hdcMem = nullptr;
        }
        if (m_hbmMem) {
            DeleteObject(m_hbmMem);
            m_hbmMem = nullptr;
        }
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        m_isActive = false;
    }
}

void OverlayWindow::Show() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_SHOW);
        SetCapture(m_hwnd);
        m_isActive = true;
        m_selectionConfirmed = false;
        DrawOverlay();
    }
}

void OverlayWindow::Hide() {
    if (m_hwnd) {
        ShowWindow(m_hwnd, SW_HIDE);
        if (m_isActive) {
            ReleaseCapture();
            m_isActive = false;
        }
    }
}

void OverlayWindow::Reset() {
    m_selectionComplete = false;
    m_selectionConfirmed = false;
    m_isDragging = false;
    ZeroMemory(&m_selection, sizeof(RECT));
    DrawOverlay();
}

LRESULT CALLBACK OverlayWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    OverlayWindow* pThis = reinterpret_cast<OverlayWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(m_hwnd, &ps);
        DrawOverlay();
        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (!m_selectionComplete && !m_selectionConfirmed) {
            m_startPoint.x = GET_X_LPARAM(lParam);
            m_startPoint.y = GET_Y_LPARAM(lParam);
            m_endPoint = m_startPoint;
            m_isDragging = true;
            ZeroMemory(&m_selection, sizeof(RECT));
            std::cout << "[Overlay] Drag started at (" << m_startPoint.x << "," << m_startPoint.y << ")\n";
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (m_isDragging && !m_selectionConfirmed) {
            m_endPoint.x = GET_X_LPARAM(lParam);
            m_endPoint.y = GET_Y_LPARAM(lParam);
            UpdateSelection(m_endPoint);
            DrawOverlay();
        }
        SetCursor(LoadCursor(nullptr, IDC_CROSS));
        return 0;
    }

    case WM_LBUTTONUP: {
        if (m_isDragging && !m_selectionConfirmed) {
            m_isDragging = false;

            // Сортируем координаты
            if (m_selection.left > m_selection.right) {
                std::swap(m_selection.left, m_selection.right);
            }
            if (m_selection.top > m_selection.bottom) {
                std::swap(m_selection.top, m_selection.bottom);
            }

            int width = m_selection.right - m_selection.left;
            int height = m_selection.bottom - m_selection.top;

            if (width < MIN_SELECTION_SIZE || height < MIN_SELECTION_SIZE) {
                std::cout << "[Overlay] Selection too small (" << width << "x" << height << "), ignoring\n";
                ZeroMemory(&m_selection, sizeof(RECT));
                DrawOverlay();
                return 0;
            }

            // Выделение создано, но еще не подтверждено
            m_selectionComplete = true;
            std::cout << "[Overlay] Selection created, press ENTER to confirm or ESC to cancel\n";
            std::cout << "[Overlay] Selection: ("
                << m_selection.left << "," << m_selection.top
                << ") - (" << m_selection.right << "," << m_selection.bottom
                << ") size: " << width << "x" << height << "\n";

            DrawOverlay();
        }
        return 0;
    }

    case WM_RBUTTONDOWN: {
        if (m_isDragging) {
            m_isDragging = false;
        }
        m_selectionComplete = false;
        m_selectionConfirmed = false;
        ZeroMemory(&m_selection, sizeof(RECT));
        DrawOverlay();
        std::cout << "[Overlay] Selection cancelled by right click\n";
        return 0;
    }

    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE) {
            // Отмена выделения по ESC
            m_selectionComplete = false;
            m_selectionConfirmed = false;
            m_isDragging = false;
            ZeroMemory(&m_selection, sizeof(RECT));
            DrawOverlay();
            std::cout << "[Overlay] Selection cancelled by ESC\n";
            return 0;
        }
        else if (wParam == VK_RETURN && m_selectionComplete && !m_selectionConfirmed) {
            // Подтверждение выделения по Enter
            m_selectionConfirmed = true;
            ReleaseCapture();
            Hide();

            std::cout << "[Overlay] Selection confirmed by ENTER\n";
            std::cout << "[Overlay] Final selection: ("
                << m_selection.left << "," << m_selection.top
                << ") - (" << m_selection.right << "," << m_selection.bottom
                << ")\n";

            // Отправляем сообщение о завершении
            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(m_hwnd, msg, wParam, lParam);
}

void OverlayWindow::UpdateSelection(POINT pt) {
    m_selection.left = min(m_startPoint.x, pt.x);
    m_selection.top = min(m_startPoint.y, pt.y);
    m_selection.right = max(m_startPoint.x, pt.x);
    m_selection.bottom = max(m_startPoint.y, pt.y);
}

void OverlayWindow::DrawOverlay() {
    if (!m_hwnd) return;

    int width = m_windowRect.right - m_windowRect.left;
    int height = m_windowRect.bottom - m_windowRect.top;

    // Создаем 32-битный DIB с альфа-каналом
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hBitmap || !bits) {
        std::cerr << "[Overlay] Failed to create DIB section\n";
        return;
    }

    BYTE* pixels = (BYTE*)bits;
    int pitch = width * 4;

    // Заполняем пиксели
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * pitch + x * 4;

            bool inSelection = false;
            if (m_selection.left != m_selection.right && m_selection.top != m_selection.bottom) {
                int screenX = m_windowRect.left + x;
                int screenY = m_windowRect.top + y;
                inSelection = (screenX >= m_selection.left && screenX < m_selection.right &&
                    screenY >= m_selection.top && screenY < m_selection.bottom);
            }

            if (inSelection) {
                // Выделенная область - полностью прозрачная
                pixels[idx + 0] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
                pixels[idx + 3] = 0;
            }
            else {
                // Затемненная область - полупрозрачная
                pixels[idx + 0] = 0;
                pixels[idx + 1] = 0;
                pixels[idx + 2] = 0;
                pixels[idx + 3] = DARKEN_ALPHA;
            }
        }
    }

    // Рисуем рамку выделения и информацию
    if (m_selection.left != m_selection.right && m_selection.top != m_selection.bottom) {
        int left = m_selection.left - m_windowRect.left;
        int top = m_selection.top - m_windowRect.top;
        int right = m_selection.right - m_windowRect.left;
        int bottom = m_selection.bottom - m_windowRect.top;

        // Рисуем белую рамку
        for (int x = left; x < right; x++) {
            if (top >= 0 && top < height && x >= 0 && x < width) {
                int idx = top * pitch + x * 4;
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
            if (top + 1 >= 0 && top + 1 < height && x >= 0 && x < width) {
                int idx = (top + 1) * pitch + x * 4;
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
            if (bottom - 1 >= 0 && bottom - 1 < height && x >= 0 && x < width) {
                int idx = (bottom - 1) * pitch + x * 4;
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
            if (bottom - 2 >= 0 && bottom - 2 < height && x >= 0 && x < width) {
                int idx = (bottom - 2) * pitch + x * 4;
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
        }

        for (int y = top; y < bottom; y++) {
            if (y >= 0 && y < height && left >= 0 && left < width) {
                int idx = y * pitch + left * 4;
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
            if (y >= 0 && y < height && left + 1 >= 0 && left + 1 < width) {
                int idx = y * pitch + (left + 1) * 4;
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
            if (y >= 0 && y < height && right - 1 >= 0 && right - 1 < width) {
                int idx = y * pitch + (right - 1) * 4;
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
            if (y >= 0 && y < height && right - 2 >= 0 && right - 2 < width) {
                int idx = y * pitch + (right - 2) * 4;
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
        }

        // Рисуем информацию о размерах
        HDC hdc = GetDC(m_hwnd);
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, hBitmap);

        int infoWidth = m_selection.right - m_selection.left;
        int infoHeight = m_selection.bottom - m_selection.top;
        std::string sizeText = std::to_string(infoWidth) + " x " + std::to_string(infoHeight);

        // Добавляем подсказку о подтверждении
        std::string hintText = "Press ENTER to confirm, ESC to cancel";

        // Фон для текста
        RECT textRect = {
            m_selection.left + 10,
            m_selection.top + 10,
            m_selection.left + 300,
            m_selection.top + 70
        };
        HBRUSH bgBrush = CreateSolidBrush(RGB(40, 40, 40));
        FillRect(hdcMem, &textRect, bgBrush);
        DeleteObject(bgBrush);

        // Рамка вокруг текста
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN oldPen = (HPEN)SelectObject(hdcMem, borderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
        Rectangle(hdcMem, textRect.left, textRect.top, textRect.right, textRect.bottom);
        SelectObject(hdcMem, oldPen);
        SelectObject(hdcMem, oldBrush);
        DeleteObject(borderPen);

        // Текст с размерами
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(255, 255, 255));
        TextOutA(hdcMem, m_selection.left + 15, m_selection.top + 12,
            sizeText.c_str(), static_cast<int>(sizeText.length()));

        // Текст с подсказкой (если выделение еще не подтверждено)
        if (!m_selectionConfirmed) {
            SetTextColor(hdcMem, RGB(200, 200, 100));
            TextOutA(hdcMem, m_selection.left + 15, m_selection.top + 35,
                hintText.c_str(), static_cast<int>(hintText.length()));
        }

        ReleaseDC(m_hwnd, hdc);
        DeleteDC(hdcMem);
    }

    // Обновляем слоеное окно
    BLENDFUNCTION blend = {};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    POINT ptSrc = { 0, 0 };
    SIZE sizeWnd = { width, height };
    POINT ptDst = { m_windowRect.left, m_windowRect.top };

    HDC hdcScreen = GetDC(nullptr);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    SelectObject(hdcMem, hBitmap);

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    DeleteDC(hdcMem);
    ReleaseDC(nullptr, hdcScreen);
    DeleteObject(hBitmap);
}