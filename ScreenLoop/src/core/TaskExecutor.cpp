#include "TaskExecutor.h"
#include "../overlay/OverlayWindow.h"
#include "../utils/PageInputParser.h"
#include "../utils/PDFSettings.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <algorithm>
#include <limits>
#include <vector>

// РЕАЛИЗАЦИЯ GeneratePDFFilename
std::string TaskExecutor::GeneratePDFFilename(const std::string& suffix) {
    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", &timeinfo);

    if (suffix.empty()) {
        return "screenshots_" + std::string(timeStr) + ".pdf";
    }
    else {
        return "screenshots_" + std::string(timeStr) + "_" + suffix + ".pdf";
    }
}

// РЕАЛИЗАЦИЯ PrintAreaInfo
void TaskExecutor::PrintAreaInfo(const RECT& rect) {
    int left = std::min<int>(rect.left, rect.right);
    int top = std::min<int>(rect.top, rect.bottom);
    int right = std::max<int>(rect.left, rect.right);
    int bottom = std::max<int>(rect.top, rect.bottom);

    std::cout << "  Top-left: (" << left << ", " << top << ")\n";
    std::cout << "  Bottom-right: (" << right << ", " << bottom << ")\n";
    std::cout << "  Size: " << (right - left) << "x" << (bottom - top) << "\n";
}

// РЕАЛИЗАЦИЯ CapturePage
bool TaskExecutor::CapturePage(const RECT& rect, int pageNumber, const std::string& filename) {
    int left = std::min<int>(rect.left, rect.right);
    int top = std::min<int>(rect.top, rect.bottom);
    int right = std::max<int>(rect.left, rect.right);
    int bottom = std::max<int>(rect.top, rect.bottom);

    std::cout << "\n=== CAPTURING PAGE " << pageNumber << " ===\n";
    std::cout << "Area: (" << left << ", " << top << ") - (" << right << ", " << bottom << ")\n";
    std::cout << "Size: " << (right - left) << "x" << (bottom - top) << "\n";

    if (ScreenCapture::CaptureArea(left, top, right, bottom, filename)) {
        std::cout << "  Saved: " << filename << "\n";
        return true;
    }
    else {
        std::cerr << "[ERROR] Failed to capture page " << pageNumber << "\n";
        return false;
    }
}

// РЕАЛИЗАЦИЯ GetPDFSettingsFromUser
bool TaskExecutor::GetPDFSettingsFromUser(PDFConfig& config) {
    std::cout << "\n=== PDF SETTINGS FOR THIS REGION ===\n";
    std::cout << "Select paper format and orientation for this region.\n\n";

    const auto& formats = PDFSettings::GetFormats();
    std::cout << "Available formats:\n";
    for (size_t i = 0; i < formats.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << formats[i] << "\n";
    }
    std::cout << "Enter format number (default: 4 - A4): ";

    int formatChoice;
    std::cin >> formatChoice;
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    if (formatChoice >= 1 && formatChoice <= static_cast<int>(formats.size())) {
        config.format = formats[formatChoice - 1];
    }
    else {
        config.format = "A4";
        std::cout << "Invalid choice, using default: A4\n";
    }

    const auto& orientations = PDFSettings::GetOrientations();
    std::cout << "\nAvailable orientations:\n";
    for (size_t i = 0; i < orientations.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << orientations[i] << "\n";
    }
    std::cout << "Enter orientation number (default: 1 - Portrait): ";

    int orientationChoice;
    std::cin >> orientationChoice;
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

    if (orientationChoice >= 1 && orientationChoice <= static_cast<int>(orientations.size())) {
        config.orientation = orientations[orientationChoice - 1];
    }
    else {
        config.orientation = "Portrait";
        std::cout << "Invalid choice, using default: Portrait\n";
    }

    std::cout << "\nRegion PDF Settings:\n";
    std::cout << "  Format: " << config.format << "\n";
    std::cout << "  Orientation: " << config.orientation << "\n";
    std::cout << "===================================\n";

    return true;
}

