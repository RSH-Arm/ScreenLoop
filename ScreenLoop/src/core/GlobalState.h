#pragma once
#include <windows.h>
#include <atomic>
#include <string>
#include <vector>
#include <map>
#include "../screenshots/MonitorManager.h"

// Структура для настроек PDF региона
struct PDFConfig {
    std::string format = "A4";
    std::string orientation = "Portrait";
};

// Структура для хранения области захвата и связанных с ней страниц
struct CaptureRegion {
    RECT rect;
    std::vector<int> pages;
    PDFConfig pdfConfig;
    std::string description;
};

struct GlobalState {
    std::atomic<bool> hotkeyTriggered{ false };
    std::atomic<bool> isProcessing{ false };
    std::atomic<bool> waitingForContinue{ false };
    std::atomic<bool> appReady{ false };
    std::atomic<bool> isSelectingRegions{ false };

    int totalPages = 0;  // Общее количество страниц в документе (для ограничения)

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