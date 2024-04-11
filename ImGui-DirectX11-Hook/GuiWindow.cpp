#include "GuiWindow.h"

GuiWindow::GuiWindow()
{
    // �����ڴ�
    this->lpBuffer = (LPBYTE)::VirtualAlloc(NULL, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    ::memset(this->lpBuffer, 0xCC, 0x1000);

    // ������������
    this->FontPath = new char[MAX_PATH] {};
    ::GetEnvironmentVariableA("WINDIR", this->FontPath, MAX_PATH);
    ::strcat_s(this->FontPath, MAX_PATH, "\\Fonts\\msyh.ttc");

    // ��������
    std::string strText;
    strText = strText.append(WINDOWNAME).append(" v") + std::to_string(MAJORVERSION) + std::string().append(".") + std::to_string(MINORVERSION) + std::string().append(".") + std::to_string(REVISIONVERSION);
    size_t nLength = strText.length() + 1;
    this->WindowName = new char[nLength] {};
    ::memcpy(this->WindowName, strText.c_str(), strText.length());

    // ��������
    this->StartPostion = ImVec2(0.0f, 0.0f);
    this->UIStatus = GuiStatus::Reset;

    // ���ܲ˵�
    this->bCrosshair = false;
    this->bShowMenu = true;
}

GuiWindow::~GuiWindow()
{
    if (this->lpBuffer)
        ::VirtualFree(this->lpBuffer, 0, MEM_RELEASE);

    delete[] this->FontPath;
    delete[] this->WindowName;
}

void GuiWindow::Init()
{
    do
    {
        this->hWnd = FindWindow(TARGETCLASS, TARGETWINDOW);
        this->hModule = GetModuleHandle(TARGETMODULE);
        this->hProcess = ::GetCurrentProcess();
        Sleep(200);
    } while (this->hWnd == NULL || this->hModule == NULL || this->hProcess == NULL);

    this->ModuleAddress = (LPBYTE)this->hModule;
}

void GuiWindow::Update()
{
    if (this->bShowMenu)
    {
        const ImGuiWindowFlags windowflags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("ImGuiWindow", nullptr, windowflags);
        if (this->UIStatus & GuiStatus::Reset)
        {
            ImGui::SetWindowPos(this->StartPostion);
            ImGui::SetWindowSize(ImVec2(WIDTH, HEIGHT));
            this->UIStatus &= ~GuiStatus::Reset;
        }

        ImVec2 windowPostion = ImGui::GetWindowPos();
        ImGui::Text(this->WindowName);
        if (ImGui::CloseButton(0x1000, ImVec2(windowPostion.x + WIDTH - 20.0f, windowPostion.y)))
            this->UIStatus |= GuiStatus::Exit;

        if (ImGui::Checkbox(u8"����׼��", &this->bCrosshair))
            Toggle_CrossHair(this->bCrosshair);

        const std::string authorInfo = std::string(AUTHORINFO);
        ImVec2 textSize = ImGui::CalcTextSize(authorInfo.c_str());
        ImGui::SetCursorPosY(HEIGHT - textSize.y);
        ImGui::Text(authorInfo.c_str());

        const std::string hotKey = std::string(u8"INSERT��ʾ/���ؽ���");
        textSize = ImGui::CalcTextSize(hotKey.c_str());
        ImGui::SetCursorPos(ImVec2(WIDTH - textSize.x, HEIGHT - textSize.y));
        ImGui::Text(hotKey.c_str());

        if (this->UIStatus & GuiStatus::Exit)
            this->Button_Exit();

        ImGui::End();
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (this->bCrosshair)
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;

        drawList->AddLine(
            ImVec2(displaySize.x / 2 - 30.0f, displaySize.y / 2),
            ImVec2(displaySize.x / 2 + 30.0f, displaySize.y / 2),
            ImColor(0, 255, 0, 255));
        drawList->AddLine(
            ImVec2(displaySize.x / 2, displaySize.y / 2 - 30.0f),
            ImVec2(displaySize.x / 2, displaySize.y / 2 + 30.0f),
            ImColor(0, 255, 0, 255));
    }
}

void GuiWindow::Button_Exit()
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChildFrame(0x2000, ImVec2(WIDTH, HEIGHT));
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChild("ExitWindow", ImVec2(WIDTH, HEIGHT));

    std::string strText = std::string(u8"�Ƿ��˳�����");
    ImVec2 textSize = ImGui::CalcTextSize(strText.c_str());
    ImGui::SetCursorPos(ImVec2((WIDTH - textSize.x) * 0.5f, HEIGHT * 0.382f - textSize.y * 0.5f));
    ImGui::Text(strText.c_str());

    ImGui::SetCursorPos(ImVec2(WIDTH * 0.5f - 120, HEIGHT * 0.618f));
    if (ImGui::Button(u8"ȷ��", ImVec2(100.0f, 50.0f)))
        this->UIStatus |= GuiStatus::Detach;

    ImGui::SetCursorPos(ImVec2(WIDTH * 0.5f + 20, HEIGHT * 0.618f));
    if (ImGui::Button(u8"ȡ��", ImVec2(100.0f, 50.0f)))
        this->UIStatus &= ~GuiStatus::Exit;

    ImGui::EndChild();
    ImGui::EndChildFrame();
    ImGui::PopStyleColor(2);
}

void GuiWindow::Toggle_CrossHair(const bool& isEnable)
{
    this->bCrosshair = isEnable;
}