// РЕАЛИЗАЦИЯ GetRegionFromUser - с ограничением по totalPages
bool TaskExecutor::GetRegionFromUser(CaptureRegion& region, int maxPages) {
    std::cout << "\n=== SELECT SCREENSHOT AREA ===\n";
    std::cout << "Click and drag to select area\n";
    std::cout << "Press ENTER to confirm selection\n";
    std::cout << "Press ESC to cancel and exit\n\n";

    OverlayWindow overlay;
    if (!overlay.Create()) {
        std::cerr << "[ERROR] Failed to create overlay window\n";
        return false;
    }

    MSG msg;
    bool confirmed = false;
    bool cancelled = false;

    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    while (!confirmed && !cancelled) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT) {
                if (msg.wParam == 1) {
                    cancelled = true;
                }
                else {
                    confirmed = true;
                }
                break;
            }
        }
        Sleep(10);
    }

    RECT selection = overlay.GetSelection();
    overlay.Destroy();

    if (cancelled) {
        return false;
    }

    if (selection.left == 0 && selection.right == 0 &&
        selection.top == 0 && selection.bottom == 0) {
        std::cerr << "[ERROR] Invalid selection\n";
        return false;
    }

    region.rect = selection;

    std::cout << "\n=== SELECTED AREA ===\n";
    PrintAreaInfo(region.rect);
    std::cout << "========================\n";

    std::cout << "\n=== ENTER PAGES ===\n";
    std::cout << "Enter pages for this area (examples):\n";
    std::cout << "  - Range: 1-100\n";
    std::cout << "  - List: 1,2,3,4,5\n";
    std::cout << "  - Combined: 1-10,15,20-25\n";
    std::cout << "Available pages: 1 to " << maxPages << "\n";
    std::cout << "Enter pages: ";

    std::string input;
    std::getline(std::cin, input);

    if (input.empty()) {
        std::cerr << "[ERROR] Empty input. Please enter page numbers.\n";
        return false;
    }

    // Парсим с ограничением maxPages
    auto pages = PageInputParser::ParsePageString(input, maxPages);
    if (pages.empty()) {
        std::cerr << "[ERROR] Invalid page input. Please try again.\n";
        return false;
    }

    region.pages = pages;

    std::cout << "\nPages assigned: ";
    for (size_t i = 0; i < pages.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << pages[i];
    }
    std::cout << "\n";

    if (!GetPDFSettingsFromUser(region.pdfConfig)) {
        std::cerr << "[ERROR] Failed to get PDF settings\n";
        return false;
    }

    return true;
}

// РЕАЛИЗАЦИЯ CreateMixedPDFFromRegions
bool TaskExecutor::CreateMixedPDFFromRegions(const std::vector<CaptureRegion>& regions) {
    if (regions.empty()) {
        std::cerr << "[PDF] No regions\n";
        return false;
    }

    std::set<int> allPages;
    for (const auto& region : regions) {
        allPages.insert(region.pages.begin(), region.pages.end());
    }

    if (allPages.empty()) {
        std::cerr << "[PDF] No pages found in regions\n";
        return false;
    }

    std::cout << "\n=== CREATING SINGLE PDF WITH MIXED PAGE SIZES ===\n";
    std::cout << "Total unique pages: " << allPages.size() << "\n";

    std::vector<std::string> orderedFiles;
    std::vector<PDFConfig> pageConfigs;

    for (int page : allPages) {
        bool found = false;
        int regionIndex = -1;

        for (size_t i = 0; i < regions.size(); ++i) {
            const auto& region = regions[i];
            if (std::find(region.pages.begin(), region.pages.end(), page) != region.pages.end()) {
                found = true;
                regionIndex = static_cast<int>(i);
                break;
            }
        }

        if (!found) {
            std::cout << "  Page " << page << " has no region, skipping\n";
            continue;
        }

        std::string filename = "page_" + std::to_string(page) + "_region_" +
            std::to_string(regionIndex + 1) + ".png";

        if (!PDFGenerator::ValidateImageFile(filename)) {
            std::cerr << "[PDF] File not found: " << filename << "\n";
            continue;
        }

        orderedFiles.push_back(filename);
        pageConfigs.push_back(regions[regionIndex].pdfConfig);

        std::cout << "  Page " << page << ": "
            << regions[regionIndex].pdfConfig.format << " "
            << regions[regionIndex].pdfConfig.orientation << "\n";
    }

    if (orderedFiles.empty()) {
        std::cerr << "[PDF] No valid files found\n";
        return false;
    }

    std::string pdfFilename = GeneratePDFFilename("mixed");

    if (PDFGenerator::CreateMixedPDF(orderedFiles, pageConfigs, pdfFilename)) {
        std::cout << "\n[PDF] File created: " << pdfFilename << "\n";
        std::cout << "[PDF] Total pages: " << orderedFiles.size() << "\n";
        return true;
    }
    else {
        std::cerr << "[PDF] ERROR: Failed to create mixed PDF\n";
        return false;
    }
}

