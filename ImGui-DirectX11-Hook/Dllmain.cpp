#include <d3d11.h>
#include "GuiWindow.h"
#include "MinHook/include/MinHook.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
typedef HRESULT(WINAPI* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(WINAPI* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
HRESULT WINAPI HK_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT WINAPI HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
LRESULT WINAPI HK_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Present Original_Present;
ResizeBuffers Original_ResizeBuffers;
WNDPROC Original_WndProc;
HMODULE g_hInstance;
HANDLE g_hEndEvent;
LPVOID g_lpVirtualTable;
GuiWindow* g_GuiWindow;
ID3D11Device* g_pDx11Device;
ID3D11DeviceContext* g_pDx11Context;
ID3D11RenderTargetView* g_pDx11RenderTargetView;

void InitHook()
{
    ULONG_PTR* lpVTable = (ULONG_PTR*)g_lpVirtualTable;

    MH_Initialize();

    // Present
    LPVOID lpTarget = (LPVOID)lpVTable[8];
    MH_CreateHook(lpTarget, HK_Present, (void**)&Original_Present);
    MH_EnableHook(lpTarget);

    // ResizeBuffers
    lpTarget = (LPVOID)lpVTable[13];
    MH_CreateHook(lpTarget, HK_ResizeBuffers, (void**)&Original_ResizeBuffers);
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

    ::free(g_lpVirtualTable);
    g_lpVirtualTable = nullptr;

    ::SetEvent(g_hEndEvent);
}

LRESULT WINAPI HK_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_INSERT)
            g_GuiWindow->bShowMenu = !g_GuiWindow->bShowMenu;
        break;

    case WM_DESTROY:
        ReleaseHook();
        break;
    }

    if (g_GuiWindow->bShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return ::CallWindowProc(Original_WndProc, hWnd, uMsg, wParam, lParam);
}

inline void InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    io.Fonts->AddFontFromFileTTF(g_GuiWindow->FontPath, 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;

    ImGuiStyle& Style = ImGui::GetStyle();
    Style.ButtonTextAlign.y = 0.46f;
    Style.WindowBorderSize = 0.0f;
    Style.WindowRounding = 0.0f;
    Style.WindowPadding.x = 0.0f;
    Style.WindowPadding.y = 0.0f;
    Style.FrameRounding = 0.0f;
    Style.FrameBorderSize = 0.0f;
    Style.FramePadding.x = 0.0f;
    Style.FramePadding.y = 0.0f;
    Style.ChildRounding = 0.0f;
    Style.ChildBorderSize = 0.0f;
    Style.GrabRounding = 0.0f;
    Style.GrabMinSize = 8.0f;
    Style.PopupBorderSize = 0.0f;
    Style.PopupRounding = 0.0f;
    Style.ScrollbarRounding = 0.0f;
    Style.TabBorderSize = 0.0f;
    Style.TabRounding = 0.0f;
    Style.DisplaySafeAreaPadding.x = 0.0f;
    Style.DisplaySafeAreaPadding.y = 0.0f;
    Style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    Style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    Style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    Style.Colors[ImGuiCol_FrameBg] = ImColor(0, 74, 122, 100).Value;
    Style.Colors[ImGuiCol_FrameBgHovered] = ImColor(0, 74, 122, 175).Value;
    Style.Colors[ImGuiCol_FrameBgActive] = ImColor(0, 74, 122, 255).Value;
    Style.Colors[ImGuiCol_TitleBg] = ImColor(0, 74, 122, 255).Value;
    Style.Colors[ImGuiCol_TitleBgActive] = ImColor(0, 74, 122, 255).Value;

    ImGui_ImplWin32_Init(g_GuiWindow->hWnd);
    ImGui_ImplDX11_Init(g_pDx11Device, g_pDx11Context);
    Original_WndProc = (WNDPROC)::SetWindowLongPtr(g_GuiWindow->hWnd, GWLP_WNDPROC, (LONG_PTR)HK_WndProc);
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
    static bool bImGuiInit = false;

    if (!bImGuiInit)
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
                bImGuiInit = true;
            }
        }
        else
            return Original_Present(pSwapChain, SyncInterval, Flags);
    }
    else if (g_GuiWindow->UIStatus & GuiWindow::Detach)
    {
        ReleaseHook();
        return Original_Present(pSwapChain, SyncInterval, Flags);
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();
    //g_GuiWindow->Update();

    ImGui::EndFrame();
    ImGui::Render();
    g_pDx11Context->OMSetRenderTargets(1, &g_pDx11RenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return Original_Present(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI Start(LPVOID lpParameter)
{
    g_hInstance = (HMODULE)lpParameter;
    g_hEndEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    g_GuiWindow = new GuiWindow();
    g_GuiWindow->Init();

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
    windowClass.lpszClassName = "DirectX11";
    windowClass.hIconSm = NULL;

    ::RegisterClassEx(&windowClass);
    HWND hWnd = ::CreateWindow(
        windowClass.lpszClassName,
        "DirectX11Window",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        100,
        100,
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
        g_lpVirtualTable = ::calloc(205, sizeof(ULONG_PTR));
        if (g_lpVirtualTable)
        {
            ::memcpy(g_lpVirtualTable, *(ULONG_PTR**)pSwapChain, 18 * sizeof(ULONG_PTR));
            ::memcpy((ULONG_PTR*)g_lpVirtualTable + 18, *(ULONG_PTR**)pD3D11Device, 43 * sizeof(ULONG_PTR));
            ::memcpy((ULONG_PTR*)g_lpVirtualTable + 18 + 43, *(ULONG_PTR**)pD3D11Context, 144 * sizeof(ULONG_PTR));
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
    ::FreeLibraryAndExitThread(g_hInstance, EXIT_SUCCESS);

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        if (::GetModuleHandleA("d3d11.dll") == NULL)
            return false;

        ::DisableThreadLibraryCalls(hModule);
        ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Start, hModule, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        ::Sleep(100);
        MH_Uninitialize();
        break;
    }

    return true;
}