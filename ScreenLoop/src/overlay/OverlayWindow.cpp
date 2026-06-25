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
    , m_isMoving(false)
    , m_isResizing(false)
    , m_selectionComplete(false)
    , m_isActive(false)
    , m_selectionConfirmed(false)
    , m_hitZone(HitZone::None)
    , m_hdcMem(nullptr)
    , m_hbmMem(nullptr) {
    ZeroMemory(&m_selection, sizeof(RECT));
    ZeroMemory(&m_originalSelection, sizeof(RECT));
    ZeroMemory(&m_startPoint, sizeof(POINT));
    ZeroMemory(&m_endPoint, sizeof(POINT));
    ZeroMemory(&m_dragOffset, sizeof(POINT));
}

OverlayWindow::~OverlayWindow() {
    Destroy();
}

bool OverlayWindow::Create() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    // Очищаем очередь сообщений перед созданием окна
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

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
    m_isMoving = false;
    m_isResizing = false;
    ZeroMemory(&m_selection, sizeof(RECT));
    DrawOverlay();
}

bool OverlayWindow::IsPointInSelection(POINT pt) const {
    if (m_selection.left == m_selection.right || m_selection.top == m_selection.bottom) {
        return false;
    }
    int margin = 5;
    return (pt.x >= m_selection.left - margin && pt.x <= m_selection.right + margin &&
            pt.y >= m_selection.top - margin && pt.y <= m_selection.bottom + margin);
}

