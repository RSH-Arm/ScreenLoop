#pragma once
#include <windows.h>
#include <string>
#include <vector>

struct MonitorInfo {
    RECT rect;
    std::string name;
    bool isPrimary;
};

class MonitorManager {
public:
    static void EnumerateMonitors(std::vector<MonitorInfo>& monitors);
    static void PrintMonitorInfo();

private:
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData);
};