#ifndef H_TINY
#define H_TINY

#include <Windows.h>
#include <cstdio>

static inline unsigned int SwapColorOrder(unsigned int color)
{
	return ((color&0xff0000)>>16) | (color&0x00ff00) | ((color&0x0000ff)<<16);
}

void SetClipboardText(char *output){
	const SIZE_T len = 8; // #ffffff0 <- 8db
	HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
	
	memcpy(GlobalLock(hMem), output, len);
	GlobalUnlock(hMem);
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

unsigned int GetPixelColorRGB(POINT curpos){
	HDC dc = GetDC(NULL);
	COLORREF color = GetPixel(dc, curpos.x, curpos.y);
	DeleteDC(dc);
	return SwapColorOrder(color);
}

void DrawBitmapRect(
	HDC display, 
	int x, int y, int width, 
	int height, 
	int hexcolor
){
	// rgb -> bgr :(
	hexcolor = SwapColorOrder(hexcolor);
	HBRUSH brush = CreateSolidBrush(hexcolor);
	RECT r = {x, y, width, height};
	FillRect(display,&r,brush);
	DeleteObject(brush);
}

void CreateBitmapForIcon() {
	HDC wDC = GetDC(NULL);
	g_hbmColorIcon = CreateCompatibleBitmap(wDC,16,16);
	HDC bDC = CreateCompatibleDC(wDC);
	HBITMAP bOld = (HBITMAP)SelectObject(bDC,g_hbmColorIcon);
	DrawBitmapRect(bDC, 0, 0, 16, 16, 0x000000);
	SelectObject(bDC, bOld);
	DeleteDC(bDC);
	DeleteDC(wDC);
}

HICON HBitmapToHIcon(HBITMAP hBmp)
{
	BITMAP bm;
	HDC tmpDC = GetDC(NULL);
	GetObject(hBmp, sizeof(BITMAP), &bm);
	HBITMAP hbmMask = CreateCompatibleBitmap(tmpDC, bm.bmWidth, bm.bmHeight);
	ICONINFO iconinf;
	ZeroMemory(&iconinf, sizeof(ICONINFO));
	iconinf.fIcon    = TRUE;
	iconinf.hbmColor = hBmp;
	iconinf.hbmMask  = hbmMask;
	HICON hIcon = CreateIconIndirect(&iconinf);

	DeleteDC(tmpDC);
	DeleteObject(hbmMask);
	
	return hIcon;
}

void RefreshBitmapIcon(POINT curpos, int key_pressed)
{
	HDC wDC = GetDC(NULL);
	HDC bDC = CreateCompatibleDC(wDC);
	HBITMAP bOld = (HBITMAP)SelectObject(bDC, g_hbmColorIcon);
	
	// if the key is pressed, we put a little red border around the icon
	if (key_pressed != 0)
	{
		DrawBitmapRect(bDC, 0, 0, 16, 16, 0xff0000);
		DrawBitmapRect(bDC, 2, 2, 14, 14, GetPixelColorRGB(curpos));
	}
	// otherwise, black background
	else {
		DrawBitmapRect(bDC, 0, 0, 16, 16, 0x000000);
		DrawBitmapRect(bDC, 1, 1, 15, 15, GetPixelColorRGB(curpos));
	}
	
	SelectObject(bDC, bOld);
	DeleteDC(bDC);
	DeleteDC(wDC);

	g_TrayIconInfo.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	g_TrayIconInfo.hIcon = HBitmapToHIcon(g_hbmColorIcon);
	Shell_NotifyIcon(NIM_MODIFY, &g_TrayIconInfo);
}

void CreateTrayIcon(HWND hwnd, NOTIFYICONDATA *trayinf, const char *title, const char *message)
{
	ZeroMemory(trayinf, sizeof(NOTIFYICONDATA));
	trayinf->cbSize		      = sizeof( NOTIFYICONDATA );
	trayinf->hWnd			  = hwnd;
	trayinf->uFlags		      = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
	trayinf->dwInfoFlags      = NIIF_NONE;
	trayinf->uCallbackMessage = WM_TRAY_DBCLK;
	trayinf->uTimeout         = 100 * 1000;
	CreateBitmapForIcon();
	trayinf->hIcon            = HBitmapToHIcon(g_hbmColorIcon);
	
	strcpy(trayinf->szInfoTitle, title);
	strcpy(trayinf->szInfo, message);
	strcpy(trayinf->szTip, "Tiny Color Picker");
	
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
			sprintf(clip_txt, "#%06x", GetPixelColorRGB(curpos));
			SetClipboardText(clip_txt);
		}

		// every 100ms we update the tray icon
		// so that it shows the current color
		RefreshBitmapIcon(curpos, keypress);

		Sleep(1000/10);
	}
	return 0;
}

#endif
