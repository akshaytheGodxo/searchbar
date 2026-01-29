#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <dwmapi.h>
#include <iostream>
#include <CommCtrl.h>
#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>
#include <gdiplus.h>
#include "Manifest.h"
#include "FileFinder.h"
#include "IndexFiles.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

#define ID_EDITBOX 1001
#define ID_CLOSE_BTN 1002
#define TIMER_FADE_IN 1
#define TIMER_FADE_OUT 2

#define FONT_PATH "E:\\Personal Projects\\myengine\\assets\\Poppins\\Poppins-Regular.ttf"

/* Enhanced UI Color Scheme - Modern Dark Theme */
constexpr COLORREF BG_COLOR = RGB(18, 18, 18);           // Darker background
constexpr COLORREF PANEL_COLOR = RGB(28, 28, 30);        // Elevated surface
constexpr COLORREF SEARCH_BOX_COLOR = RGB(38, 38, 40);   // Search input background
constexpr COLORREF TEXT_COLOR = RGB(235, 235, 245);      // High contrast text
constexpr COLORREF PLACEHOLDER_COLOR = RGB(142, 142, 147); // Subtle placeholder
constexpr COLORREF ACCENT_COLOR = RGB(10, 132, 255);     // iOS-style blue
constexpr COLORREF ACCENT_HOVER = RGB(32, 148, 250);     // Lighter blue on hover
constexpr COLORREF BORDER_COLOR = RGB(56, 56, 58);       // Subtle borders
constexpr COLORREF SELECTED_BG = RGB(48, 48, 52);        // Selected item
constexpr COLORREF SHADOW_COLOR = RGB(0, 0, 0);          // For drop shadows

const wchar_t CLASS_NAME[] = L"ModernSearchBar";
WCHAR placeholderText[] = L"Search files and folders...";
HBITMAP g_hBackground = NULL;
HWND g_hEdit = NULL;
HWND g_hResult = NULL;
HWND g_hCloseBtn = NULL;
HFONT g_hFont = NULL;
HFONT g_hFontBold = NULL;
HFONT g_hFontSmall = NULL;
IndexFiles indexer = IndexFiles();
FileFinder fileFinder;
WNDPROC g_OldListProc;
WNDPROC g_OldEditProc;
WNDPROC g_OldButtonProc;

// Animation variables
int g_iAlpha = 0;
bool g_bFadingIn = false;
bool g_bFadingOut = false;

// GDI+ token
ULONG_PTR gdiplusToken;

void StartIndexing();

void APP_PROTOCOL_HANDLER() {
    std::wcout << "Index is empty?: " << indexer._isEmpty() << "\n";
    std::wcout << "Index File exists: " << indexer.diskIndexExists() << "\n";
    if (indexer._isEmpty()) {
        indexer.openFile();
        StartIndexing();
    }
    else {
        std::wcout << L"Index loaded from disk. \n";
    }
}

void StartIndexing() {
    std::wcout << "Index was empty so building index from scratch...\n";
    std::thread([] {
        fileFinder.BuildIndexAtStartup(L"C:\\Users\\aksha\\OneDrive\\Pictures", indexer);
        fileFinder.BuildIndexAtStartup(L"C:\\Users\\aksha\\OneDrive\\Documents", indexer);
        fileFinder.BuildIndexAtStartup(L"C:\\Users\\aksha\\Downloads", indexer);
        fileFinder.BuildIndexAtStartup(L"C:\\Users\\aksha\\OneDrive\\Desktop", indexer);
        std::wcout << "Indexing is Done, you may test your input\n";
        }).detach();
}

std::wstring ParseResult(const std::wstring& path) {
    std::filesystem::path p(path);
    return p.filename().wstring();
}

void SearchFile(const std::wstring& target)
{
    SendMessageW(g_hResult, LB_RESETCONTENT, 0, 0);

    if (target.empty()) return;

    const auto& index = indexer.getIndex();

    for (const auto& i : index)
    {
        const auto file = i.first;
        const auto path = i.second;

        if (file.find(target) != std::wstring::npos)
        {
            SendMessageW(
                g_hResult,
                LB_ADDSTRING,
                0,
                (LPARAM)path.c_str()
            );
        }
    }
}

void OpenFileAtPath(const std::wstring& path) {
    ShellExecuteW(
        NULL,
        L"open",
        path.c_str(),
        NULL,
        NULL,
        SW_SHOWNORMAL
    );
}

