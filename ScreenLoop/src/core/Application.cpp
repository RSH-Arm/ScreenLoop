#include "Application.h"
#include "GlobalState.h"
#include "../hooks/KeyboardHook.h"
#include "../screenshots/MonitorManager.h"
#include <iostream>
#include <thread>
#include <limits> 

#include <windows.h>
#include <gdiplus.h>

using namespace Gdiplus;

Application::Application() {
    GlobalStateManager::Initialize();
    KeyboardHook::GetInstance().SetState(GlobalStateManager::GetInstance());
}

Application::~Application() {
    Shutdown();
}

bool Application::Initialize() {
    // DPI Awareness
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        auto pSetCtx = (decltype(&SetProcessDpiAwarenessContext))GetProcAddress(hUser, "SetProcessDpiAwarenessContext");
        if (pSetCtx) {
            pSetCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
    }

    // GDI+ Initialization
    GdiplusStartupInput startupInput;
    auto& state = GlobalStateManager::GetInstance();
    state.appReady = true; // Пометить приложение как готовое
    if (GdiplusStartup(&state.gdiplusToken, &startupInput, nullptr) != Ok) {
        std::cerr << "ERROR: Failed to initialize GDI+\n";
        return false;
    }
    state.gdiplusInitialized = true;

    return true;
}

void Application::Shutdown() {
    auto& state = GlobalStateManager::GetInstance();

    if (state.completionEvent) CloseHandle(state.completionEvent);
    if (state.continueEvent) CloseHandle(state.continueEvent);
    if (state.gdiplusInitialized) GdiplusShutdown(state.gdiplusToken);

    GlobalStateManager::Cleanup();
}

void Application::PrintInstructions() {
    std::cout << "========================================\n";
    std::cout << "   SCREENLOOP - PDF AUTOMATION\n";
    std::cout << "========================================\n\n";

    MonitorManager::PrintMonitorInfo();

    std::cout << "Enter total number of pages in the document (n > 0): ";
    auto& state = GlobalStateManager::GetInstance();
    std::cin >> state.totalPages;

    while (state.totalPages <= 0) {
        std::cout << "Error! Try again: ";
        std::cin >> state.totalPages;
    }

    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    std::cout << "Total pages: " << state.totalPages << "\n\n";

    std::cout << "HOTKEY: Ctrl+Shift+F12\n";
    std::cout << "INSTRUCTIONS:\n";
    std::cout << "  1. Press Ctrl+Shift+F12 to start\n";
    std::cout << "  2. Define capture regions one by one\n";
    std::cout << "  3. For each region, enter page numbers (e.g., 1-10, 15, 20-25)\n";
    std::cout << "  4. All page numbers must be within 1-" << state.totalPages << "\n";
    std::cout << "  5. After all regions, capture will start\n";
    std::cout << "  6. Press Ctrl+Shift+F12 to start automatic capture\n";
    std::cout << "  7. Program will create PDF with all captures\n\n";
}

void Application::WaitForHotkey() {
    std::cout << "Waiting for hotkey...\n\n";

    // Очистить очередь сообщений перед ожиданием
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

int Application::Run() {
    if (!Initialize()) {
        system("pause");
        return 1;
    }

    PrintInstructions();

    KeyboardHook& hook = KeyboardHook::GetInstance();
    if (!hook.Install()) {
        std::cerr << "ERROR: Failed to install keyboard hook!\n";
        std::cerr << "Run program with administrator privileges.\n";
        Shutdown();
        system("pause");
        return 1;
    }

    WaitForHotkey();

    hook.Uninstall();
    Shutdown();
    return 0;
}