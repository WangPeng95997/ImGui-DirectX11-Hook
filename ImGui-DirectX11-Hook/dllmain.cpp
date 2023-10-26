#include <d3d11.h>
#include "mainwindow.h"
#include "MinHook.h"
#pragma comment(lib, "d3d11.lib")

typedef HRESULT(WINAPI* Present)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(WINAPI* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
HRESULT WINAPI HK_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT WINAPI HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
LRESULT WINAPI HK_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Present Original_Present;
ResizeBuffers Original_ResizeBuffers;
WNDPROC Original_WndProc;
HWND g_mainWindow;
HMODULE g_hHinstance;
HANDLE g_hEndEvent;
DWORD64* g_methodsTable;
GuiWindow* g_GuiWindow;
ID3D11Device* g_dx11Device;
ID3D11DeviceContext* g_dx11Context;
ID3D11RenderTargetView* g_dx11RenderTargetView;
bool g_ImGuiInit = false;

void InitHook()
{
    MH_Initialize();

    // Present
    int index = 8;
    void* pTarget = (void*)g_methodsTable[index];
    MH_CreateHook(pTarget, HK_Present, (void**)&Original_Present);
    MH_EnableHook(pTarget);

    // ResizeBuffers
    index = 13;
    pTarget = (void*)g_methodsTable[index];
    MH_CreateHook(pTarget, HK_ResizeBuffers, (void**)&Original_ResizeBuffers);
    MH_EnableHook(pTarget);
}

void ReleaseHook()
{
    MH_DisableHook(MH_ALL_HOOKS);
    SetWindowLongPtr(g_mainWindow, GWLP_WNDPROC, (LONG_PTR)Original_WndProc);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    g_dx11Device->Release();
    g_dx11Context->Release();
    g_dx11RenderTargetView->Release();
    ::free(g_methodsTable);
    g_methodsTable = nullptr;

    SetEvent(g_hEndEvent);
}

LRESULT WINAPI HK_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_M)
            g_GuiWindow->showMenu = !g_GuiWindow->showMenu;
        break;

    case WM_DESTROY:
        ReleaseHook();
        break;
    }

    if (g_GuiWindow->showMenu && ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(Original_WndProc, hwnd, uMsg, wParam, lParam);
}

inline void InitImGui()
{
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
    io.Fonts->AddFontFromFileTTF(g_GuiWindow->fontPath, 20.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
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

    ImGui_ImplWin32_Init(g_mainWindow);
    ImGui_ImplDX11_Init(g_dx11Device, g_dx11Context);
    Original_WndProc = (WNDPROC)SetWindowLongPtr(g_mainWindow, GWLP_WNDPROC, (LONG_PTR)HK_WndProc);

    g_ImGuiInit = true;
}

HRESULT __fastcall HK_ResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    g_dx11Context->OMSetRenderTargets(0, nullptr, nullptr);
    g_dx11RenderTargetView->Release();
    HRESULT hResult = Original_ResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_dx11Device->GetImmediateContext(&g_dx11Context);
    g_dx11Device->CreateRenderTargetView(pBackBuffer, NULL, &g_dx11RenderTargetView);
    g_dx11Device->Release();
    pBackBuffer->Release();
    g_dx11Context->OMSetRenderTargets(1, &g_dx11RenderTargetView, nullptr);

    D3D11_VIEWPORT viewPort;
    viewPort.Width = (float)Width;
    viewPort.Height = (float)Height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;
    g_dx11Context->RSSetViewports(1, &viewPort);

    return hResult;
}

HRESULT __fastcall HK_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!g_ImGuiInit)
    {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_dx11Device)))
        {
            DXGI_SWAP_CHAIN_DESC swapChainDesc;
            pSwapChain->GetDesc(&swapChainDesc);
            g_mainWindow = swapChainDesc.OutputWindow;

            ID3D11Texture2D* pBackBuffer;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            g_dx11Device->GetImmediateContext(&g_dx11Context);
            g_dx11Device->CreateRenderTargetView(pBackBuffer, NULL, &g_dx11RenderTargetView);
            pBackBuffer->Release();

            InitImGui();
        }
        else
            return Original_Present(pSwapChain, SyncInterval, Flags);
    }
    else if (g_GuiWindow->windowStatus & WindowStatus::Exit)
    {
        ReleaseHook();
        return Original_Present(pSwapChain, SyncInterval, Flags);
    }
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    //ImGui::ShowDemoWindow();
    g_GuiWindow->Update();

    ImGui::EndFrame();
    ImGui::Render();
    g_dx11Context->OMSetRenderTargets(1, &g_dx11RenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return Original_Present(pSwapChain, SyncInterval, Flags);
}

DWORD WINAPI Start(LPVOID lpParameter)
{
    g_hHinstance = (HMODULE)lpParameter;
    g_hEndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_GuiWindow = new GuiWindow();
    g_GuiWindow->Init();

    WNDCLASSEX windowClass;
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DefWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = "DirectX11";
    windowClass.hIconSm = NULL;

    ::RegisterClassEx(&windowClass);
    HWND hwnd = ::CreateWindow(windowClass.lpszClassName, "DirectX11Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

    HMODULE d3d11Module = ::GetModuleHandleA("d3d11.dll");
    if (d3d11Module)
    {
        LPVOID D3D11CreateDeviceAndSwapChain = ::GetProcAddress(d3d11Module, "D3D11CreateDeviceAndSwapChain");
        if (D3D11CreateDeviceAndSwapChain)
        {
            D3D_FEATURE_LEVEL featureLevel;
            D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };

            DXGI_SWAP_CHAIN_DESC swapChainDesc;
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
            swapChainDesc.OutputWindow = hwnd;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapChainDesc.Windowed = 1;

            IDXGISwapChain* swapChain;
            ID3D11Device* d3d11Device;
            ID3D11DeviceContext* d3d11Context;
            if (!((long(__stdcall*)(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**))
                (D3D11CreateDeviceAndSwapChain))(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &swapChain, &d3d11Device, &featureLevel, &d3d11Context))
            {
                g_methodsTable = (DWORD64*)::calloc(205, sizeof(DWORD64));
                if (g_methodsTable)
                {
                    ::memcpy(g_methodsTable, *(DWORD64**)swapChain, 18 * sizeof(DWORD64));
                    ::memcpy(g_methodsTable + 18, *(DWORD64**)d3d11Device, 43 * sizeof(DWORD64));
                    ::memcpy(g_methodsTable + 18 + 43, *(DWORD64**)d3d11Context, 144 * sizeof(DWORD64));

                    InitHook();

                    swapChain->Release();
                    d3d11Device->Release();
                    d3d11Context->Release();
                }
            }
        }
    }
    ::DestroyWindow(hwnd);
    ::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

    ::WaitForSingleObject(g_hEndEvent, INFINITE);
    ::FreeLibraryAndExitThread(g_hHinstance, 0);

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
        MH_Uninitialize();
        break;
    }

    return true;
}