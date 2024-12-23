#include "GuiWindow.h"
#include "MinHook/include/MinHook.h"
#include <d3d11.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(WINAPI* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(WINAPI* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
HRESULT WINAPI HK_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT WINAPI HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
LRESULT WINAPI HK_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Present                 Original_Present;
ResizeBuffers           Original_ResizeBuffers;
WNDPROC                 Original_WndProc;
HMODULE                 g_hInstance;
HANDLE                  g_hEndEvent;
ULONG_PTR*              g_lpVTable;
GuiWindow*              g_GuiWindow;
ID3D11Device*           g_pDx11Device;
ID3D11DeviceContext*    g_pDx11Context;
ID3D11RenderTargetView* g_pDx11RenderTargetView;

void InitHook()
{
    MH_Initialize();

    // Present
    LPVOID lpTarget = (LPVOID)g_lpVTable[8];
    MH_CreateHook(lpTarget, HK_Present, (LPVOID*)&Original_Present);
    MH_EnableHook(lpTarget);

    // ResizeBuffers
    lpTarget = (LPVOID)g_lpVTable[13];
    MH_CreateHook(lpTarget, HK_ResizeBuffers, (LPVOID*)&Original_ResizeBuffers);
    MH_EnableHook(lpTarget);
}

void ReleaseHook()
{
    ::SetWindowLongPtr(g_GuiWindow->hWnd, GWLP_WNDPROC, (LONG_PTR)Original_WndProc);
    MH_DisableHook(MH_ALL_HOOKS);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_pDx11Device->Release();
    g_pDx11Context->Release();
    g_pDx11RenderTargetView->Release();

    ::free(g_lpVTable);
    g_lpVTable = nullptr;

    ::SetEvent(g_hEndEvent);
}

inline void InitImGui()
{
    ImGui::CreateContext();

    ImFontConfig fontConfig{};
    fontConfig.GlyphOffset.y = -1.75f;

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(g_GuiWindow->fontPath.c_str(), FONT_SIZE, &fontConfig);
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowBorderSize = 0.0f;
    style.WindowRounding = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 5.0f;
    style.TabRounding = 4.0f;
    style.WindowPadding = ImVec2(10.0f, 5.0f);
    style.FramePadding = ImVec2(0.0f, 0.0f);
    style.ItemSpacing = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = 25.0f;
    style.ScrollbarSize = 0.0f;
    style.GrabMinSize = 10.0f;
    style.ButtonTextAlign = ImVec2(0.5f, 0.50f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.12f, 0.14f, 0.65f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.28f, 0.56f, 1.00f, 0.78f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.56f, 1.00f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.28f, 0.56f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.28f, 0.56f, 1.00f, 0.95f);
    colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.56f, 1.00f, 0.80f);
    colors[ImGuiCol_TabActive] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    ImGui_ImplWin32_Init(g_GuiWindow->hWnd);
    ImGui_ImplDX11_Init(g_pDx11Device, g_pDx11Context);
    Original_WndProc = (WNDPROC)::SetWindowLongPtr(g_GuiWindow->hWnd, GWLP_WNDPROC, (LONG_PTR)HK_WndProc);
}

LRESULT WINAPI HK_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_INSERT)
            g_GuiWindow->showMenu = !g_GuiWindow->showMenu;
        break;

    case WM_DESTROY:
        ReleaseHook();
        break;
    }

    if (g_GuiWindow->showMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return ::CallWindowProc(Original_WndProc, hWnd, uMsg, wParam, lParam);
}

HRESULT WINAPI HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    g_pDx11Context->OMSetRenderTargets(0, nullptr, nullptr);
    g_pDx11RenderTargetView->Release();
    HRESULT hResult = Original_ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (!pBackBuffer)
        return hResult;

    g_pDx11Device->GetImmediateContext(&g_pDx11Context);
    g_pDx11Device->CreateRenderTargetView(pBackBuffer, NULL, &g_pDx11RenderTargetView);
    g_pDx11Device->Release();
    pBackBuffer->Release();
    g_pDx11Context->OMSetRenderTargets(1, &g_pDx11RenderTargetView, nullptr);

    D3D11_VIEWPORT viewPort{};
    viewPort.Width = (float)Width;
    viewPort.Height = (float)Height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;
    g_pDx11Context->RSSetViewports(1, &viewPort);

    return hResult;
}

