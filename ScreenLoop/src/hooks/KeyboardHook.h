#pragma once
#include <windows.h>
#include "../core/GlobalState.h"

class KeyboardHook {
public:
    static KeyboardHook& GetInstance();

    bool Install();
    void Uninstall();
    void SetState(GlobalState& state);

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
    KeyboardHook() = default;
    ~KeyboardHook() = default;

    KeyboardHook(const KeyboardHook&) = delete;
    KeyboardHook& operator=(const KeyboardHook&) = delete;

    HHOOK m_hook = nullptr;
    GlobalState* m_state = nullptr;
};