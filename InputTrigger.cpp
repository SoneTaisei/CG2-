#include "InputTrigger.h"
#include <Windows.h>
#include <dinput.h> // DirectInput 用

// 入力状態
char keys[256] = { 0 };
char prevKeys[256] = { 0 };

// 外部から呼び出して更新する
void UpdateInputState(IDirectInputDevice8 *keyboard) {
    memcpy(prevKeys, keys, 256);
    keyboard->Acquire();
    keyboard->GetDeviceState(256, keys);
}

// キーが押されているか
bool IsKeyHeld(uint8_t key) {
    return keys[key] & 0x80;
}

// キーが押された瞬間
bool IsKeyPressed(uint8_t key) {
    return (keys[key] & 0x80) && !(prevKeys[key] & 0x80);
}

// キーが離された瞬間
bool IsKeyReleased(uint8_t key) {
    return !(keys[key] & 0x80) && (prevKeys[key] & 0x80);
}