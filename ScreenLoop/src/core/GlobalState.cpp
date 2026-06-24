#include "GlobalState.h"

GlobalState& GlobalStateManager::GetInstance() {
    static GlobalState instance;
    return instance;
}

void GlobalStateManager::Initialize() {
    InitializeCriticalSection(&GetInstance().cs);
}

void GlobalStateManager::Cleanup() {
    DeleteCriticalSection(&GetInstance().cs);
}