// Custom owner-drawn listbox for better visual appearance
void DrawListBoxItem(DRAWITEMSTRUCT* dis)
{
    if (dis->itemID == -1) return;

    wchar_t buffer[MAX_PATH];
    SendMessageW(dis->hwndItem, LB_GETTEXT, dis->itemID, (LPARAM)buffer);

    std::wstring fullPath(buffer);
    std::filesystem::path p(fullPath);
    std::wstring filename = p.filename().wstring();
    std::wstring directory = p.parent_path().wstring();

    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;

    // Background
    COLORREF bgColor = (dis->itemState & ODS_SELECTED) ? SELECTED_BG : PANEL_COLOR;
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    // Add subtle left border for selected items
    if (dis->itemState & ODS_SELECTED) {
        RECT borderRect = rc;
        borderRect.right = borderRect.left + 3;
        HBRUSH accentBrush = CreateSolidBrush(ACCENT_COLOR);
        FillRect(hdc, &borderRect, accentBrush);
        DeleteObject(accentBrush);
    }

    SetBkMode(hdc, TRANSPARENT);

    // Draw filename (bold)
    RECT nameRect = rc;
    nameRect.left += 16;
    nameRect.top += 8;
    SetTextColor(hdc, TEXT_COLOR);
    SelectObject(hdc, g_hFontBold);
    DrawTextW(hdc, filename.c_str(), -1, &nameRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

    // Draw directory path (smaller, dimmed)
    RECT pathRect = rc;
    pathRect.left += 16;
    pathRect.top += 30;
    SetTextColor(hdc, PLACEHOLDER_COLOR);
    SelectObject(hdc, g_hFontSmall);
    DrawTextW(hdc, directory.c_str(), -1, &pathRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
}

LRESULT CALLBACK ListBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_RETURN)
    {
        int sel = (int)SendMessageW(hwnd, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR)
        {
            wchar_t buffer[MAX_PATH];
            SendMessageW(hwnd, LB_GETTEXT, sel, (LPARAM)buffer);
            std::wcout << L"Opening: " << buffer << L"\n";
            OpenFileAtPath(buffer);
        }
    }

    return CallWindowProc(g_OldListProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN && wParam == VK_DOWN)
    {
        int count = (int)SendMessageW(g_hResult, LB_GETCOUNT, 0, 0);
        if (count > 0)
        {
            SetFocus(g_hResult);
            SendMessageW(g_hResult, LB_SETCURSEL, 0, 0);
        }
        return 0;
    }

    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE)
    {
        SendMessageW(GetParent(hwnd), WM_CLOSE, 0, 0);
        return 0;
    }

    return CallWindowProcW(g_OldEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static bool isHovered = false;

    if (msg == WM_MOUSEMOVE && !isHovered)
    {
        isHovered = true;
        InvalidateRect(hwnd, NULL, TRUE);

        TRACKMOUSEEVENT tme = {};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
    }

    if (msg == WM_MOUSELEAVE)
    {
        isHovered = false;
        InvalidateRect(hwnd, NULL, TRUE);
    }

    return CallWindowProcW(g_OldButtonProc, hwnd, msg, wParam, lParam);
}

