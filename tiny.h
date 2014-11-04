#ifndef H_TINY
#define H_TINY

#include <Windows.h>
#include "crt.h"

unsigned int SwapColorOrder(unsigned int color)
{
    return 
          ((color & 0xff0000) >> 16)
        | ((color & 0x00ff00) >>  0)
        | ((color & 0x0000ff) << 16);
}

void SetClipboardText(char *output)
{
    // len: #aabbcc\0 <- 8db
    const SIZE_T len = 8;

    // allocate space for clipboard only once
    static HGLOBAL hOSMem = NULL;

    if (hOSMem == NULL)
    {
        // allocate clipboard-size space in OS memory
        hOSMem = GlobalAlloc(GMEM_MOVEABLE, len);
    }

    hOSMem = GlobalLock(hOSMem);
    tiny_memcpy(hOSMem, output, len);
    GlobalUnlock(hOSMem);
    
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hOSMem);
    CloseClipboard();
}

unsigned int GetPixelColorRGB(POINT curpos)
{
    HDC dc = GetDC(NULL);
    COLORREF color = GetPixel(dc, curpos.x, curpos.y);
    DeleteDC(dc);
    return SwapColorOrder(color);
}

void DrawBitmapRect(
    HDC display, 
    int x, int y, 
    int width, int height, 
    int hexcolor
){
    HBRUSH brush = CreateSolidBrush(SwapColorOrder(hexcolor));
    RECT r = {x, y, width, height};
    FillRect(display, &r, brush);
    DeleteObject(brush);
}

void CreateBitmapForIcon(void)
{
    HDC hDisplay = GetDC(NULL);
    g_hbmColorIcon = CreateCompatibleBitmap(hDisplay, 16, 16);
    HDC hCanvas = CreateCompatibleDC(hDisplay);
    HBITMAP hbmIcon = (HBITMAP)SelectObject(hCanvas, g_hbmColorIcon);
    // this will be the black border:
    DrawBitmapRect(hCanvas, 0, 0, 16, 16, 0x000000);
    SelectObject(hCanvas, hbmIcon);
    DeleteDC(hCanvas);
    DeleteDC(hDisplay);
}

HICON HBitmapToHIcon(HBITMAP hBmp)
{
    BITMAP bm;
    HDC hDisplay = GetDC(NULL);
    GetObject(hBmp, sizeof(BITMAP), &bm);
    HBITMAP hbmMask = CreateCompatibleBitmap(hDisplay, bm.bmWidth, bm.bmHeight);
    ICONINFO iconinf;
    tiny_memset(&iconinf, 0, sizeof(ICONINFO));
    iconinf.fIcon    = TRUE;
    iconinf.hbmColor = hBmp;
    iconinf.hbmMask  = hbmMask;
    HICON hIcon = CreateIconIndirect(&iconinf);

    DeleteDC(hDisplay);
    DeleteObject(hbmMask);
    
    return hIcon;
}

void RefreshBitmapIcon(POINT curpos, int keyPressed)
{
    static int s_lastPixelColor = 0xffffff;
    static int s_lastKeyPress = 0;
    int pixelColor = GetPixelColorRGB(curpos);
    
    // if the current color is the same as the one before
    // we simply skip refreshing the icon
    // if key state is changing we also want to refresh the icon
    if (s_lastPixelColor == pixelColor && (keyPressed == s_lastKeyPress))
        return;
    else
    {
        s_lastPixelColor = pixelColor;
        s_lastKeyPress = keyPressed;
    }
    
    HDC hDisplay = GetDC(NULL);
    HDC hCanvas = CreateCompatibleDC(hDisplay);
    HBITMAP hIcon = (HBITMAP)SelectObject(hCanvas, g_hbmColorIcon);

    // if the key is pressed, we put a little red border around the icon
    if (keyPressed != 0)
    {
        DrawBitmapRect(hCanvas, 0, 0, 16, 16, 0xff0000);
        DrawBitmapRect(hCanvas, 2, 2, 14, 14, pixelColor);
    }
    // otherwise, black background
    else {
        DrawBitmapRect(hCanvas, 0, 0, 16, 16, 0x000000);
        DrawBitmapRect(hCanvas, 1, 1, 15, 15, pixelColor);
    }
    
    SelectObject(hCanvas, hIcon);
    DeleteDC(hCanvas);
    DeleteDC(hDisplay);

    g_TrayIconInfo.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_TrayIconInfo.hIcon = HBitmapToHIcon(g_hbmColorIcon);
    Shell_NotifyIcon(NIM_MODIFY, &g_TrayIconInfo);
}

void CreateTrayIcon(
    HWND hwnd, 
    NOTIFYICONDATA *trayinf, 
    const char *title, 
    const char *message
){
    //ZeroMemory(trayinf, sizeof(NOTIFYICONDATA));
    tiny_memset(trayinf, 0, sizeof(NOTIFYICONDATA));
    trayinf->cbSize           = sizeof( NOTIFYICONDATA );
    trayinf->hWnd             = hwnd;
    trayinf->uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
    trayinf->dwInfoFlags      = NIIF_NONE;
    trayinf->uCallbackMessage = WM_TRAY_EVENT;
    
    // actually  it doesn't make any sense because
    // the balloon message stays for like 3500 ms in any case
    trayinf->uTimeout         = 5 * 1000;
    CreateBitmapForIcon();
    trayinf->hIcon            = HBitmapToHIcon(g_hbmColorIcon);
    
    // WinAPI strcpy (omit CRT)
    lstrcpy(trayinf->szInfoTitle, title);
    lstrcpy(trayinf->szInfo, message);
    lstrcpy(trayinf->szTip, g_ProgramTitle);
    
    Shell_NotifyIcon(NIM_ADD, trayinf);
}

DWORD WINAPI Thread_PixelToClipboard(LPVOID lpParam)
{
    int keypress;

    // this is the key-hook loop
    for(;;)
    {
        // every 100ms we get the cursor position
        POINT curpos;
        GetCursorPos(&curpos);

        if (keypress = (GetAsyncKeyState(VK_SCROLL) != 0))
        {
            // if a keypress has happened we set the clipboard
            // to the hex value of the color under the cursor
            char clip_txt[8];
            // WinAPI wsprintf (omit CRT)
            wsprintf(clip_txt, "#%06x", GetPixelColorRGB(curpos));
            SetClipboardText(clip_txt);
        }

        // every 100ms we update the tray icon
        // so that it shows the current color
        RefreshBitmapIcon(curpos, keypress);
        Sleep(100);
    }
    return 0;
}
#endif
