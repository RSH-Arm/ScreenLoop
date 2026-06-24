#pragma once
#include "GlobalState.h"
#include "../screenshots/ScreenCapture.h"
#include "../pdf/PDFGenerator.h"
#include "../input/KeyboardSimulator.h"
#include <string>
#include <ctime>

class TaskExecutor {
public:
    static void Execute();

private:
    static void SaveScreenshot(int iteration);
    static std::string GeneratePDFFilename();
    static void PrintAreaInfo();
};