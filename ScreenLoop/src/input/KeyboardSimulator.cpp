#include "KeyboardSimulator.h"
#include <string>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

void KeyboardSimulator::GenerateKeyPress(WORD keyCode, bool extended) {
    INPUT input = { 0 };
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = keyCode;
    if (extended) {
        input.ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    }
    SendInput(1, &input, sizeof(INPUT));
    std::this_thread::sleep_for(15ms);

    input.ki.dwFlags = KEYEVENTF_KEYUP;
    if (extended) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    SendInput(1, &input, sizeof(INPUT));
    std::this_thread::sleep_for(15ms);
}

void KeyboardSimulator::GenerateDigitPress(int digit) {
    if (digit < 0 || digit > 9) return;
    GenerateKeyPress(0x30 + digit);
}

void KeyboardSimulator::GenerateNumberPress(int number) {
    std::string numStr = std::to_string(number);
    for (char c : numStr) {
        GenerateDigitPress(c - '0');
        std::this_thread::sleep_for(60ms);
    }
}

void KeyboardSimulator::GenerateBackspaces(int count) {
    for (int i = 0; i < count; ++i) {
        GenerateKeyPress(VK_BACK);
        std::this_thread::sleep_for(60ms);
    }
}

void KeyboardSimulator::GenerateEnterPress() {
    GenerateKeyPress(VK_RETURN);
}