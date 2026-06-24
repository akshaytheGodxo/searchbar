#pragma comment(lib, "dwmapi.lib")
#define STB_IMAGE_IMPLEMENTATION

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "stb/stb_image.h"
#include "FileEntry.h"
#include "image_data.h"
#include <d3d11.h>
#include <tchar.h>
#include <cmath>
#include <unordered_map>
#include <string>
#include <windowsx.h>
#include <dwmapi.h>
#include <iostream>
#include <atomic>
#include <thread>


std::atomic<bool> g_IndexLoaded = false;

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static bool g_showIndexManagerWindow = false;

ID3D11ShaderResourceView* myTexture = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void EXTRACT_ICON(std::string filepath);
void LOAD_RGBA_DATA(unsigned char* rgba_data, int width, int height);
unsigned char* CONVERT_ICON_TO_RGBA(HICON hIcon, int* width, int* height);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

FileEntry g_FileIndexer;


void EXTRACT_ICON(std::string filepath) {
    LPCSTR converted_path = filepath.c_str();
    
    SHFILEINFOA sfi = {};

    DWORD_PTR result = SHGetFileInfoA(
        converted_path,
        0,
        &sfi,
        sizeof(sfi),
        SHGFI_ICON | SHGFI_LARGEICON
    );

    if (result == 0) {
        return;
    }


    int width, height;
    unsigned char* rgba_data = CONVERT_ICON_TO_RGBA(sfi.hIcon, &width, &height);

    LOAD_RGBA_DATA(rgba_data, width, height);


}
unsigned char* CONVERT_ICON_TO_RGBA(HICON hIcon, int* width, int* height) {
    ICONINFO iconinfo;
    GetIconInfo(hIcon, &iconinfo);
    HBITMAP hBmp = (HBITMAP)CopyImage(iconinfo.hbmColor, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

    if (!hBmp) {
        if (iconinfo.hbmColor) DeleteObject(iconinfo.hbmColor);
        if (iconinfo.hbmMask) DeleteObject(iconinfo.hbmMask);
        return nullptr;
    }

    BITMAP btmp;
    GetObject(hBmp, sizeof(BITMAP), &btmp);

    *width = btmp.bmWidth;
    *height = btmp.bmHeight;
    int pixelcount = *width * *height;
    unsigned char* rgbaData = new unsigned char[pixelcount * 4];

    HDC hDC = GetDC(NULL);
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = *width;
    bmi.bmiHeader.biHeight = -*height; // Negative height for top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;


    GetDIBits(hDC, hBmp, 0, *height, rgbaData, &bmi, DIB_RGB_COLORS);

    ReleaseDC(NULL, hDC);

    for (int i = 0; i < pixelcount; ++i) {
        int index = i * 4;
        unsigned char b = rgbaData[index];
        unsigned char g = rgbaData[index + 1];
        unsigned char r = rgbaData[index + 2];
        unsigned char a = rgbaData[index + 3]; // May be 0 if the icon mask applies

        // Apply BGRA -> RGBA mapping
        rgbaData[index] = r;
        rgbaData[index + 1] = g;
        rgbaData[index + 2] = b;
        rgbaData[index + 3] = a;
    }
    DeleteObject(hBmp);
    if (iconinfo.hbmColor) DeleteObject(iconinfo.hbmColor);
    if (iconinfo.hbmMask) DeleteObject(iconinfo.hbmMask);

    return rgbaData;

}

void LOAD_RGBA_DATA(unsigned char* rgba_data, int width, int height) {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA subresource = {};
    subresource.pSysMem = rgba_data;
    subresource.SysMemPitch = width * 4;

    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subresource, &pTexture);

    if (SUCCEEDED(hr)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;


        hr = g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &myTexture);
        pTexture->Release();
    }
    stbi_image_free(rgba_data);
}

void LoadDB() {
    std::thread([]()
        {
            sqlite3* db = nullptr;

            if (sqlite3_open("index.db", &db) == SQLITE_OK)
            {
                sqlite3_close(db);
                g_IndexLoaded = true;
            }
        }).detach();
}

void EnableAcrylicBlur(HWND hwnd) {
    enum ACCENT_STATE {
        ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
    };
    struct ACCENT_POLICY {
        int AccentState;
        DWORD AccentFlags;
        DWORD GradientColor; // AABBGGRR
        DWORD AnimationId;
    };
    struct WINDOWCOMPOSITIONATTRIBDATA {
        DWORD Attrib;
        PVOID pvData;
        SIZE_T cbData;
    };
    typedef BOOL(WINAPI* Fn)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

    auto fn = (Fn)GetProcAddress(GetModuleHandleA("user32.dll"),
        "SetWindowCompositionAttribute");
    if (!fn) return;

    ACCENT_POLICY accent = { ACCENT_ENABLE_ACRYLICBLURBEHIND, 0, 0x00000000, 0 };
    WINDOWCOMPOSITIONATTRIBDATA data = { 19, &accent, sizeof(accent) };
    fn(hwnd, &data);
}

