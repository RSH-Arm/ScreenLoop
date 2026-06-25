#pragma once
#include "GlobalState.h"
#include "../screenshots/ScreenCapture.h"
#include "../pdf/PDFGenerator.h"
#include "../input/KeyboardSimulator.h"
#include <string>
#include <ctime>
#include <vector>

class TaskExecutor {
public:
    static void Execute();

private:
    static std::string GeneratePDFFilename(const std::string& suffix = "");  // ДОБАВЛЕН ПАРАМЕТР
    static void PrintAreaInfo(const RECT& rect);
    static bool GetRegionFromUser(CaptureRegion& region, int maxPages);
    static bool CapturePage(const RECT& rect, int pageNumber, const std::string& filename);
    static bool GetPDFSettingsFromUser(PDFConfig& config);
    static bool CreateMixedPDFFromRegions(const std::vector<CaptureRegion>& regions, int totalPages);  // ДОБАВЛЕНО
};