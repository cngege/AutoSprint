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

struct vec2 {
    float x;
    float y;
};

struct ControlKey
{
    bool Sneak;							// 潜行
    BYTE __unknowkey1;
    bool ProhibitionOfFloating;			// 禁止上浮(在水中无法上浮和创造模式自动下沉)
    BYTE __unknowkey3;
    BYTE __unknowkey4;
    BYTE __unknowkey5;
    BYTE __unknowkey6;
    bool Jump;                          // 跳跃键
    bool Sprinting;                     // 疾跑键
    bool WA;							// 向左前移动
    bool WD;							// 向右前移动
    bool SA;							// 向左后移动
    bool SD;							// 向右后移动
    bool W;
    bool S;
    bool A;
    bool D;
    BYTE __unknowkey7[2];
    BYTE __unknowkey8[16];
    BYTE __unknowkey9[14];
    vec2 NormalizedAxisValue;           // 左右和前后的轴值 -1 到 1 之间 左和前表示 1
    BYTE __unknowkey10[8];
    /**
     * @brief x: 玩家视角与水平方向夹角（上90下-90） y: 玩家视角与北南向量夹角 正东-90, 正西+90, 正南0
     */
    vec2 ViewDirectionAngles;
    const float HorizontalRocker;		    // [未使用]水平的值（左右 左1  右-1  好像是只读
    const float VerticalRocker;				// [未使用]垂直的值 (前后 前1  后-1  好像是只读
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
static int offset1;
static int offset2;
static uintptr_t ptr;

static bool debugConsole = false;

template<typename T>
T toType(void* a, T b) {
    return (T)a;
}

// Hook 后的关键函数
auto LockControlInputCallBack(void* thi, ControlKey* a2) -> void*
{
    //auto control = (ControlKey*)(((uintptr_t)a2 + offset));
    a2->Sprinting = true;
    if(debugConsole) {
        static bool trigger = false;
        if(!trigger) {
            trigger = true;
            printf_s("this: 0x%llx, a2: 0x%llx\n", \
                     (uintptr_t)thi, (uintptr_t)a2
            );
        }
    }
    auto original = toType(info.Trampoline, LockControlInputCallBack);
    return original(thi, a2);
}

auto LockControlInputCallBack2(void* thi, ControlKey* a2, void* a3, void* a4, void* a5, void* a6, void* a7, void* a8, void* a9, void* a10, void* a11) -> void* {
    auto control = (ControlKey*)(((uintptr_t)a2 + offset2));
    control->Sprinting = true;
    
    if(debugConsole){
        static bool trigger = false;
        if(!trigger) {
            trigger = true;
            printf_s("this: 0x%llx, a2: 0x%llx, a3: 0x%llx, a4: 0x%llx, a5: 0x%llx, a6: 0x%llx, a7: 0x%llx, a8: 0x%llx, a9: 0x%llx, a10: 0x%llx, a11: 0x%llx\n", \
                     (uintptr_t)thi, (uintptr_t)a2, (uintptr_t)a3, (uintptr_t)a4, (uintptr_t)a5, (uintptr_t)a6, (uintptr_t)a7, (uintptr_t)a8, (uintptr_t)a9, (uintptr_t)a10, (uintptr_t)a11
            );
        }
    }
    auto original = toType(info.Trampoline, LockControlInputCallBack2);
    return original(thi, a2,a3,a4,a5,a6,a7,a8,a9,a10,a11);
}

static auto start(HMODULE hModule) -> void {
    if(debugConsole){
        //显示控制台
        HWND consoleHwnd = GetConsoleWindow();
        if(!consoleHwnd) {
            static FILE* f;
            AllocConsole();
            freopen_s(&f, "CONOUT$", "w", stdout);
            freopen_s(&f, "CONIN$", "r", stdin);
            SetConsoleTitle("Debug Console");
        }
        else {
            ShowWindow(GetConsoleWindow(), SW_SHOW);
        }
    }


    // 拿到要Hook的关键函数的指针
    //ptr = findSig("0F B6 ? 88 ? 0F B6 42 01 88 41 01 0F"); //new
    ptr = findSig("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F B6 ? ? 49"); //old
    if(ptr) {
        offset2 = *reinterpret_cast<byte*>(ptr + 18); // 0f b6 ? 28
        info = CreateHook((void*)ptr, (void*)&LockControlInputCallBack2);
        status = EnableHook(&info);

        if(debugConsole) std::cout << "[old] 特征码找到 已创建hook " << ((status) ? "开启Hook成功" : "开启Hook失败") << std::endl;
    }
    else {
        ptr = findSig("0F B6 ? 88 ? 0F B6 42 01 88 41 01 0F"); //new
        if(ptr) {
            info = CreateHook((void*)ptr, (void*)&LockControlInputCallBack);
            status = EnableHook(&info);
            if(debugConsole) std::cout << "[new] 特征码找到 已创建hook " << ((status) ? "开启Hook成功" : "开启Hook失败") << std::endl;
        }
        if(debugConsole) std::cout << "[all] 特征码查找失败 " << std::endl;
    }

    if(!debugConsole) _ASSERT(ptr);
}

void exit(HMODULE hModule) {
    if(debugConsole) {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
        system("cls");
    }
    if(status) {
        // 反注入则关闭Hook
        DisableHook(&info);
    }
}

// Dll入口函数
auto APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) -> BOOL {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)start, hModule, NULL, nullptr);
    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        exit(hModule);
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