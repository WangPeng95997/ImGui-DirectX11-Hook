#pragma once
#include "ImGui/imgui.h"
#include "Imgui/imgui_impl_dx11.h"
#include "Imgui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"
#include <Windows.h>
#include <sstream>
#include <string>

#define BUILD_DATE []{ \
    const char* date = __DATE__; \
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec"; \
    char month[4] = {date[0], date[1], date[2], '\0'}; \
    size_t monthIndex = (strstr(months, month) - months) / 3 + 1; \
    std::ostringstream oss; \
    oss << date + 7 << "-" << (monthIndex < 10 ? "0" : "") << monthIndex << "-" << (date[4] == ' ' ? '0' : date[4]) << date[5]; \
    static std::string result = oss.str(); \
    return result.c_str(); \
}()

constexpr const char* AUTHOR_INFO = "Created by l4kkS41 On ";
constexpr const char* WINDOW_NAME = "Dear ImGui";
constexpr const char* MODULE_NAME = nullptr;
constexpr DWORD MAJOR_VERSION = 1;
constexpr DWORD MINOR_VERSION = 0;
constexpr DWORD REVISION_VERSION = 0;
constexpr float FONT_SIZE = 20.0f;
constexpr float WINDOW_WIDTH = 600.0f;
constexpr float WINDOW_HEIGHT = 400.0f;

class GuiWindow
{
public:
    enum class GuiState : DWORD
    {
        GuiState_None = 0,
        GuiState_Reset = 1,
        GuiState_Exiting = 2,
        GuiState_Detach = 4
    };

    HWND hWnd;
    HMODULE hModule;
    HANDLE hProcess;
    std::string fontPath;
    std::string windowTitle;
    LPBYTE lpBuffer;
    LPBYTE lpModuleAddress;
    ImVec2 initialPosition;
    DWORD uiStatus;
    bool showMenu;

    GuiWindow();
    ~GuiWindow();

    void Initialize();
    void Update();

    void ExitButton();
};