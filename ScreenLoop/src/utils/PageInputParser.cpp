#include "PageInputParser.h"
#include <sstream>
#include <regex>
#include <iostream>
#include <algorithm>

std::vector<int> PageInputParser::ParsePageString(const std::string& input, int maxPages) {
    std::set<int> pageSet;

    if (input.empty() || maxPages <= 0) {
        return std::vector<int>();
    }

    // Разбиваем по запятой
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, ',')) {
        // Удаляем пробелы
        token.erase(0, token.find_first_not_of(" \t\n\r"));
        token.erase(token.find_last_not_of(" \t\n\r") + 1);

        if (token.empty()) continue;

        // Проверяем, содержит ли токен дефис (диапазон)
        if (token.find('-') != std::string::npos) {
            auto rangePages = ParseRange(token, maxPages);
            pageSet.insert(rangePages.begin(), rangePages.end());
        }
        else {
            int page = ParseSingleNumber(token, maxPages);
            if (page > 0) {
                pageSet.insert(page);
            }
        }
    }

    // Преобразуем set в vector и сортируем
    std::vector<int> result(pageSet.begin(), pageSet.end());
    std::sort(result.begin(), result.end());
    return result;
}

bool PageInputParser::IsValidPageString(const std::string& input, int maxPages) {
    if (input.empty() || maxPages <= 0) return false;

    // Проверяем, что строка содержит только допустимые символы
    std::regex validPattern(R"(^[\d,\s-]+$)");
    if (!std::regex_match(input, validPattern)) {
        return false;
    }

    // Пробуем распарсить
    auto pages = ParsePageString(input, maxPages);
    return !pages.empty();
}

std::vector<int> PageInputParser::ParseRange(const std::string& range, int maxPages) {
    std::vector<int> result;

    size_t dashPos = range.find('-');
    if (dashPos == std::string::npos) return result;

    std::string startStr = range.substr(0, dashPos);
    std::string endStr = range.substr(dashPos + 1);

    // Удаляем пробелы
    startStr.erase(0, startStr.find_first_not_of(" \t\n\r"));
    startStr.erase(startStr.find_last_not_of(" \t\n\r") + 1);
    endStr.erase(0, endStr.find_first_not_of(" \t\n\r"));
    endStr.erase(endStr.find_last_not_of(" \t\n\r") + 1);

    int start = ParseSingleNumber(startStr, maxPages);
    int end = ParseSingleNumber(endStr, maxPages);

    if (start > 0 && end > 0 && start <= end && end <= maxPages) {
        for (int i = start; i <= end; ++i) {
            result.push_back(i);
        }
    }

    return result;
}

int PageInputParser::ParseSingleNumber(const std::string& num, int maxPages) {
    if (num.empty()) return -1;

    try {
        int page = std::stoi(num);
        if (page >= 1 && page <= maxPages) {
            return page;
        }
    }
    catch (...) {
        // Игнорируем ошибки парсинга
    }
    return -1;
}