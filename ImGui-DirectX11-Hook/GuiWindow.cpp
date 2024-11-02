#include "GuiWindow.h"

GuiWindow::GuiWindow()
{
    // Initialize settings
    this->hWnd = nullptr;
    this->hModule = nullptr;
    this->hProcess = nullptr;
    this->lpModuleAddress = nullptr;
    this->initialPostion = ImVec2(0.0f, 0.0f);
    this->uiStatus = GuiState::Reset;
    this->showMenu = true;

    // Set font path
    this->fontPath = new char[MAX_PATH] {};
    ::GetEnvironmentVariable("WINDIR", this->fontPath, MAX_PATH);
    ::strcat_s(this->fontPath, MAX_PATH, "\\Fonts\\segoeui.ttf");

    // Set window title
    std::string strText{};
    strText = strText.append(WINDOWNAME).append(" v") +
        std::to_string(MAJORVERSION) +
        std::string().append(".") +
        std::to_string(MINORVERSION) +
        std::string().append(".") +
        std::to_string(REVISIONVERSION);
    size_t nLength = strText.length() + 1;
    this->windowTitle = new char[nLength] {};
    ::memcpy(this->windowTitle, strText.c_str(), strText.length());

    // Allocate memory
    this->lpBuffer = (LPBYTE)::VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    ::memset(this->lpBuffer, 0xCC, 0x1000);
}

GuiWindow::~GuiWindow()
{
    if (this->lpBuffer)
        ::VirtualFree(this->lpBuffer, 0, MEM_RELEASE);

    delete[] this->fontPath;
    delete[] this->windowTitle;
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

void GuiWindow::Init()
{
    do
    {
        ::EnumWindows((WNDENUMPROC)EnumHwndCallback, (LPARAM)&this->hWnd);
        Sleep(200);
    } while (!this->hWnd);

    this->hProcess = ::GetCurrentProcess();
    this->hModule = ::GetModuleHandle(MODULENAME);
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
    ImGui::Begin(WINDOWNAME, nullptr, windowflags);
    if (this->uiStatus & GuiState::Reset)
    {
        ImGui::SetWindowPos(this->initialPostion);
        ImGui::SetWindowSize(ImVec2(WIDTH, HEIGHT));
        this->uiStatus &= ~GuiState::Reset;
    }

    ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
    ImVec2 windowPostion = ImGui::GetWindowPos();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::CloseButton(0x1000, ImVec2(windowPostion.x + WIDTH - FONTSIZE, windowPostion.y)))
        this->uiStatus |= GuiState::Exiting;
    ImGui::PopStyleVar();

    ImGui::Text(this->windowTitle);

    const std::string authorInfo = std::string(AUTHORINFO) + " " + std::string(BUILDDATE);
    ImVec2 textSize = ImGui::CalcTextSize(authorInfo.c_str());
    ImGui::SetCursorPosY(HEIGHT - windowPadding.y - textSize.y);
    ImGui::Text(authorInfo.c_str());

    const std::string hotKey = std::string("INSERT to Show/Hide UI");
    textSize = ImGui::CalcTextSize(hotKey.c_str());
    ImGui::SetCursorPos(ImVec2(WIDTH - windowPadding.x - textSize.x, HEIGHT - windowPadding.y - textSize.y));
    ImGui::Text(hotKey.c_str());

    if (this->uiStatus & GuiState::Exiting)
        this->ButtonExit();

    ImGui::End();
}

void GuiWindow::ButtonExit()
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChildFrame(0x2000, ImVec2(WIDTH, HEIGHT));
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChild("Exiting", ImVec2(WIDTH, HEIGHT));

    std::string strText = std::string("Do you want to exit this program?");
    ImVec2 textSize = ImGui::CalcTextSize(strText.c_str());
    ImGui::SetCursorPos(ImVec2((WIDTH - textSize.x) * 0.5f, HEIGHT * 0.382f - textSize.y * 0.5f));
    ImGui::Text(strText.c_str());

    ImGui::SetCursorPos(ImVec2(WIDTH * 0.5f - 120, HEIGHT * 0.618f));
    if (ImGui::Button("Confirm", ImVec2(100.0f, 50.0f)))
        this->uiStatus |= GuiState::Detach;

    ImGui::SetCursorPos(ImVec2(WIDTH * 0.5f + 20, HEIGHT * 0.618f));
    if (ImGui::Button("Cancel", ImVec2(100.0f, 50.0f)))
        this->uiStatus &= ~GuiState::Exiting;

    ImGui::EndChild();
    ImGui::EndChildFrame();
    ImGui::PopStyleColor(2);
}