HRESULT WINAPI HK_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    static bool imGuiInitialized = false;

    if (!imGuiInitialized)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDx11Device)))
        {
            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            if (pBackBuffer) {
                g_pDx11Device->GetImmediateContext(&g_pDx11Context);
                g_pDx11Device->CreateRenderTargetView(pBackBuffer, NULL, &g_pDx11RenderTargetView);
                pBackBuffer->Release();

                InitImGui();
                imGuiInitialized = true;
            }
        }
        else
            return Original_Present(pSwapChain, SyncInterval, Flags);
    }
    else if (g_GuiWindow->uiStatus & static_cast<DWORD>(GuiWindow::GuiState::GuiState_Detach))
    {
        ReleaseHook();
        return Original_Present(pSwapChain, SyncInterval, Flags);
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_GuiWindow->showMenu) {
        ImGui::ShowDemoWindow();
        //g_GuiWindow->Update();
    }

    ImGui::EndFrame();
    ImGui::Render();
    g_pDx11Context->OMSetRenderTargets(1, &g_pDx11RenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return Original_Present(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI ThreadEntry(LPVOID lpParameter)
{
    g_hInstance = (HMODULE)lpParameter;
    g_hEndEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    g_GuiWindow = new GuiWindow();
    g_GuiWindow->Initialize();

    WNDCLASSEX windowClass{};
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = ::DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = ::GetModuleHandle(NULL);
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = "Dear ImGui DirectX11";
    windowClass.hIconSm = NULL;

    ::RegisterClassEx(&windowClass);
    HWND hWnd = ::CreateWindow(
        windowClass.lpszClassName,
        windowClass.lpszClassName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        windowClass.hInstance,
        NULL);

    LPVOID D3D11CreateDeviceAndSwapChain = ::GetProcAddress(::GetModuleHandle("d3d11.dll"), "D3D11CreateDeviceAndSwapChain");
    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Width = 100;
    swapChainDesc.BufferDesc.Height = 100;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Windowed = 1;

    IDXGISwapChain* pSwapChain;
    ID3D11Device* pD3D11Device;
    ID3D11DeviceContext* pD3D11Context;
    if (!((DWORD(WINAPI*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**))
        (D3D11CreateDeviceAndSwapChain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &pD3D11Device, &featureLevel, &pD3D11Context))
    {
        g_lpVTable = (ULONG_PTR*)::calloc(205, sizeof(ULONG_PTR));
        if (g_lpVTable)
        {
            ::memcpy(g_lpVTable, *(ULONG_PTR**)pSwapChain, 18 * sizeof(ULONG_PTR));
            ::memcpy((ULONG_PTR*)g_lpVTable + 18, *(ULONG_PTR**)pD3D11Device, 43 * sizeof(ULONG_PTR));
            ::memcpy((ULONG_PTR*)g_lpVTable + 18 + 43, *(ULONG_PTR**)pD3D11Context, 144 * sizeof(ULONG_PTR));
            pSwapChain->Release();
            pD3D11Device->Release();
            pD3D11Context->Release();

            InitHook();
        }
    }
    ::DestroyWindow(hWnd);
    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

    if (g_hEndEvent)
        ::WaitForSingleObject(g_hEndEvent, INFINITE);
    delete g_GuiWindow;
    ::FreeLibraryAndExitThread(g_hInstance, EXIT_SUCCESS);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (::GetModuleHandle("d3d11.dll") == NULL)
            return false;

        ::DisableThreadLibraryCalls(hModule);
        ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadEntry, hModule, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        ::Sleep(100);
        MH_Uninitialize();
        break;
    }

    return true;
}