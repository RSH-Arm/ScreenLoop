#include "ScreenCapture.h"
#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace Gdiplus;

int ScreenCapture::GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    auto* pImageCodecInfo = (ImageCodecInfo*)malloc(size);
    if (!pImageCodecInfo) return -1;

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }

    free(pImageCodecInfo);
    return -1;
}

bool ScreenCapture::CaptureArea(int x1, int y1, int x2, int y2, const std::string& filename) {
    try {
        int left = std::min<int>(x1, x2);
        int top = std::min<int>(y1, y2);
        int right = std::max<int>(x1, x2);
        int bottom = std::max<int>(y1, y2);

        int width = right - left;
        int height = bottom - top;

        if (width <= 0 || height <= 0) {
            std::cerr << "[ERROR] Invalid dimensions\n";
            return false;
        }

        int vLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int vTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int vWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int vHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        HDC hdcScreen = GetDC(nullptr);
        if (!hdcScreen) return false;

        HDC hdcFull = CreateCompatibleDC(hdcScreen);
        HBITMAP hFullBitmap = CreateCompatibleBitmap(hdcScreen, vWidth, vHeight);
        HBITMAP hOldFull = (HBITMAP)SelectObject(hdcFull, hFullBitmap);
        BitBlt(hdcFull, 0, 0, vWidth, vHeight, hdcScreen, vLeft, vTop, SRCCOPY);

        HDC hdcResult = CreateCompatibleDC(hdcScreen);
        HBITMAP hResultBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
        HBITMAP hOldResult = (HBITMAP)SelectObject(hdcResult, hResultBitmap);
        BitBlt(hdcResult, 0, 0, width, height, hdcFull, left - vLeft, top - vTop, SRCCOPY);

        Bitmap bitmap(hResultBitmap, nullptr);
        if (bitmap.GetLastStatus() != Ok) {
            // Cleanup
            SelectObject(hdcResult, hOldResult);
            DeleteObject(hResultBitmap);
            DeleteDC(hdcResult);
            SelectObject(hdcFull, hOldFull);
            DeleteObject(hFullBitmap);
            DeleteDC(hdcFull);
            ReleaseDC(nullptr, hdcScreen);
            return false;
        }

        CLSID pngClsid;
        if (GetEncoderClsid(L"image/png", &pngClsid) == -1) {
            // Cleanup
            return false;
        }

        EncoderParameters encoderParams;
        encoderParams.Count = 1;
        encoderParams.Parameter[0].Guid = EncoderColorDepth;
        encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
        encoderParams.Parameter[0].NumberOfValues = 1;
        ULONG colorDepth = 24;
        encoderParams.Parameter[0].Value = &colorDepth;

        std::wstring wideFilename(filename.begin(), filename.end());
        Status saveStatus = bitmap.Save(wideFilename.c_str(), &pngClsid, &encoderParams);

        // Cleanup
        SelectObject(hdcResult, hOldResult);
        DeleteObject(hResultBitmap);
        DeleteDC(hdcResult);
        SelectObject(hdcFull, hOldFull);
        DeleteObject(hFullBitmap);
        DeleteDC(hdcFull);
        ReleaseDC(nullptr, hdcScreen);

        return saveStatus == Ok;
    }
    catch (...) {
        return false;
    }
}