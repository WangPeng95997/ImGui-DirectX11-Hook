#include "GuiWindow.h"

GuiWindow::GuiWindow()
{
    // Initialize settings
    this->hWnd = nullptr;
    this->hModule = nullptr;
    this->hProcess = nullptr;
    this->lpModuleAddress = nullptr;
    this->initialPosition = ImVec2(0.0f, 0.0f);
    this->uiStatus = static_cast<DWORD>(GuiState::GuiState_Reset);
    this->showMenu = true;

    // Set font path
    LPSTR lpBuffer = new char[MAX_PATH] {};
    ::GetEnvironmentVariable("WINDIR", lpBuffer, MAX_PATH);
    ::strcat_s(lpBuffer, MAX_PATH, "\\Fonts\\segoeui.ttf");
    this->fontPath = lpBuffer;
    delete[] lpBuffer;

    // Set window title
    this->windowTitle = this->windowTitle.append(WINDOW_NAME).append(" v") +
        std::to_string(MAJOR_VERSION) + "." +
        std::to_string(MINOR_VERSION) + "." +
        std::to_string(REVISION_VERSION);

    // Allocate memory
    this->lpBuffer = (LPBYTE)::VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (this->lpBuffer)
        ::memset(this->lpBuffer, 0xCC, 0x1000);
}

GuiWindow::~GuiWindow()
{
    ::VirtualFree(this->lpBuffer, 0, MEM_RELEASE);
}

static bool CALLBACK EnumHwndCallback(HWND hWnd, LPARAM lParam)
{
    const auto isMainWindow = [hWnd]() {
        return GetWindow(hWnd, GW_OWNER) == nullptr && IsWindowVisible(hWnd);
        };

    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    if (GetCurrentProcessId() != dwProcessId || !isMainWindow() || hWnd == GetConsoleWindow())
        return true;

    *(HWND*)lParam = hWnd;

    return false;
}

void GuiWindow::Initialize()
{
    do
    {
        ::EnumWindows((WNDENUMPROC)EnumHwndCallback, (LPARAM)&this->hWnd);
        Sleep(200);
    } while (!this->hWnd);

    this->hProcess = ::GetCurrentProcess();
    this->hModule = ::GetModuleHandle(MODULE_NAME);
    this->lpModuleAddress = (LPBYTE)this->hModule;
}

void GuiWindow::Update()
{
    const ImGuiWindowFlags windowflags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin(WINDOW_NAME, nullptr, windowflags);

    if (this->uiStatus & static_cast<DWORD>(GuiState::GuiState_Reset))
    {
        ImGui::SetWindowPos(this->initialPosition);
        ImGui::SetWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));
        this->uiStatus &= ~static_cast<DWORD>(GuiState::GuiState_Reset);
    }

    ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
    ImVec2 windowPostion = ImGui::GetWindowPos();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::CloseButton(0x1000, ImVec2(windowPostion.x + WINDOW_WIDTH - FONT_SIZE, windowPostion.y)))
        this->uiStatus |= static_cast<DWORD>(GuiState::GuiState_Exiting);
    ImGui::PopStyleVar();

    ImGui::Text(windowTitle.c_str());

    const std::string authorInfo = std::string(AUTHOR_INFO) + std::string(BUILD_DATE);
    ImVec2 textSize = ImGui::CalcTextSize(authorInfo.c_str());
    ImGui::SetCursorPosY(WINDOW_HEIGHT - windowPadding.y - textSize.y);
    ImGui::Text(authorInfo.c_str());

    const std::string hotKey = std::string("Press INSERT to Show/Hide Menu");
    textSize = ImGui::CalcTextSize(hotKey.c_str());
    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH - windowPadding.x - textSize.x, WINDOW_HEIGHT - windowPadding.y - textSize.y));
    ImGui::Text(hotKey.c_str());

    if (this->uiStatus & static_cast<DWORD>(GuiState::GuiState_Exiting))
        this->ExitButton();

    ImGui::End();
}

void GuiWindow::ExitButton()
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChildFrame(0x2000, ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChild("Exiting", ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT));

    std::string strText = std::string("Do you want to exit this program?");
    ImVec2 textSize = ImGui::CalcTextSize(strText.c_str());
    ImGui::SetCursorPos(ImVec2((WINDOW_WIDTH - textSize.x) * 0.5f, WINDOW_HEIGHT * 0.382f - textSize.y * 0.5f));
    ImGui::Text(strText.c_str());

    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH * 0.5f - 120, WINDOW_HEIGHT * 0.618f));
    if (ImGui::Button("Confirm", ImVec2(100.0f, 50.0f)))
        this->uiStatus |= static_cast<DWORD>(GuiState::GuiState_Detach);

    ImGui::SetCursorPos(ImVec2(WINDOW_WIDTH * 0.5f + 20, WINDOW_HEIGHT * 0.618f));
    if (ImGui::Button("Cancel", ImVec2(100.0f, 50.0f)))
        this->uiStatus &= ~static_cast<DWORD>(GuiState::GuiState_Exiting);

    ImGui::EndChild();
    ImGui::EndChildFrame();
    ImGui::PopStyleColor(2);
}