// РЕАЛИЗАЦИЯ Execute
void TaskExecutor::Execute() {
    auto& state = GlobalStateManager::GetInstance();

    if (state.isProcessing) return;
    state.isProcessing = true;
    state.screenshotFiles.clear();
    state.regions.clear();

    MonitorManager::PrintMonitorInfo();

    std::cout << "\n=== MULTI-REGION CAPTURE SETUP ===\n";
    std::cout << "You will define one or more capture regions.\n";
    std::cout << "Each region can be assigned to specific pages and PDF settings.\n";
    std::cout << "Total pages in document: " << state.totalPages << "\n\n";

    bool continueAdding = true;
    while (continueAdding) {
        CaptureRegion region;
        if (!GetRegionFromUser(region, state.totalPages)) {
            std::cout << "\n=== CAPTURE SETUP CANCELLED ===\n";
            state.isProcessing = false;
            return;
        }

        state.regions.push_back(region);

        std::cout << "\n=== ADD ANOTHER REGION? ===\n";
        std::cout << "Do you want to add another capture region? (y/n): ";
        std::string answer;
        std::getline(std::cin, answer);

        std::cin.clear();

        if (answer != "y" && answer != "Y" && answer != "yes" && answer != "Yes") {
            continueAdding = false;
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    if (state.regions.empty()) {
        std::cerr << "[ERROR] No regions defined\n";
        state.isProcessing = false;
        return;
    }

    // Собираем все уникальные страницы
    std::set<int> allPages;
    for (const auto& region : state.regions) {
        allPages.insert(region.pages.begin(), region.pages.end());
    }

    std::cout << "\n=== CAPTURE REGIONS SUMMARY ===\n";
    for (size_t i = 0; i < state.regions.size(); ++i) {
        const auto& region = state.regions[i];
        std::cout << "Region " << (i + 1) << ":\n";
        PrintAreaInfo(region.rect);
        std::cout << "  Pages: ";
        for (size_t j = 0; j < region.pages.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << region.pages[j];
        }
        std::cout << "\n";
        std::cout << "  PDF Format: " << region.pdfConfig.format << "\n";
        std::cout << "  PDF Orientation: " << region.pdfConfig.orientation << "\n";
    }
    std::cout << "============================\n";
    std::cout << "Total unique pages to capture: " << allPages.size() << "\n";

    std::cout << "\nPosition cursor at the page change location and press Ctrl+Shift+F12 to start capture...\n";

    if (state.continueEvent) {
        CloseHandle(state.continueEvent);
    }
    state.continueEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    state.waitingForContinue = true;

    WaitForSingleObject(state.continueEvent, INFINITE);
    state.waitingForContinue = false;
    CloseHandle(state.continueEvent);
    state.continueEvent = nullptr;

    std::cout << "\n=== STARTING AUTOMATIC CAPTURE ===\n";
    std::cout << "Total unique pages: " << allPages.size() << "\n";
    std::cout << "Total regions: " << state.regions.size() << "\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Захват страниц
    for (int page : allPages) {
        std::cout << "\n--- Page " << page << " ---\n";

        std::cout << "  Ввод 4-х Backspaces...\n";
        KeyboardSimulator::GenerateBackspaces(4);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::cout << "  Ввод числа " << page << "...\n";
        KeyboardSimulator::GenerateNumberPress(page);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::cout << "  Ввод Enter...\n";
        KeyboardSimulator::GenerateEnterPress();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        bool regionFound = false;
        for (size_t i = 0; i < state.regions.size(); ++i) {
            const auto& region = state.regions[i];
            if (std::find(region.pages.begin(), region.pages.end(), page) != region.pages.end()) {
                std::string filename = "page_" + std::to_string(page) + "_region_" + std::to_string(i + 1) + ".png";

                if (CapturePage(region.rect, page, filename)) {
                    state.screenshotFiles.push_back(filename);
                }

                regionFound = true;
                break;
            }
        }

        if (!regionFound) {
            std::cout << "  No region assigned for this page, skipping\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n=== CAPTURE COMPLETE ===\n";
    std::cout << "Captured " << state.screenshotFiles.size() << " pages\n";

    if (!state.regions.empty()) {
        CreateMixedPDFFromRegions(state.regions);
    }

    std::cout << "\n=== TASK COMPLETED ===\n";
    state.isProcessing = false;

    if (state.completionEvent) {
        SetEvent(state.completionEvent);
    }
}