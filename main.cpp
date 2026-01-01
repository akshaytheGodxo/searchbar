#include <windows.h>

#define ID_EDITBOX    1001
#define ID_BUTTON_OK  1002

const wchar_t CLASS_NAME[] = L"SearchBar";
HBITMAP g_hBackground = NULL;
HBITMAP hBGImage;
HWND hBGhandler;

void loadImage() {
    hBGImage = (HBITMAP)LoadImageW(
        NULL,
		L"E:\\Personal Projects\\myengine\\assets\\makimaaaa.bmp",
        IMAGE_BITMAP,
        0,0,
        LR_LOADFROMFILE
    );
}

void DrawCustomButton(LPDRAWITEMSTRUCT lpDrawItem) {

    COLORREF bgColor = RGB(255, 165, 0);     // orange
    COLORREF textColor = RGB(128, 128, 128);   // grey

    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, hBrush);
    DeleteObject(hBrush);

    HFONT hFont = CreateFontW(
        20, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Helvetica"
    );
    HFONT hOldFont = (HFONT) SelectObject(lpDrawItem->hDC, hFont);

    SetTextColor(lpDrawItem->hDC, textColor);
    SetBkMode(lpDrawItem->hDC, TRANSPARENT);

    wchar_t text[256];
    GetWindowTextW(lpDrawItem->hwndItem, text, 256);

    RECT rcText = lpDrawItem->rcItem;
    DrawTextW(lpDrawItem->hDC, text, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(lpDrawItem->hDC, hOldFont);
    DeleteObject(hFont);

    if (lpDrawItem->itemState & ODS_FOCUS) {
        DrawFocusRect(lpDrawItem->hDC, &lpDrawItem->rcItem);
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT hFontButton = NULL;
    switch (msg)
    {
    case WM_CREATE:
    {
        


        LPCREATESTRUCTW pcs = (LPCREATESTRUCTW)lParam;
        loadImage();

        hBGhandler = CreateWindowW(
            L"STATIC",
            NULL,
            WS_CHILD | WS_VISIBLE | SS_BITMAP,
            0, 0, 800, 600,
            NULL,
            NULL,
            pcs->hInstance,
            NULL
        );

        SendMessageW(hBGhandler,
            STM_SETIMAGE,
            IMAGE_BITMAP,
            (LPARAM)hBGImage);

        HWND hEdit = CreateWindowExW(
            0,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            3, 10, 780, 25,
            hwnd,
            (HMENU)ID_EDITBOX,
            pcs->hInstance,
            NULL
        );
        

    }
    break;

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
        if (lpDrawItem->CtlID == ID_BUTTON_OK) {
            DrawCustomButton(lpDrawItem);
            return TRUE;
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        TextOutW(hdc, 10, 10, L"Hello, Win32!", 13);

        EndPaint(hwnd, &ps);
        return 0;
    }
    break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);

}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow)
{
    
    

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = NULL;
	RegisterClass(&wc);


    

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"My Win32 App",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);
   
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
