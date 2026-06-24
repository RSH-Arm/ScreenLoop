#include "TaskExecutor.h"
#include "../overlay/OverlayWindow.h"
#include <iostream>
#include <thread>
#include <chrono>

// РЕАЛИЗАЦИЯ SaveScreenshot
void TaskExecutor::SaveScreenshot(int iteration) {
    auto& state = GlobalStateManager::GetInstance();

    int x1 = state.click1.x;
    int y1 = state.click1.y;
    int x2 = state.click2.x;
    int y2 = state.click2.y;

    std::string filename = "screenshot_" + std::to_string(iteration) + ".png";

    std::cout << "\n=== SAVING SCREENSHOT " << iteration << " ===\n";
    std::cout << "Point 1: (" << x1 << ", " << y1 << ")\n";
    std::cout << "Point 2: (" << x2 << ", " << y2 << ")\n";

    if (ScreenCapture::CaptureArea(x1, y1, x2, y2, filename)) {
        state.screenshotFiles.push_back(filename);
    }
    else {
        std::cerr << "[ERROR] Failed to save screenshot\n";
    }
    std::cout << "=====================================\n";
}

// РЕАЛИЗАЦИЯ GeneratePDFFilename
std::string TaskExecutor::GeneratePDFFilename() {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", &timeinfo);
    return "screenshots_" + std::string(timeStr) + ".pdf";
}

// РЕАЛИЗАЦИЯ PrintAreaInfo
void TaskExecutor::PrintAreaInfo() {
    auto& state = GlobalStateManager::GetInstance();
    int left = std::min<int>(state.click1.x, state.click2.x);
    int top = std::min<int>(state.click1.y, state.click2.y);
    int right = std::max<int>(state.click1.x, state.click2.x);
    int bottom = std::max<int>(state.click1.y, state.click2.y);

    std::cout << "\n=== SCREENSHOT AREA ===\n";
    std::cout << "  Top-left: (" << left << ", " << top << ")\n";
    std::cout << "  Bottom-right: (" << right << ", " << bottom << ")\n";
    std::cout << "  Size: " << (right - left) << "x" << (bottom - top) << "\n";
    std::cout << "========================\n";
}

// РЕАЛИЗАЦИЯ Execute
void TaskExecutor::Execute() {
    auto& state = GlobalStateManager::GetInstance();

    if (state.isProcessing) return;
    state.isProcessing = true;
    state.screenshotFiles.clear();

    MonitorManager::PrintMonitorInfo();

    std::cout << "\n=== SELECT SCREENSHOT AREA ===\n";
    std::cout << "Click and drag to select area\n";
    std::cout << "Press ENTER to confirm selection\n";
    std::cout << "Press ESC to cancel and exit\n\n";

    // Создаем оверлей для выделения
    OverlayWindow overlay;
    if (!overlay.Create()) {
        std::cerr << "[ERROR] Failed to create overlay window\n";
        state.isProcessing = false;
        return;
    }

    // Ждем завершения выделения (подтверждения Enter или отмены ESC)
    MSG msg;
    bool confirmed = false;
    bool cancelled = false;
    while (!confirmed && !cancelled) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                if (msg.wParam == 1) {
                    // Отмена по ESC
                    cancelled = true;
                    std::cout << "\n[Selection cancelled by user]\n";
                }
                else {
                    // Подтверждение Enter
                    confirmed = true;
                }
                break;
            }
        }
        Sleep(10);
    }

    overlay.Destroy();

    // Если отмена - завершаем задачу
    if (cancelled) {
        state.isProcessing = false;
        std::cout << "\n=== TASK CANCELLED ===\n";
        return;
    }

    // Проверяем, что выделение было подтверждено
    RECT selection = overlay.GetSelection();
    if (selection.left == 0 && selection.right == 0 && selection.top == 0 && selection.bottom == 0) {
        std::cerr << "[ERROR] Invalid selection\n";
        state.isProcessing = false;
        return;
    }

    state.click1.x = selection.left;
    state.click1.y = selection.top;
    state.click2.x = selection.right;
    state.click2.y = selection.bottom;

    if (state.click1.x >= state.click2.x || state.click1.y >= state.click2.y) {
        std::cerr << "[ERROR] Invalid selection\n";
        state.isProcessing = false;
        return;
    }

    PrintAreaInfo();

    std::cout << "\nPosition cursor at the page change location and press Ctrl+Shift+F12 to continue...\n";

    if (state.continueEvent) {
        CloseHandle(state.continueEvent);
    }
    state.continueEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    state.waitingForContinue = true;

    WaitForSingleObject(state.continueEvent, INFINITE);
    state.waitingForContinue = false;
    CloseHandle(state.continueEvent);
    state.continueEvent = nullptr;

    std::cout << "\nRunning loop from 1 to " << state.n << "\n";

    for (int i = 1; i <= state.n; i++) {
        std::cout << "\n--- Iteration " << i << " ---\n";

        std::cout << "  Ввод 4-х Backspaces...\n";
        KeyboardSimulator::GenerateBackspaces(4);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::cout << "  Ввод числа " << i << "...\n";
        KeyboardSimulator::GenerateNumberPress(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::cout << "  Ввод Enter...\n";
        KeyboardSimulator::GenerateEnterPress();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        SaveScreenshot(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!state.screenshotFiles.empty()) {
        std::cout << "\n=== CREATING PDF ===\n";
        std::string pdfFilename = GeneratePDFFilename();

        if (PDFGenerator::CreatePDF(state.screenshotFiles, pdfFilename)) {
            std::cout << "[PDF] File created: " << pdfFilename << "\n";
        }
        else {
            std::cerr << "[PDF] ERROR: Failed to create PDF\n";
        }
    }

    std::cout << "\n=== TASK COMPLETED ===\n";
    state.isProcessing = false;

    if (state.completionEvent) {
        SetEvent(state.completionEvent);
    }
}