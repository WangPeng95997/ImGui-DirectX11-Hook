#pragma once
#include <Windows.h>
#include <string>
#include "ImGui/imgui.h"
#include "Imgui/imgui_impl_dx11.h"
#include "Imgui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"

#define AUTHORINFO          "Build.20xx.xx.xx\nby l4kkS41"	

#define WINDOWNAME          "ImGui Window"
#define MAJORVERSION        1
#define MINORVERSION        0
#define REVISIONVERSION     0

#define WIDTH               600
#define HEIGHT              400

#define TARGETCLASS         "gfx_test"
#define TARGETWINDOW        "Renderer: [DirectX11], Input: [Window Messages], 64 bits"
#define TARGETMODULE        "GFXTest64.exe"

typedef unsigned __int64 QWORD;

class GuiWindow
{
public:
    enum GuiStatus : DWORD
    {
        Reset = 1 << 0,
        Exit = 1 << 1,
        Detach = 1 << 2
    };

    HWND        hWnd;
    HMODULE     hModule;
    HANDLE      hProcess;
    PCHAR       FontPath;
    PCHAR       WindowName;
    LPBYTE      ModuleAddress;
    LPBYTE      lpBuffer;
    ImVec2      StartPostion;
    DWORD       UIStatus;
    bool        bCrosshair;
    bool        bShowMenu;

    GuiWindow();
    ~GuiWindow();

    void Init();
    void Update();

    void Button_Exit();

    void Toggle_CrossHair(const bool& isEnable);
};