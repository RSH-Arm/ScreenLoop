#pragma once
#include <string>
#include <vector>
#include "hpdf.h"

class PDFGenerator {
public:
    static bool CreatePDF(const std::vector<std::string>& imageFiles, const std::string& pdfFilename);

private:
    static bool ValidateImageFile(const std::string& filename);
    static HPDF_Page CreatePage(HPDF_Doc pdf, HPDF_Image image, int pageNumber, int totalPages);
};