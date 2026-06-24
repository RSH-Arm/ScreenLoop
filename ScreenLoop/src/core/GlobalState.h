#pragma once
#include <windows.h>
#include <atomic>
#include <string>
#include <vector>
#include "../screenshots/MonitorManager.h"

struct GlobalState {
    std::atomic<bool> hotkeyTriggered{ false };
    std::atomic<bool> isProcessing{ false };
    std::atomic<bool> waitingForContinue{ false };
    std::atomic<bool> appReady{ false };

    RECT selectionRect{ 0, 0, 0, 0 };

    int n = 0;

    POINT click1{ 0, 0 };
    POINT click2{ 0, 0 };

    HANDLE completionEvent = nullptr;
    HANDLE continueEvent = nullptr;

    ULONG_PTR gdiplusToken = 0;
    bool gdiplusInitialized = false;

    std::vector<MonitorInfo> monitors;
    std::vector<std::string> screenshotFiles;
    std::string outputDirectory;

    CRITICAL_SECTION cs;
};

class GlobalStateManager {
public:
    static GlobalState& GetInstance();
    static void Initialize();
    static void Cleanup();

private:
    GlobalStateManager() = default;
};