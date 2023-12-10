// AutoSprint.cpp: 定义应用程序的入口点。
//
#include <iostream>
#include "LightHook.h"

#include <Shlobj.h>
#include <Psapi.h>

#define INRANGE(x, a, b) (x >= a && x <= b)
#define GET_BYTE(x) (GET_BITS(x[0]) << 4 | GET_BITS(x[1]))
#define GET_BITS(x)                                                            \
  (INRANGE((x & (~0x20)), 'A', 'F') ? ((x & (~0x20)) - 'A' + 0xa)              \
                                    : (INRANGE(x, '0', '9') ? x - '0' : 0))

struct ControlKey
{
    bool Sneak;							// 潜行
    BYTE __unknowkey1;
    bool ProhibitionOfFloating;			// 禁止上浮(在水中无法上浮和创造模式自动下沉)
    BYTE __unknowkey3;
    BYTE __unknowkey4;
    BYTE __unknowkey5;
    bool Jump;
    bool Sprinting;
    bool WA;							// 向左前移动
    bool WD;							// 向右前移动
    bool W;
    bool S;
    bool A;
    bool D;
    BYTE __unknowkey6[2];
    BYTE __unknowkey7[16];
    BYTE __unknowkey8[8];
    float HorizontalRocker;				// 水平的值（左右 左1  右-1  好像是只读
    float VerticalRocker;				// 垂直的值 (前后 前1  后-1  好像是只读
};

// 使用特征码查找地址
auto findSig(const char* szSignature) -> uintptr_t {
    const char* pattern = szSignature;
    uintptr_t firstMatch = 0;

    static const auto rangeStart = (uintptr_t)GetModuleHandleA("Minecraft.Windows.exe");
    static MODULEINFO miModInfo;
    static bool init = false;

    if (!init) {
        init = true;
        GetModuleInformation(GetCurrentProcess(), (HMODULE)rangeStart, &miModInfo, sizeof(MODULEINFO));
    }

    static const uintptr_t rangeEnd = rangeStart + miModInfo.SizeOfImage;

    BYTE patByte = GET_BYTE(pattern);
    const char* oldPat = pattern;

    for (uintptr_t pCur = rangeStart; pCur < rangeEnd; pCur++) {
        if (!*pattern)
            return firstMatch;

        while (*(PBYTE)pattern == ' ')
            pattern++;

        if (!*pattern)
            return firstMatch;

        if (oldPat != pattern) {
            oldPat = pattern;
            if (*(PBYTE)pattern != '\?')
                patByte = GET_BYTE(pattern);
        }

        if (*(PBYTE)pattern == '\?' || *(BYTE*)pCur == patByte) {
            if (!firstMatch)
                firstMatch = pCur;

            if (!pattern[2] || !pattern[1])
                return firstMatch;

            pattern += 2;
        }
        else {
            pattern = szSignature;
            firstMatch = 0;
        }
    }

    return 0;
}

/**
 * @brief 从某个给定的地址开始 寻找特征码, 不超过 border 范围
 * @param szPtr 给定的开始地址
 * @param szSignature
 * @param border
 * @return
*/
uintptr_t FindSignatureRelay(uintptr_t szPtr, const char* szSignature, int border) {
    //uintptr_t startPtr = szPtr;
    const char* pattern = szSignature;
    for (;;) {
        if (border <= 0) return 0;
        pattern = szSignature;
        uintptr_t startPtr = szPtr;
        for (;;) {
            if (*pattern == ' ') {
                pattern++;
            }
            if (*pattern == '\0') {
                return szPtr;
            }
            if (*pattern == '\?') {
                pattern++;
                startPtr++;
                continue;
            }
            if (*(BYTE*)startPtr == GET_BYTE(pattern)) {
                pattern += 2;
                startPtr++;
                continue;
            }
            break;
        }
        szPtr++;
        border--;
    }
};

static HookInformation info;
static int status;
static int offset;
static uintptr_t ptr;
typedef void* (*LockControlInputCall)(void* thi, void* a2, void* a3, void* a4, void* a5, void* a6);

// Hook 后的关键函数
auto LockControlInputCallBack(void* thi, void* a2, void* a3, void* a4, void* a5, void* a6) -> void*
{
    auto control = (ControlKey*)(((uintptr_t)a2 + offset));
    control->Sprinting = true;

    auto original = (LockControlInputCall)info.Trampoline;
    return original(thi, a2, a3, a4, a5, a6);
}


auto start(HMODULE hModule) -> void {
    // 拿到要Hook的关键函数的指针
    ptr = findSig("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F 10 42 ? 48 8B D9");
    _ASSERT(ptr);

    // 拿到参数到 玩家行为控制系统的指针
    auto _offset = FindSignatureRelay(ptr, "0F 10 42", 32);
    _ASSERT(_offset);
    offset = (int)*reinterpret_cast<byte*>(_offset + 3);

    // 创建&开启Hook
    info = CreateHook((void*)ptr, (void*)&LockControlInputCallBack);
    status = EnableHook(&info);

}

// Dll入口函数
auto APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) -> BOOL {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)start, hModule, NULL, nullptr);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        if (status) {
            // 反注入则关闭Hook
            DisableHook(&info);
        }
    }

    return TRUE;
}