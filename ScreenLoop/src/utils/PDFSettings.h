#pragma once
#include <string>
#include <vector>
#include <algorithm>

class PDFSettings {
public:
    // Список поддерживаемых форматов
    static const std::vector<std::string>& GetFormats() {
        static const std::vector<std::string> formats = { "A1", "A2", "A3", "A4", "A5" };
        return formats;
    }

    // Список поддерживаемых ориентаций
    static const std::vector<std::string>& GetOrientations() {
        static const std::vector<std::string> orientations = { "Portrait", "Landscape" };
        return orientations;
    }

    // Проверка валидности формата
    static bool IsValidFormat(const std::string& format) {
        const auto& formats = GetFormats();
        return std::find(formats.begin(), formats.end(), format) != formats.end();
    }

    // Проверка валидности ориентации
    static bool IsValidOrientation(const std::string& orientation) {
        const auto& orientations = GetOrientations();
        return std::find(orientations.begin(), orientations.end(), orientation) != orientations.end();
    }

    // Получение размеров страницы в пунктах (1 пункт = 1/72 дюйма)
    static void GetPageSize(const std::string& format, const std::string& orientation,
        double& width, double& height) {
        // Размеры в пунктах для форматов ISO 216
        struct PageSize {
            std::string format;
            double width;
            double height;
        };

        static const PageSize sizes[] = {
            {"A1", 1684.0, 2384.0},
            {"A2", 1191.0, 1684.0},
            {"A3", 842.0, 1191.0},
            {"A4", 595.0, 842.0},
            {"A5", 420.0, 595.0}
        };

        for (const auto& size : sizes) {
            if (size.format == format) {
                if (orientation == "Landscape") {
                    width = size.height;
                    height = size.width;
                }
                else {
                    width = size.width;
                    height = size.height;
                }
                return;
            }
        }

        // По умолчанию A4 Portrait
        width = 595.0;
        height = 842.0;
    }
};