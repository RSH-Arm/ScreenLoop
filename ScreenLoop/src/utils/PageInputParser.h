#pragma once
#include <string>
#include <vector>
#include <set>

class PageInputParser {
public:
    // Парсит строку со страницами и возвращает отсортированный список уникальных страниц
    static std::vector<int> ParsePageString(const std::string& input, int maxPages);

    // Проверяет валидность строки
    static bool IsValidPageString(const std::string& input, int maxPages);

private:
    // Парсит диапазон "1-10"
    static std::vector<int> ParseRange(const std::string& range, int maxPages);

    // Парсит одиночное число
    static int ParseSingleNumber(const std::string& num, int maxPages);
};