void DrawBetterGlass(ImGuiWindow* w) {
    if (!w) return;
    ImDrawList* dl = w->DrawList;
    const ImVec2 a = w->Pos;
    const ImVec2 b = ImVec2(w->Pos.x + w->Size.x, w->Pos.y + w->Size.y);
    const float r = ImGui::GetStyle().WindowRounding;

    dl->AddRectFilled(a, b, IM_COL32(20, 22, 30, 80), r);           // dark tint
    dl->AddRectFilled(a, ImVec2(b.x, a.y + 2.0f),
        IM_COL32(255, 255, 255, 60), r);               // top sheen
    dl->AddRect(a, b, IM_COL32(255, 255, 255, 40), r);              // border
}
    
void ImGuiCreateInput() {
    


    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(
        "Better Search",
        nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse
    );
    DrawBetterGlass(ImGui::GetCurrentWindow());

    if (!g_IndexLoaded)
    {
        static const char* spinner[] =
        {
            "|",
            "/",
            "-",
            "\\"
        };

        int frame =
            (int)(ImGui::GetTime() * 8.0f) % 4;

        ImGui::SetCursorPosY(60);

        ImGui::Text(
            "Loading index %s",
            spinner[frame]
        );

        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }


    static char user_input[64] = "";
    static bool search_success = false;

    static std::vector<FileLogs> results;
    ImGui::PushItemWidth(-1);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(255, 255, 255, 10));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(255, 255, 255, 20));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(255, 255, 255, 30));
    static bool first_frame = true;

    if (first_frame)
    {
        ImGui::SetKeyboardFocusHere();
        first_frame = false;
    }

    if (ImGui::InputText(
        "##search",
        user_input,
        IM_ARRAYSIZE(user_input)))
    {
        std::string input(user_input);

        if (!input.empty())
        {

            //std::cout << "Searching for " << user_input << "\n";
            results =
                g_FileIndexer.SearchFiles(input);
        }
        else
        {
            results.clear();
        }
    }

    ImGui::PopStyleColor(3);

    ImGui::PushStyleColor(
        ImGuiCol_Separator,
        IM_COL32(255, 255, 255, 30)
    );

    ImGui::Separator();

    ImGui::PopStyleColor();

    static float results_alpha = 0.0f;

    float target = results.empty() ? 0.0f : 1.0f;

    results_alpha +=
        (target - results_alpha)
        * ImGui::GetIO().DeltaTime
        * 10.0f;

    results_alpha = ImClamp(results_alpha, 0.0f, 1.0f);

    ImGui::PushStyleVar(
        ImGuiStyleVar_Alpha,
        results_alpha
    );

    for (const auto& file : results)
    {
        ImGui::PushID(file.fullpath.c_str());

        EXTRACT_ICON(file.fullpath);

        ImGui::Image(
            (ImTextureID)myTexture,
            ImVec2(32, 32)
        );

        ImGui::SameLine();

        if (ImGui::Selectable(
            file.filename.c_str(),
            false,
            ImGuiSelectableFlags_SpanAvailWidth))
        {
            ShellExecuteA(
                nullptr,
                "open",
                file.fullpath.c_str(),
                nullptr,
                nullptr,
                SW_SHOWNORMAL
            );
        }

        ImGui::PopID();
    }
    ImGui::PopStyleVar();

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::SetCursorPos(ImVec2(windowSize.x - 30.0f, windowSize.y - 30.0f));

    if (ImGui::Button("##gear", ImVec2(22, 22)))
        g_showIndexManagerWindow = !g_showIndexManagerWindow;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 btnPos = ImGui::GetItemRectMin();
    ImVec2 btnCenter = ImVec2(btnPos.x + 11, btnPos.y + 11);
    dl->AddCircle(btnCenter, 7.0f, IM_COL32(200, 200, 200, 180), 16, 1.5f);
    dl->AddCircle(btnCenter, 3.0f, IM_COL32(200, 200, 200, 180), 8, 1.5f);



    ImGui::End();

    ImGui::PopStyleVar();
}


