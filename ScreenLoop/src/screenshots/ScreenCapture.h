#pragma once
#include <string>
#include <windows.h>    // Для WCHAR, CLSID
#include <gdiplus.h>    // Для CLSID (GDI+ определяет его как GUID)

class ScreenCapture {
public:
    static bool CaptureArea(int x1, int y1, int x2, int y2, const std::string& filename);

private:
    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
};