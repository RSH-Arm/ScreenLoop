#pragma once
#include <string>
#include <vector>
#include <set>
#include <climits>

class PageInputParser {
public:
    static std::vector<int> ParsePageString(const std::string& input, int maxPages = INT_MAX);
    static bool IsValidPageString(const std::string& input, int maxPages = INT_MAX);

private:
    static std::vector<int> ParseRange(const std::string& range, int maxPages);
    static int ParseSingleNumber(const std::string& num, int maxPages);
};