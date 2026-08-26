// viewer.c
#include <windows.h>
#include <stdint.h>
#include "input.h"
#include "ppu.h"
#include "viewer.h"

static HWND viewer_hwnd;
static BITMAPINFO viewer_bmi;
static int viewer_scale = 2;

static LRESULT CALLBACK viewer_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_KEYDOWN:
            input_key_event((unsigned int)wparam, true);
            return 0;
        case WM_KEYUP:
            input_key_event((unsigned int)wparam, false);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(hwnd, msg, wparam, lparam);
}

bool_t viewer_init(void)
{
    WNDCLASSA wc;
    RECT rect = { 0, 0, PPU_FB_WIDTH * viewer_scale, PPU_FB_HEIGHT * viewer_scale };

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = viewer_wndproc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.lpszClassName = "GameBoyLCDViewer";
    wc.hCursor = LoadCursorA(NULL, IDC_ARROW);

    if (!RegisterClassA(&wc))
        return false;

    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    viewer_hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "Game Boy LCD",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        wc.hInstance,
        NULL);

    if (!viewer_hwnd)
        return false;

    ZeroMemory(&viewer_bmi, sizeof(viewer_bmi));
    viewer_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    viewer_bmi.bmiHeader.biWidth = PPU_FB_WIDTH;
    viewer_bmi.bmiHeader.biHeight = -PPU_FB_HEIGHT;
    viewer_bmi.bmiHeader.biPlanes = 1;
    viewer_bmi.bmiHeader.biBitCount = 32;
    viewer_bmi.bmiHeader.biCompression = BI_RGB;

    ShowWindow(viewer_hwnd, SW_SHOW);
    UpdateWindow(viewer_hwnd);
    return true;
}

bool_t viewer_pump(void)
{
    MSG msg;

    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return true;
}

void viewer_present(void)
{
    if (!viewer_hwnd)
        return;

    const uint32_t *fb = ppu_framebuffer();
    HDC dc = GetDC(viewer_hwnd);
    RECT client;

    if (!dc)
        return;

    GetClientRect(viewer_hwnd, &client);
    StretchDIBits(
        dc,
        0,
        0,
        client.right - client.left,
        client.bottom - client.top,
        0,
        0,
        PPU_FB_WIDTH,
        PPU_FB_HEIGHT,
        fb,
        &viewer_bmi,
        DIB_RGB_COLORS,
        SRCCOPY);
    ReleaseDC(viewer_hwnd, dc);
}
