#pragma once
#include <windows.h>

class Application {
public:
    Application();
    ~Application();

    int Run();

private:
    bool Initialize();
    void Shutdown();
    void PrintInstructions();
    void WaitForHotkey();
};