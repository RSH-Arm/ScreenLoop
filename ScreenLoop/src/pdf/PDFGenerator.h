#pragma once
#include <string>
#include <vector>
#include "../core/GlobalState.h"  // ДОБАВЛЯЕМ для PDFConfig
#include "hpdf.h"

class PDFGenerator {
public:
    // Создание PDF с едиными настройками для всех страниц
    static bool CreatePDF(const std::vector<std::string>& imageFiles,
        const std::string& pdfFilename,
        const std::string& format = "A4",
        const std::string& orientation = "Portrait");

    // Создание PDF с разными настройками для каждой страницы
    static bool CreateMixedPDF(const std::vector<std::string>& imageFiles,
        const std::vector<PDFConfig>& pageConfigs,
        const std::string& pdfFilename);

    // Вспомогательные методы (публичные)
    static bool ValidateImageFile(const std::string& filename);
    static void GetPageSize(const std::string& format, const std::string& orientation,
        double& width, double& height);

private:
    static HPDF_Page CreatePage(HPDF_Doc pdf, HPDF_Image image, int pageNumber, int totalPages,
        const std::string& format, const std::string& orientation);
};