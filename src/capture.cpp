
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

static HDC screenDC = nullptr;
static HDC memoryDC = nullptr;
static HBITMAP bitmap = nullptr;
static HGDIOBJ previousBitmap = nullptr;

static int screenW = 0;
static int screenH = 0;
static bool running = true;

static void cleanupCapture()
{
    if (memoryDC && previousBitmap)
        SelectObject(memoryDC, previousBitmap);

    if (bitmap)
        DeleteObject(bitmap);

    if (memoryDC)
        DeleteDC(memoryDC);

    if (screenDC)
        ReleaseDC(nullptr, screenDC);
}

static bool initializeCapture()
{
    screenW = GetSystemMetrics(SM_CXSCREEN);
    screenH = GetSystemMetrics(SM_CYSCREEN);

    screenDC = GetDC(nullptr);
    if (!screenDC)
        return false;

    memoryDC = CreateCompatibleDC(screenDC);
    if (!memoryDC)
        return false;

    bitmap = CreateCompatibleBitmap(
        screenDC, screenW, screenH);

    if (!bitmap)
        return false;

    previousBitmap =
        SelectObject(memoryDC, bitmap);

    if (!previousBitmap ||
        previousBitmap == HGDI_ERROR)
    {
        previousBitmap = nullptr;
        return false;
    }

    return true;
}

static bool captureFrame()
{
    return BitBlt(
        memoryDC,
        0, 0,
        screenW, screenH,
        screenDC,
        0, 0,
        SRCCOPY | CAPTUREBLT
    ) != 0;
}

static LRESULT CALLBACK WindowProc(
    HWND hwnd, UINT msg,
    WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_TIMER:
        if (captureFrame())
            InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        SetStretchBltMode(dc, HALFTONE);

        StretchBlt(
            dc,
            0, 0,
            rect.right, rect.bottom,
            memoryDC,
            0, 0,
            screenW, screenH,
            SRCCOPY
        );

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_KEYDOWN:
        if (wp == VK_ESCAPE)
            DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        running = false;
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

int main()
{
    SetProcessDPIAware();

    printf("GhostCapture v0.2\n");
    printf("Initializing desktop capture...\n");

    if (!initializeCapture())
    {
        printf("[ERROR] Capture initialization failed.\n");
        cleanupCapture();
        return 1;
    }

    HINSTANCE instance = GetModuleHandle(nullptr);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "GhostCaptureWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassA(&wc))
    {
        cleanupCapture();
        return 1;
    }

    HWND hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "GhostCapture - Cursor-Free Preview",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1000, 650,
        nullptr, nullptr,
        instance, nullptr
    );

    if (!hwnd)
    {
        cleanupCapture();
        return 1;
    }

    SetTimer(hwnd, 1, 50, nullptr);

    printf("[READY] Capture running.\n");
    printf("[INFO] Primary monitor: %d x %d\n",
           screenW, screenH);
    printf("[INFO] Press Escape in preview to exit.\n");

    MSG msg = {};

    while (running &&
           GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cleanupCapture();

    printf("[EXIT] Capture stopped.\n");
    return 0;
}
