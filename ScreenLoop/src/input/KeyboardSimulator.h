#pragma once
#include <windows.h>

class KeyboardSimulator {
public:
    static void GenerateKeyPress(WORD keyCode, bool extended = false);
    static void GenerateDigitPress(int digit);
    static void GenerateNumberPress(int number);
    static void GenerateBackspaces(int count);
    static void GenerateEnterPress();
};