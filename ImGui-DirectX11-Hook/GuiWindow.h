#pragma once
#include <Windows.h>
#include <string>
#include "ImGui/imgui.h"
#include "Imgui/imgui_impl_dx11.h"
#include "Imgui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"

#define AUTHORINFO          "Build.20xx.xx.xx\nby l4kkS41"	

#define WINDOWNAME          "ImGui Window"
#define MAJORVERSION        0
#define MINORVERSION        1
#define REVISIONVERSION     0

#define WIDTH               600
#define HEIGHT              400

#define TARGETCLASS         "gfx_test"
#define TARGETWINDOW        "Renderer: [DirectX11], Input: [Window Messages], 64 bits"
#define TARGETMODULE        "GFXTest64.exe"

class GuiWindow
{
public:
    enum GuiStatus : DWORD
    {
        Normal = 0,
        Reset = 1 << 0,
        Exit = 1 << 1,
        Detach = 1 << 2
    };

    HANDLE      hProcess;
    HMODULE     hModule;
    HWND        hwnd;
    PCHAR       FontPath;
    PCHAR       Name;
    LPBYTE      ModuleAddress;
    LPBYTE      lpBuffer;
    ImVec2      StartPostion;
    DWORD       UIStatus;
    bool        bCrossHair;
    bool        bShowMenu;

    GuiWindow();
    ~GuiWindow();

    void Init();
    void Release();
    void Update();

    void Button_Exit();
    void ResetWindow();
    void Toggle_CrossHair(const bool& isEnable);
};