// Draw rounded rectangle
void DrawRoundedRect(HDC hdc, RECT rect, int radius, COLORREF color)
{
    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

    Gdiplus::SolidBrush brush(Gdiplus::Color(
        GetRValue(color),
        GetGValue(color),
        GetBValue(color)
    ));

    Gdiplus::GraphicsPath path;
    path.AddArc(rect.left, rect.top, radius, radius, 180, 90);
    path.AddArc(rect.right - radius, rect.top, radius, radius, 270, 90);
    path.AddArc(rect.right - radius, rect.bottom - radius, radius, radius, 0, 90);
    path.AddArc(rect.left, rect.bottom - radius, radius, radius, 90, 90);
    path.CloseFigure();

    graphics.FillPath(&brush, &path);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        // Enable drop shadow
        MARGINS margins = { 0, 0, 0, 1 };
        DwmExtendFrameIntoClientArea(hwnd, &margins);

        // Add fonts
        AddFontResourceExW((LPCWSTR)FONT_PATH, FR_PRIVATE, NULL);

        // Main font
        g_hFont = CreateFontW(
            20, 0, 0, 0,
            FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            VARIABLE_PITCH,
            L"Poppins"
        );

        // Bold font for results
        g_hFontBold = CreateFontW(
            16, 0, 0, 0,
            FW_SEMIBOLD,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            VARIABLE_PITCH,
            L"Poppins"
        );

        // Small font for paths
        g_hFontSmall = CreateFontW(
            13, 0, 0, 0,
            FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            VARIABLE_PITCH,
            L"Poppins"
        );

        // Search box with more padding
        g_hEdit = CreateWindowExW(
            0,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            24, 24, 702, 48,
            hwnd,
            (HMENU)ID_EDITBOX,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL
        );

        SendMessageW(g_hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
        SendMessageW(g_hEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(16, 16));
        SendMessage(g_hEdit, EM_SETCUEBANNER, 1, (LPARAM)placeholderText);

        g_OldEditProc = (WNDPROC)SetWindowLongPtrW(
            g_hEdit,
            GWLP_WNDPROC,
            (LONG_PTR)EditProc
        );

        // Results listbox - owner drawn for custom styling
        g_hResult = CreateWindowExW(
            0,
            L"LISTBOX",
            NULL,
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_OWNERDRAWFIXED | WS_VSCROLL,
            24, 88, 702, 360,
            hwnd,
            NULL,
            ((LPCREATESTRUCT)lParam)->hInstance,
            NULL
        );

        SendMessageW(g_hResult, LB_SETITEMHEIGHT, 0, 56); // Taller items for two-line display
        SendMessageW(g_hResult, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_OldListProc = (WNDPROC)SetWindowLongPtrW(
            g_hResult,
            GWLP_WNDPROC,
            (LONG_PTR)ListBoxProc
        );

        // Set initial focus
        SetFocus(g_hEdit);

        // Start fade-in animation
        g_bFadingIn = true;
        SetTimer(hwnd, TIMER_FADE_IN, 15, NULL);

        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == TIMER_FADE_IN)
        {
            if (g_iAlpha < 255)
            {
                g_iAlpha += 15;
                if (g_iAlpha > 255) g_iAlpha = 255;

                SetLayeredWindowAttributes(hwnd, 0, (BYTE)g_iAlpha, LWA_ALPHA);
            }
            else
            {
                KillTimer(hwnd, TIMER_FADE_IN);
                g_bFadingIn = false;
            }
        }
        else if (wParam == TIMER_FADE_OUT)
        {
            if (g_iAlpha > 0)
            {
                g_iAlpha -= 25;
                if (g_iAlpha < 0) g_iAlpha = 0;

                SetLayeredWindowAttributes(hwnd, 0, (BYTE)g_iAlpha, LWA_ALPHA);
            }
            else
            {
                KillTimer(hwnd, TIMER_FADE_OUT);
                g_bFadingOut = false;
                ShowWindow(hwnd, SW_HIDE);
            }
        }
        return 0;
    }

    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
        {
            ShowWindow(hwnd, SW_HIDE);
            SendMessageW(g_hEdit, WM_SETTEXT, 0, (LPARAM)L"");
            SendMessageW(g_hResult, LB_RESETCONTENT, 0, 0);
            return 0;
        }
        if (wParam == VK_DOWN)
        {
            SetFocus(g_hResult);
            SendMessageW(g_hResult, LB_SETCURSEL, 0, 0);
        }
        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_EDITBOX && HIWORD(wParam) == EN_CHANGE)
        {
            int len = GetWindowTextLengthW(g_hEdit);
            std::wstring text(len + 1, L'\0');
            GetWindowTextW(g_hEdit, &text[0], len + 1);
            text.resize(len);
            SearchFile(text);
        }
        return 0;
    }

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
        if (dis->CtlID == 0 && dis->hwndItem == g_hResult)
        {
            DrawListBoxItem(dis);
            return TRUE;
        }
        return 0;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Fill background
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        HBRUSH bgBrush = CreateSolidBrush(BG_COLOR);
        FillRect(hdc, &clientRect, bgBrush);
        DeleteObject(bgBrush);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, TEXT_COLOR);
        SetBkColor(hdc, SEARCH_BOX_COLOR);
        static HBRUSH hBrush = CreateSolidBrush(SEARCH_BOX_COLOR);
        return (INT_PTR)hBrush;
    }

    case WM_CTLCOLORLISTBOX:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, TEXT_COLOR);
        SetBkColor(hdc, PANEL_COLOR);
        static HBRUSH hBrush = CreateSolidBrush(PANEL_COLOR);
        return (INT_PTR)hBrush;
    }

    case WM_HOTKEY:
    {
        if (wParam == 1)
        {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            SetFocus(g_hEdit);

            // Fade in
            g_iAlpha = 0;
            g_bFadingIn = true;
            SetTimer(hwnd, TIMER_FADE_IN, 15, NULL);
        }
        return 0;
    }

    case WM_ACTIVATE:
    {
        if (LOWORD(wParam) == WA_INACTIVE)
        {
            // Hide when losing focus
            ShowWindow(hwnd, SW_HIDE);
            SendMessageW(g_hEdit, WM_SETTEXT, 0, (LPARAM)L"");
            SendMessageW(g_hResult, LB_RESETCONTENT, 0, 0);
        }
        return 0;
    }

    case WM_DESTROY:
    {
        if (g_hBackground)
            DeleteObject(g_hBackground);
        if (g_hFont)
            DeleteObject(g_hFont);
        if (g_hFontBold)
            DeleteObject(g_hFontBold);
        if (g_hFontSmall)
            DeleteObject(g_hFontSmall);

        RemoveFontResourceExW((LPCWSTR)FONT_PATH, FR_PRIVATE, NULL);
        UnregisterHotKey(hwnd, 1);

        Gdiplus::GdiplusShutdown(gdiplusToken);

        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    indexer.loadFromDisk();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG_COLOR);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    RegisterClassW(&wc);

    // Create layered window for transparency/fade effects
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST,
        CLASS_NAME,
        L"Search",
        WS_POPUP,
        (screenWidth / 2) - 375, (screenHeight / 2) - 240,
        750, 480,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd)
        return 0;

    // Enable rounded corners (Windows 11)
    DWM_WINDOW_CORNER_PREFERENCE corner = DWMWCP_ROUND;
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &corner, sizeof(corner));

    // Set window shadow
    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
    ShowWindow(hwnd, nCmdShow);

    RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 'S');
    APP_PROTOCOL_HANDLER();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}