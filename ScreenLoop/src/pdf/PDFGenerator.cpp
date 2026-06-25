#include "PDFGenerator.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <windows.h>
#include <map>

// Получение размеров страницы
void PDFGenerator::GetPageSize(const std::string& format, const std::string& orientation,
    double& width, double& height) {
    // Размеры в пунктах для форматов ISO 216
    std::map<std::string, std::pair<double, double>> sizes = {
        {"A1", {1684.0, 2384.0}},
        {"A2", {1191.0, 1684.0}},
        {"A3", {842.0, 1191.0}},
        {"A4", {595.0, 842.0}},
        {"A5", {420.0, 595.0}}
    };

    auto it = sizes.find(format);
    if (it != sizes.end()) {
        if (orientation == "Landscape") {
            width = it->second.second;
            height = it->second.first;
        }
        else {
            width = it->second.first;
            height = it->second.second;
        }
    }
    else {
        // По умолчанию A4 Portrait
        width = 595.0;
        height = 842.0;
    }
}

bool PDFGenerator::ValidateImageFile(const std::string& filename) {
    if (GetFileAttributesA(filename.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return false;
    }

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (file && file.tellg() > 0) {
        return true;
    }
    return false;
}

HPDF_Page PDFGenerator::CreatePage(HPDF_Doc pdf, HPDF_Image image, int pageNumber, int totalPages,
    const std::string& format, const std::string& orientation) {
    double pageWidth, pageHeight;
    GetPageSize(format, orientation, pageWidth, pageHeight);

    HPDF_UINT imgWidth = HPDF_Image_GetWidth(image);
    HPDF_UINT imgHeight = HPDF_Image_GetHeight(image);

    double scaleX = pageWidth / imgWidth;
    double scaleY = pageHeight / imgHeight;
    double scale = std::min<double>(scaleX, scaleY) * 0.9;

    double imageWidth = imgWidth * scale;
    double imageHeight = imgHeight * scale;

    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetWidth(page, pageWidth);
    HPDF_Page_SetHeight(page, pageHeight);

    double x = (pageWidth - imageWidth) / 2;
    double y = (pageHeight - imageHeight) / 2;

    HPDF_Page_DrawImage(page, image, x, y, imageWidth, imageHeight);

    return page;
}

// Создание PDF с едиными настройками
bool PDFGenerator::CreatePDF(const std::vector<std::string>& imageFiles,
    const std::string& pdfFilename,
    const std::string& format,
    const std::string& orientation) {
    if (imageFiles.empty()) {
        std::cerr << "[PDF] No images to create PDF\n";
        return false;
    }

    HPDF_Doc pdf = HPDF_New(nullptr, nullptr);
    if (!pdf) {
        std::cerr << "[PDF] Failed to create PDF document\n";
        return false;
    }

    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);
    HPDF_SetInfoAttr(pdf, HPDF_INFO_CREATOR, "ScreenLoop");
    HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, "LibHaru + GDI+");

    int successCount = 0;

    for (size_t i = 0; i < imageFiles.size(); ++i) {
        if (!ValidateImageFile(imageFiles[i])) {
            std::cerr << "[PDF] Invalid or empty file: " << imageFiles[i] << "\n";
            continue;
        }

        HPDF_Image image = HPDF_LoadPngImageFromFile(pdf, imageFiles[i].c_str());
        if (!image) {
            std::cerr << "[PDF] Failed to load image: " << imageFiles[i] << "\n";
            continue;
        }

        if (CreatePage(pdf, image, i + 1, imageFiles.size(), format, orientation)) {
            successCount++;
        }
    }

    if (successCount == 0) {
        std::cerr << "[PDF] No pages added\n";
        HPDF_Free(pdf);
        return false;
    }

    HPDF_SaveToFile(pdf, pdfFilename.c_str());
    HPDF_Free(pdf);

    std::cout << "[PDF] Successfully created: " << pdfFilename << "\n";
    std::cout << "[PDF] Total pages: " << successCount << "\n";

    return true;
}

// Создание PDF с разными настройками для каждой страницы
bool PDFGenerator::CreateMixedPDF(const std::vector<std::string>& imageFiles,
    const std::vector<PDFConfig>& pageConfigs,
    const std::string& pdfFilename) {
    if (imageFiles.empty() || pageConfigs.empty()) {
        std::cerr << "[PDF] No images or configs to create PDF\n";
        return false;
    }

    if (imageFiles.size() != pageConfigs.size()) {
        std::cerr << "[PDF] Image files count does not match configs count\n";
        return false;
    }

    HPDF_Doc pdf = HPDF_New(nullptr, nullptr);
    if (!pdf) {
        std::cerr << "[PDF] Failed to create PDF document\n";
        return false;
    }

    HPDF_SetCompressionMode(pdf, HPDF_COMP_ALL);
    HPDF_SetInfoAttr(pdf, HPDF_INFO_CREATOR, "ScreenLoop");
    HPDF_SetInfoAttr(pdf, HPDF_INFO_PRODUCER, "LibHaru + GDI+");

    int successCount = 0;

    for (size_t i = 0; i < imageFiles.size(); ++i) {
        if (!ValidateImageFile(imageFiles[i])) {
            std::cerr << "[PDF] Invalid or empty file: " << imageFiles[i] << "\n";
            continue;
        }

        HPDF_Image image = HPDF_LoadPngImageFromFile(pdf, imageFiles[i].c_str());
        if (!image) {
            std::cerr << "[PDF] Failed to load image: " << imageFiles[i] << "\n";
            continue;
        }

        // Используем настройки для этой страницы
        const auto& config = pageConfigs[i];
        double pageWidth, pageHeight;
        GetPageSize(config.format, config.orientation, pageWidth, pageHeight);

        HPDF_UINT imgWidth = HPDF_Image_GetWidth(image);
        HPDF_UINT imgHeight = HPDF_Image_GetHeight(image);

        double scaleX = pageWidth / imgWidth;
        double scaleY = pageHeight / imgHeight;
        double scale = std::min<double>(scaleX, scaleY) * 0.9;

        double imageWidth = imgWidth * scale;
        double imageHeight = imgHeight * scale;

        HPDF_Page page = HPDF_AddPage(pdf);
        HPDF_Page_SetWidth(page, pageWidth);
        HPDF_Page_SetHeight(page, pageHeight);

        double x = (pageWidth - imageWidth) / 2;
        double y = (pageHeight - imageHeight) / 2;

        HPDF_Page_DrawImage(page, image, x, y, imageWidth, imageHeight);

        successCount++;
        std::cout << "  Page " << (i + 1) << ": " << config.format << " " << config.orientation << "\n";
    }

    if (successCount == 0) {
        std::cerr << "[PDF] No pages added\n";
        HPDF_Free(pdf);
        return false;
    }

    HPDF_SaveToFile(pdf, pdfFilename.c_str());
    HPDF_Free(pdf);

    std::cout << "[PDF] Successfully created: " << pdfFilename << "\n";
    std::cout << "[PDF] Total pages: " << successCount << "\n";

    return true;
}