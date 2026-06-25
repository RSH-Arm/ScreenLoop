#pragma once
#include <windows.h>
#include <atomic>
#include <string>
#include <vector>
#include <map>
#include "../screenshots/MonitorManager.h"

// Структура для настроек PDF региона
struct PDFConfig {
    std::string format = "A4";          // Формат: A1, A2, A3, A4, A5
    std::string orientation = "Portrait"; // Portrait или Landscape
};

// Структура для хранения области захвата и связанных с ней страниц
struct CaptureRegion {
    RECT rect;                          // Область захвата
    std::vector<int> pages;             // Список страниц для этой области
    PDFConfig pdfConfig;                // Настройки PDF для этого региона
    std::string description;            // Описание для отладки
};

struct GlobalState {
    std::atomic<bool> hotkeyTriggered{ false };
    std::atomic<bool> isProcessing{ false };
    std::atomic<bool> waitingForContinue{ false };
    std::atomic<bool> appReady{ false };
    std::atomic<bool> isSelectingRegions{ false };

    int n = 0;  // Общее количество страниц

    // Список областей захвата
    std::vector<CaptureRegion> regions;

    RECT currentRect{ 0, 0, 0, 0 };

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