#pragma once
#include "ImGui/imgui.h"
#include "Imgui/imgui_impl_dx11.h"
#include "Imgui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"
#include <Windows.h>
#include <sstream>
#include <string>

#define BUILDDATE []{ \
    const char* date = __DATE__; \
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec"; \
    char month[4] = {date[0], date[1], date[2], '\0'}; \
    size_t monthIndex = (strstr(months, month) - months) / 3 + 1; \
    std::ostringstream oss; \
    oss << date + 7 << "-" << (monthIndex < 10 ? "0" : "") << monthIndex << "-" << (date[4] == ' ' ? '0' : date[4]) << date[5]; \
    static std::string buildDate = oss.str(); \
    return buildDate.c_str(); \
}()

#define AUTHORINFO          "Created by l4kkS41 On"
#define WINDOWNAME          "Dear ImGui"
#define MAJORVERSION        1
#define MINORVERSION        0
#define REVISIONVERSION     0

#define FONTSIZE            20.0f
#define WIDTH               600
#define HEIGHT              400

#define MODULENAME          NULL

class GuiWindow
{
public:
    enum GuiState : DWORD
    {
        None = 0,
        Reset = 1,
        Exiting = 2,
        Detach = 4
    };

    HWND        hWnd;
    HMODULE     hModule;
    HANDLE      hProcess;
    PCHAR       fontPath;
    PCHAR       windowTitle;
    LPBYTE      lpBuffer;
    LPBYTE      lpModuleAddress;
    ImVec2      initialPostion;
    DWORD       uiStatus;
    bool        showMenu;

    GuiWindow();
    ~GuiWindow();

    void Init();
    void Update();

    void ButtonExit();
};