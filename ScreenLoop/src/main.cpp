#include "core/Application.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#pragma comment(lib, "libhpdf.lib")

#include <iostream>

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    Application app;
    return app.Run();
}