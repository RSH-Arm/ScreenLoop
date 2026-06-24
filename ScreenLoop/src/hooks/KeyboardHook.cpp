#include "KeyboardHook.h"
#include "../core/GlobalState.h"
#include "../core/TaskExecutor.h"
#include <thread>
#include <iostream>

KeyboardHook& KeyboardHook::GetInstance() {
    static KeyboardHook instance;
    return instance;
}

void KeyboardHook::SetState(GlobalState& state) {
    m_state = &state;
}

bool KeyboardHook::Install() {
    m_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    return m_hook != nullptr;
}

void KeyboardHook::Uninstall() {
    if (m_hook) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }
}

LRESULT CALLBACK KeyboardHook::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        auto* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (pKeyboard->vkCode == VK_F12) {
                bool ctrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

                if (ctrlPressed && shiftPressed) {
                    auto& state = GlobalStateManager::GetInstance();

                    // ПРОВЕРКА: Приложение готово?
                    if (!state.appReady) {
                        return CallNextHookEx(nullptr, nCode, wParam, lParam);
                    }

                    if (state.waitingForContinue && state.continueEvent) {
                        std::cout << "\n[Continue!]\n";
                        SetEvent(state.continueEvent);
                        return CallNextHookEx(nullptr, nCode, wParam, lParam);
                    }

                    if (!state.isProcessing && !state.waitingForContinue) {
                        state.hotkeyTriggered = true;
                        std::cout << "\n[Hotkey triggered!]\n";

                        if (state.completionEvent) {
                            CloseHandle(state.completionEvent);
                        }
                        state.completionEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

                        std::thread([]() {
                            TaskExecutor::Execute();
                            }).detach();
                    }
                }
            }
        }
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}