void ImGuiIndexManager()
{
    if (!g_showIndexManagerWindow) return;

    ImGui::SetNextWindowSize(ImVec2(360, 260), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));

    bool open = true;
    ImGui::Begin("Index Manager", &open,
        ImGuiWindowFlags_NoCollapse 
    );

    // Closing via X button
    if (!open) g_showIndexManagerWindow = false;

    DrawBetterGlass(ImGui::GetCurrentWindow());

    // --- Status row ---
    ImGui::TextDisabled("Status:");
    ImGui::SameLine();
    if (g_IndexLoaded)
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.5f, 1.0f), "Index loaded");
    else
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Loading...");

    ImGui::Separator();

    // --- File count (query from DB) ---
    static int  s_FileCount = -1;
    static bool s_CountFetched = false;

    if (!s_CountFetched && g_IndexLoaded)
    {
        std::thread([]() {
            sqlite3* db = nullptr;
            if (sqlite3_open("index.db", &db) == SQLITE_OK)
            {
                sqlite3_stmt* stmt = nullptr;
                if (sqlite3_prepare_v2(db,
                    "SELECT COUNT(*) FROM files;", -1, &stmt, nullptr) == SQLITE_OK)
                {
                    if (sqlite3_step(stmt) == SQLITE_ROW)
                        s_FileCount = sqlite3_column_int(stmt, 0);
                    sqlite3_finalize(stmt);
                }
                sqlite3_close(db);
            }
            s_CountFetched = true;
            }).detach();
    }

    if (s_CountFetched)
    {
        ImGui::Text("Indexed files: %d", s_FileCount);
    }
    else
    {
        ImGui::TextDisabled("Indexed files: ...");
    }

    ImGui::Spacing();

    // --- Rebuild button ---
    static bool s_Rebuilding = false;

    if (s_Rebuilding)
    {
        ImGui::BeginDisabled();
        ImGui::Button("Rebuilding...", ImVec2(-1, 0));
        ImGui::EndDisabled();
    }
    else if (ImGui::Button("Rebuild Index", ImVec2(-1, 0)))
    {
        s_Rebuilding = true;
        s_CountFetched = false;
        s_FileCount = -1;
        g_IndexLoaded = false;

        std::thread([]() {
            // Drop and recreate — replace with your actual indexing logic
            sqlite3* db = nullptr;
            if (sqlite3_open("index.db", &db) == SQLITE_OK)
            {
                sqlite3_exec(db, "DROP TABLE IF EXISTS files;", nullptr, nullptr, nullptr);
                // ... your indexing code here ...
                sqlite3_close(db);
            }
            g_IndexLoaded = true;
            s_Rebuilding = false;
            }).detach();
    }

    // --- Delete DB button ---
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(120, 30, 30, 180));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(160, 40, 40, 220));

    if (ImGui::Button("Delete Index DB", ImVec2(-1, 0)))
    {
        // Confirm before deleting — optionally open a modal
        std::remove("index.db");
        g_IndexLoaded = false;
        s_CountFetched = false;
        s_FileCount = -1;
    }

    ImGui::PopStyleColor(2);

    ImGui::End();
    ImGui::PopStyleVar();
}

// Main code
int main(int, char**)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));



    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST,
        wc.lpszClassName,
        L"BetterSearch",
        WS_POPUP,
        100, 100,
        (int)(400 * main_scale), (int)(200 * main_scale),
        nullptr, nullptr, wc.hInstance, nullptr
    );
    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }


    EnableAcrylicBlur(hwnd);


    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();
    // removing the previous styling probably
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;




    // Setup scaling
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    LoadDB();
    
    // Our state

    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // image display code donot touch baby
    // 
 //   unsigned char* rgba_data = stbi_load_from_memory(image_data, sizeof(image_data), &image_width, &image_height, &channels, 4);

 //   D3D11_TEXTURE2D_DESC desc = {};
 //   desc.Width = image_width;
 //   desc.Height = image_height;
 //   desc.MipLevels = 1;
 //   desc.ArraySize = 1;
	//desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
 //   desc.SampleDesc.Count = 1;
	//desc.Usage = D3D11_USAGE_DEFAULT;
	//desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	//D3D11_SUBRESOURCE_DATA subResource = {};
 //   subResource.pSysMem = rgba_data;
 //   subResource.SysMemPitch = image_width * 4;


 //   ID3D11Texture2D* pTexture = nullptr;
 //   HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

 //   if (SUCCEEDED(hr)) {
 //       D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
 //       srvDesc.Format = desc.Format;
 //       srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
 //       srvDesc.Texture2D.MostDetailedMip = 0;
 //       srvDesc.Texture2D.MipLevels = 1;

 //       hr = g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &myTexture);
 //       pTexture->Release();
 //   }

	//stbi_image_free(rgba_data);
    // 
    
    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // frame cycles, probably don't tuch it
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        //show any window here
        ImGuiCreateInput();
        ImGuiIndexManager();
        // Loading Index asynchronously
        
        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;

    case WM_NCHITTEST:
    {
        POINT p = {
            GET_X_LPARAM(lParam),
            GET_Y_LPARAM(lParam)
        };

        ScreenToClient(hWnd, &p);

        // Drag only from top 30px
        if (p.y >= 0 && p.y <= 30)
            return HTCAPTION;

        return HTCLIENT;
    }

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
