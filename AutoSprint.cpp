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
static auto findSig(const char* szSignature) -> uintptr_t {
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
static uintptr_t FindSignatureRelay(uintptr_t szPtr, const char* szSignature, int border) {
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
typedef void* (*LockControlInputCall)(void* thi, ControlKey* a2);
typedef void* (*LockControlInputCall2)(void* thi, ControlKey* a2, void* a3, void* a4, void* a5, void* a6, void* a7, void* a8, void* a9, void* a10, void* a11);

// Hook 后的关键函数
auto LockControlInputCallBack(void* thi, ControlKey* a2) -> void*
{
    //auto control = (ControlKey*)(((uintptr_t)a2 + offset));
    a2->Sprinting = true;

    auto original = (LockControlInputCall)info.Trampoline;
    return original(thi, a2);
}

auto LockControlInputCallBack2(void* thi, ControlKey* a2, void* a3, void* a4, void* a5, void* a6, void* a7, void* a8, void* a9, void* a10, void* a11) -> void* {
    auto control = (ControlKey*)(((uintptr_t)a2 + 0x20));
    control->Sprinting = true;
    auto original = (LockControlInputCall2)info.Trampoline;
    return original(thi, a2,a3,a4,a5,a6,a7,a8,a9,a10,a11);
}

static auto start(HMODULE hModule) -> void {
    // 拿到要Hook的关键函数的指针
    //ptr = findSig("0F B6 ? 88 ? 0F B6 42 01 88 41 01 0F"); //new
    ptr = findSig("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 ? ? 49"); //old
    _ASSERT(ptr);

    // 创建&开启Hook
    info = CreateHook((void*)ptr, (void*)&LockControlInputCallBack2);
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


// 自动发布：
// 将主要功能写完后
// 在ReleaseBody.md中修改更新信息
// 将所有代码GitHub提交完成
// 进入 git更改  查看所有提交
// 右键最近的提交，点击新建标记
// 然后分别输入标签名(版本号), 内容，内容无所谓
// 点击创建
// 进入Git更改， 点击右上角三个点,点击将所有标记（--tags）推送到
// 点击origin， 等待推送成功即可