#include "PDFGenerator.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <windows.h>

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

HPDF_Page PDFGenerator::CreatePage(HPDF_Doc pdf, HPDF_Image image, int pageNumber, int totalPages) {
    const double A4_WIDTH = 595.0;
    const double A4_HEIGHT = 842.0;

    HPDF_UINT imgWidth = HPDF_Image_GetWidth(image);
    HPDF_UINT imgHeight = HPDF_Image_GetHeight(image);

    double scaleX = A4_WIDTH / imgWidth;
    double scaleY = A4_HEIGHT / imgHeight;
    double scale = std::min<double>(scaleX, scaleY) * 0.9;

    double pageWidth = imgWidth * scale;
    double pageHeight = imgHeight * scale;

    HPDF_Page page = HPDF_AddPage(pdf);
    HPDF_Page_SetSize(page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

    double x = (A4_WIDTH - pageWidth) / 2;
    double y = (A4_HEIGHT - pageHeight) / 2;

    HPDF_Page_DrawImage(page, image, x, y, pageWidth, pageHeight);

    char pageNum[64];
    sprintf_s(pageNum, "%d / %d", pageNumber, totalPages);

    HPDF_Page_BeginText(page);
    HPDF_Page_SetFontAndSize(page, HPDF_GetFont(pdf, "Helvetica", nullptr), 10);
    double textWidth = HPDF_Page_TextWidth(page, pageNum);
    HPDF_Page_TextOut(page, (A4_WIDTH - textWidth) / 2, 20, pageNum);
    HPDF_Page_EndText(page);

    return page;
}

bool PDFGenerator::CreatePDF(const std::vector<std::string>& imageFiles, const std::string& pdfFilename) {
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

        if (CreatePage(pdf, image, i + 1, imageFiles.size())) {
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