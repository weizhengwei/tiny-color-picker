// we don't care about safe i/o functions
#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>

// this macro (WM_TRAY_DBCLK) will be the ID for the tray icon interaction
// (since NOTIFYICONDATA's callback is an ID)
// we catch it in WndProc's nMsg which holds it when
// something happens to the tray icon. (eg. mouse click, tab focus)
#define WM_TRAY_DBCLK (WM_USER+100)

// it holds the tray icon data (icon, properties, balloon tip)
NOTIFYICONDATA g_TrayIconInfo;

// handle for the created thread of PixelCapture function
HANDLE g_hfnPixelCapture = 0;

// ??? some data which terminates PixelCapture
// if it could not be created
int g_DataPixelCapture = 1;

// handle for the tray icon image that we create programatically
HBITMAP g_hbmColorIcon;

const char g_ProgramTitle[] = "Tiny Color Picker";

// i could not find a better place to set the balloon tip
const char g_BalloonMessage[] = 
    "Press ScrollLock to copy color to Clipboard.\n"
    "Double-click on tray icon to exit.";

#include "tiny.h"

// WndProc is a callback function which is executed 
// every time some interaction happens (eg. mouse move, close, maximize)
LRESULT CALLBACK WndProc(
    HWND hWnd, 
    UINT nMsg, 
    WPARAM wParam, 
    LPARAM lParam
){  
    switch (nMsg)
    {
        // if we are doing interaction with the tray icon
        case WM_TRAY_DBCLK:
            
            // for a double-click, we simply exit the application
            if (lParam == WM_LBUTTONDBLCLK){
                
                // the tray icon would stay there if 
                // we did not call the delete function
                Shell_NotifyIcon(NIM_DELETE, &g_TrayIconInfo);

                // nMsg becomes WM_DESTROY
                DestroyWindow(hWnd);
            }

            // for right click, we show the balloon tip
            else if (lParam == WM_RBUTTONDOWN){ 

                // uFlags get an additional flag with OR, called NIF_INFO
                // because the balloon tip is not shown
                // (we disabled it after 10 sec) 
                g_TrayIconInfo.uFlags |= NIF_INFO;

                // by sending the flag NIM_MODIFY, we refresh the tray icon
                // to show the balloon again
                Shell_NotifyIcon(NIM_MODIFY, &g_TrayIconInfo); 
            }
            break;

        // initializing 
        case WM_CREATE:

            // we set up the tray icon
            // at that moment the icon only shows 
            // a still color of the pixel under the current mouse position
            // that means we update the animation somewhere else
            CreateTrayIcon(
                hWnd, 
                &g_TrayIconInfo, 
                g_ProgramTitle, 
                g_BalloonMessage
            );

            // since WndProc is an asynchronous function 
            // (and by that i mean if you do nothing it is not even called)
            // we cannot rely on it to start a key-hook loop
            // so we have to create another thread (PixelToClipboard)
            // which will catch our keypresses
            g_hfnPixelCapture = CreateThread(
                NULL, 0, 
                Thread_PixelToClipboard, 
                &g_DataPixelCapture, 
                0, NULL
            );
            break;

        // after closing the application
        // nMsg becomes WM_DESTROY so we can free memory 
        case WM_DESTROY:
            // get out of everything we were in
            ExitProcess(g_DataPixelCapture);
            CloseHandle(g_hfnPixelCapture);
            DeleteObject(g_hbmColorIcon);
            PostQuitMessage(0);
            break;
        default:
            // TODO: is this important?
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPSTR lpCmdLine, 
    int nCmdShow
){
    // the following stuff is just a basic win32 application creation 
    // without window

    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    //ZeroMemory(&wc, sizeof( WNDCLASSEX));
    tiny_memset(&wc, 0, sizeof(WNDCLASSEX));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = g_ProgramTitle;

    RegisterClassEx(&wc);

    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_ProgramTitle,
        g_ProgramTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 256, 64,
        NULL, NULL, hInstance, NULL
    );

    // disable window by not showing the window
    // ShowWindow(hwnd, nCmdShow);
    // UpdateWindow(hwnd);

    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}