OverlayWindow::HitZone OverlayWindow::GetHitZone(POINT pt) const {
    if (m_selection.left == m_selection.right || m_selection.top == m_selection.bottom) {
        return HitZone::None;
    }

    int left = m_selection.left;
    int right = m_selection.right;
    int top = m_selection.top;
    int bottom = m_selection.bottom;
    int margin = HIT_MARGIN;

    // Проверяем углы
    if (pt.x >= left - margin && pt.x <= left + margin && pt.y >= top - margin && pt.y <= top + margin) {
        return HitZone::TopLeft;
    }
    if (pt.x >= right - margin && pt.x <= right + margin && pt.y >= top - margin && pt.y <= top + margin) {
        return HitZone::TopRight;
    }
    if (pt.x >= left - margin && pt.x <= left + margin && pt.y >= bottom - margin && pt.y <= bottom + margin) {
        return HitZone::BottomLeft;
    }
    if (pt.x >= right - margin && pt.x <= right + margin && pt.y >= bottom - margin && pt.y <= bottom + margin) {
        return HitZone::BottomRight;
    }

    // Проверяем стороны
    if (pt.x >= left - margin && pt.x <= left + margin && pt.y >= top && pt.y <= bottom) {
        return HitZone::Left;
    }
    if (pt.x >= right - margin && pt.x <= right + margin && pt.y >= top && pt.y <= bottom) {
        return HitZone::Right;
    }
    if (pt.y >= top - margin && pt.y <= top + margin && pt.x >= left && pt.x <= right) {
        return HitZone::Top;
    }
    if (pt.y >= bottom - margin && pt.y <= bottom + margin && pt.x >= left && pt.x <= right) {
        return HitZone::Bottom;
    }

    // Проверяем внутреннюю область для перемещения
    if (IsPointInSelection(pt)) {
        return HitZone::Move;
    }

    return HitZone::None;
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
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        // Если выделение создано и не подтверждено
        if (m_selectionComplete && !m_selectionConfirmed) {
            HitZone zone = GetHitZone(pt);
            
            if (zone != HitZone::None) {
                m_hitZone = zone;
                m_originalSelection = m_selection;
                m_startPoint = pt;
                
                if (zone == HitZone::Move) {
                    // Перемещение
                    m_isMoving = true;
                    m_dragOffset.x = pt.x - m_selection.left;
                    m_dragOffset.y = pt.y - m_selection.top;
                    SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                    std::cout << "[Overlay] Start moving selection\n";
                } else {
                    // Ресайз
                    m_isResizing = true;
                    // Устанавливаем соответствующий курсор
                    switch (zone) {
                        case HitZone::Left:
                        case HitZone::Right:
                            SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                            break;
                        case HitZone::Top:
                        case HitZone::Bottom:
                            SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                            break;
                        case HitZone::TopLeft:
                        case HitZone::BottomRight:
                            SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
                            break;
                        case HitZone::TopRight:
                        case HitZone::BottomLeft:
                            SetCursor(LoadCursor(nullptr, IDC_SIZENESW));
                            break;
                        default:
                            break;
                    }
                    std::cout << "[Overlay] Start resizing selection\n";
                }
                return 0;
            } else {
                // Клик вне области - сбрасываем выделение и начинаем новое
                std::cout << "[Overlay] Click outside selection, resetting\n";
                m_selectionComplete = false;
                m_selectionConfirmed = false;
                ZeroMemory(&m_selection, sizeof(RECT));

                m_startPoint = pt;
                m_endPoint = pt;
                m_isDragging = true;
                std::cout << "[Overlay] New drag started at (" << pt.x << "," << pt.y << ")\n";
                DrawOverlay();
                return 0;
            }
        }

        // Если нет выделения - начинаем новое выделение
        if (!m_selectionComplete && !m_selectionConfirmed) {
            m_startPoint = pt;
            m_endPoint = pt;
            m_isDragging = true;
            ZeroMemory(&m_selection, sizeof(RECT));
            std::cout << "[Overlay] Drag started at (" << pt.x << "," << pt.y << ")\n";
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        // Если ресайзим область
        if (m_isResizing && !m_selectionConfirmed) {
            int dx = pt.x - m_startPoint.x;
            int dy = pt.y - m_startPoint.y;

            RECT newRect = m_originalSelection;
            int minSize = MIN_SELECTION_SIZE;

            switch (m_hitZone) {
                case HitZone::Left:
                    newRect.left = m_originalSelection.left + dx;
                    if (newRect.right - newRect.left < minSize) {
                        newRect.left = newRect.right - minSize;
                    }
                    break;
                case HitZone::Right:
                    newRect.right = m_originalSelection.right + dx;
                    if (newRect.right - newRect.left < minSize) {
                        newRect.right = newRect.left + minSize;
                    }
                    break;
                case HitZone::Top:
                    newRect.top = m_originalSelection.top + dy;
                    if (newRect.bottom - newRect.top < minSize) {
                        newRect.top = newRect.bottom - minSize;
                    }
                    break;
                case HitZone::Bottom:
                    newRect.bottom = m_originalSelection.bottom + dy;
                    if (newRect.bottom - newRect.top < minSize) {
                        newRect.bottom = newRect.top + minSize;
                    }
                    break;
                case HitZone::TopLeft:
                    newRect.left = m_originalSelection.left + dx;
                    newRect.top = m_originalSelection.top + dy;
                    if (newRect.right - newRect.left < minSize) {
                        newRect.left = newRect.right - minSize;
                    }
                    if (newRect.bottom - newRect.top < minSize) {
                        newRect.top = newRect.bottom - minSize;
                    }
                    break;
                case HitZone::TopRight:
                    newRect.right = m_originalSelection.right + dx;
                    newRect.top = m_originalSelection.top + dy;
                    if (newRect.right - newRect.left < minSize) {
                        newRect.right = newRect.left + minSize;
                    }
                    if (newRect.bottom - newRect.top < minSize) {
                        newRect.top = newRect.bottom - minSize;
                    }
                    break;
                case HitZone::BottomLeft:
                    newRect.left = m_originalSelection.left + dx;
                    newRect.bottom = m_originalSelection.bottom + dy;
                    if (newRect.right - newRect.left < minSize) {
                        newRect.left = newRect.right - minSize;
                    }
                    if (newRect.bottom - newRect.top < minSize) {
                        newRect.bottom = newRect.top + minSize;
                    }
                    break;
                case HitZone::BottomRight:
                    newRect.right = m_originalSelection.right + dx;
                    newRect.bottom = m_originalSelection.bottom + dy;
                    if (newRect.right - newRect.left < minSize) {
                        newRect.right = newRect.left + minSize;
                    }
                    if (newRect.bottom - newRect.top < minSize) {
                        newRect.bottom = newRect.top + minSize;
                    }
                    break;
                default:
                    break;
            }

            // Проверяем границы экрана
            if (newRect.left < m_windowRect.left) {
                newRect.left = m_windowRect.left;
            }
            if (newRect.top < m_windowRect.top) {
                newRect.top = m_windowRect.top;
            }
            if (newRect.right > m_windowRect.right) {
                newRect.right = m_windowRect.right;
            }
            if (newRect.bottom > m_windowRect.bottom) {
                newRect.bottom = m_windowRect.bottom;
            }
            // Убеждаемся, что размер не меньше минимального
            if (newRect.right - newRect.left < minSize) {
                newRect.right = newRect.left + minSize;
            }
            if (newRect.bottom - newRect.top < minSize) {
                newRect.bottom = newRect.top + minSize;
            }

            m_selection = newRect;
            DrawOverlay();
            
            // Обновляем курсор
            switch (m_hitZone) {
                case HitZone::Left:
                case HitZone::Right:
                    SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                    break;
                case HitZone::Top:
                case HitZone::Bottom:
                    SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                    break;
                case HitZone::TopLeft:
                case HitZone::BottomRight:
                    SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
                    break;
                case HitZone::TopRight:
                case HitZone::BottomLeft:
                    SetCursor(LoadCursor(nullptr, IDC_SIZENESW));
                    break;
                default:
                    break;
            }
            return 0;
        }

        // Если перетаскиваем область
        if (m_isMoving && !m_selectionConfirmed) {
            int width = m_selection.right - m_selection.left;
            int height = m_selection.bottom - m_selection.top;

            int newLeft = pt.x - m_dragOffset.x;
            int newTop = pt.y - m_dragOffset.y;
            int newRight = newLeft + width;
            int newBottom = newTop + height;

            // Проверяем границы экрана
            int screenLeft = m_windowRect.left;
            int screenTop = m_windowRect.top;
            int screenRight = m_windowRect.right;
            int screenBottom = m_windowRect.bottom;

            if (newLeft < screenLeft) {
                newLeft = screenLeft;
                newRight = newLeft + width;
            }
            if (newTop < screenTop) {
                newTop = screenTop;
                newBottom = newTop + height;
            }
            if (newRight > screenRight) {
                newRight = screenRight;
                newLeft = newRight - width;
            }
            if (newBottom > screenBottom) {
                newBottom = screenBottom;
                newTop = newBottom - height;
            }

            m_selection.left = newLeft;
            m_selection.top = newTop;
            m_selection.right = newRight;
            m_selection.bottom = newBottom;

            DrawOverlay();
            SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
            return 0;
        }

        // Если создаем новое выделение
        if (m_isDragging && !m_selectionConfirmed) {
            m_endPoint = pt;
            UpdateSelection(m_endPoint);
            DrawOverlay();
            SetCursor(LoadCursor(nullptr, IDC_CROSS));
            return 0;
        }

        // Меняем курсор при наведении на область или её границы
        if (m_selectionComplete && !m_selectionConfirmed) {
            HitZone zone = GetHitZone(pt);
            switch (zone) {
                case HitZone::None:
                    SetCursor(LoadCursor(nullptr, IDC_CROSS));
                    break;
                case HitZone::Move:
                    SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                    break;
                case HitZone::Left:
                case HitZone::Right:
                    SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                    break;
                case HitZone::Top:
                case HitZone::Bottom:
                    SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                    break;
                case HitZone::TopLeft:
                case HitZone::BottomRight:
                    SetCursor(LoadCursor(nullptr, IDC_SIZENWSE));
                    break;
                case HitZone::TopRight:
                case HitZone::BottomLeft:
                    SetCursor(LoadCursor(nullptr, IDC_SIZENESW));
                    break;
            }
        } else {
            SetCursor(LoadCursor(nullptr, IDC_CROSS));
        }
        return 0;
    }

    case WM_LBUTTONUP: {
        // Если ресайзили область
        if (m_isResizing && !m_selectionConfirmed) {
            m_isResizing = false;
            m_hitZone = HitZone::None;
            std::cout << "[Overlay] Selection resized to: ("
                << m_selection.left << "," << m_selection.top
                << ") - (" << m_selection.right << "," << m_selection.bottom
                << ") size: " << (m_selection.right - m_selection.left) 
                << "x" << (m_selection.bottom - m_selection.top) << "\n";
            DrawOverlay();
            return 0;
        }

        // Если перетаскивали область
        if (m_isMoving && !m_selectionConfirmed) {
            m_isMoving = false;
            std::cout << "[Overlay] Selection moved to: ("
                << m_selection.left << "," << m_selection.top
                << ") - (" << m_selection.right << "," << m_selection.bottom
                << ")\n";
            DrawOverlay();
            return 0;
        }

        // Если создавали новое выделение
        if (m_isDragging && !m_selectionConfirmed) {
            m_isDragging = false;

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
        if (m_isMoving) {
            m_isMoving = false;
        }
        if (m_isResizing) {
            m_isResizing = false;
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
            // Если нет выделения - выходим из режима выделения
            if (!m_selectionComplete && !m_selectionConfirmed) {
                std::cout << "[Overlay] ESC pressed - exiting selection mode\n";
                m_selectionConfirmed = false;
                ReleaseCapture();
                Hide();
                // Отправляем специальное сообщение для выхода
                PostQuitMessage(1);  // 1 означает отмену
                return 0;
            }

            // Если выделение создано - отменяем его
            m_selectionComplete = false;
            m_selectionConfirmed = false;
            m_isDragging = false;
            m_isMoving = false;
            m_isResizing = false;
            ZeroMemory(&m_selection, sizeof(RECT));
            DrawOverlay();
            std::cout << "[Overlay] Selection cancelled by ESC\n";
            return 0;
        }
        else if (wParam == VK_RETURN && m_selectionComplete && !m_selectionConfirmed) {
            m_selectionConfirmed = true;
            ReleaseCapture();
            Hide();

            std::cout << "[Overlay] Selection confirmed by ENTER\n";
            std::cout << "[Overlay] Final selection: ("
                << m_selection.left << "," << m_selection.top
                << ") - (" << m_selection.right << "," << m_selection.bottom
                << ")\n";

            PostQuitMessage(0);  // 0 означает подтверждение
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

    // Заполняем все пиксели затемнением
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * pitch + x * 4;
            pixels[idx + 0] = 0;
            pixels[idx + 1] = 0;
            pixels[idx + 2] = 0;
            pixels[idx + 3] = DARKEN_ALPHA;
        }
    }

    // Если есть выделение - делаем его почти прозрачным (альфа = 1)
    if (m_selection.left != m_selection.right && m_selection.top != m_selection.bottom) {
        int left = m_selection.left - m_windowRect.left;
        int top = m_selection.top - m_windowRect.top;
        int right = m_selection.right - m_windowRect.left;
        int bottom = m_selection.bottom - m_windowRect.top;

        // Делаем выделенную область почти прозрачной (альфа = 1)
        for (int y = top; y < bottom; y++) {
            for (int x = left; x < right; x++) {
                if (y >= 0 && y < height && x >= 0 && x < width) {
                    int idx = y * pitch + x * 4;
                    pixels[idx + 0] = 0;
                    pixels[idx + 1] = 0;
                    pixels[idx + 2] = 0;
                    pixels[idx + 3] = 1;
                }
            }
        }

        // Рисуем белую рамку (толщиной 2 пикселя)
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

        // Информация о размерах
        HDC hdc = GetDC(m_hwnd);
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, hBitmap);

        int infoWidth = m_selection.right - m_selection.left;
        int infoHeight = m_selection.bottom - m_selection.top;
        std::string sizeText = std::to_string(infoWidth) + " x " + std::to_string(infoHeight);

        RECT textRect = {
            m_selection.left + 10,
            m_selection.top + 10,
            m_selection.left + 200,
            m_selection.top + 40
        };
        HBRUSH bgBrush = CreateSolidBrush(RGB(40, 40, 40));
        FillRect(hdcMem, &textRect, bgBrush);
        DeleteObject(bgBrush);

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
        HPEN oldPen = (HPEN)SelectObject(hdcMem, borderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
        Rectangle(hdcMem, textRect.left, textRect.top, textRect.right, textRect.bottom);
        SelectObject(hdcMem, oldPen);
        SelectObject(hdcMem, oldBrush);
        DeleteObject(borderPen);

        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(255, 255, 255));
        TextOutA(hdcMem, m_selection.left + 15, m_selection.top + 12,
            sizeText.c_str(), static_cast<int>(sizeText.length()));

        ReleaseDC(m_hwnd, hdc);
        DeleteDC(hdcMem);
    }

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