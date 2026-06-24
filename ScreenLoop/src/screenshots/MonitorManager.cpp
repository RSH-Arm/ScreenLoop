#include "MonitorManager.h"
#include <iostream>

BOOL CALLBACK MonitorManager::MonitorEnumProc(HMONITOR hMonitor, HDC hdc, LPRECT lprcMonitor, LPARAM dwData) {
    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);

    MONITORINFOEX mi;
    mi.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(hMonitor, &mi);

    MonitorInfo info;
    info.rect = mi.rcMonitor;
    info.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;

    char deviceName[32];
    WideCharToMultiByte(CP_ACP, 0, mi.szDevice, -1, deviceName, sizeof(deviceName), nullptr, nullptr);
    info.name = deviceName;

    monitors->push_back(info);
    return TRUE;
}

void MonitorManager::EnumerateMonitors(std::vector<MonitorInfo>& monitors) {
    monitors.clear();
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));
}

void MonitorManager::PrintMonitorInfo() {
    std::vector<MonitorInfo> monitors;
    EnumerateMonitors(monitors);

    std::cout << "\n=== MONITOR INFORMATION ===\n";
    for (const auto& mon : monitors) {
        int width = mon.rect.right - mon.rect.left;
        int height = mon.rect.bottom - mon.rect.top;

        std::cout << "Monitor: " << mon.name << "\n";
        std::cout << "  Position: (" << mon.rect.left << ", " << mon.rect.top << ")\n";
        std::cout << "  Size: " << width << "x" << height << "\n";
        std::cout << "  Primary: " << (mon.isPrimary ? "YES" : "NO") << "\n";
        std::cout << "  Area: (" << mon.rect.left << ", " << mon.rect.top << ") - ("
            << mon.rect.right << ", " << mon.rect.bottom << ")\n\n";
    }

    int vLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    std::cout << "VIRTUAL SCREEN:\n";
    std::cout << "  Position: (" << vLeft << ", " << vTop << ")\n";
    std::cout << "  Size: " << vWidth << "x" << vHeight << "\n";
    std::cout << "=============================\n\n";
}