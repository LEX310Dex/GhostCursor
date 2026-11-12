
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00

#include <windows.h>
#include <cstdio>
extern "C" {
#include "cursor.h"
}
bool screenshot_fullscreen(HWND owner);

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

#define HOTKEY_HIDE    1
#define HOTKEY_SHOW    2
#define HOTKEY_EXIT    3
#define HOTKEY_SCREENSHOT 4
#define TIMER_TRACK    1

static HWND overlay = nullptr;
static HCURSOR originalArrow = nullptr;

static bool active = false;
static bool exclusionReady = false;

static int hotspotX = 0;
static int hotspotY = 0;
static int cursorWidth = 32;
static int cursorHeight = 32;

static const COLORREF TRANSPARENT_KEY =
    RGB(255, 0, 255);

// Paint our local replacement cursor.
static void paintOverlay(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);

    RECT rect;
    GetClientRect(hwnd, &rect);

    HBRUSH brush = CreateSolidBrush(TRANSPARENT_KEY);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);

    if (originalArrow)
    {
        DrawIconEx(
            dc, 0, 0,
            originalArrow,
            cursorWidth,
            cursorHeight,
            0, nullptr,
            DI_NORMAL
        );
    }

    EndPaint(hwnd, &ps);
}

// Follow the real mouse position without
// interfering with clicks or keyboard input.
static void updatePosition()
{
    if (!active || !overlay)
        return;

    POINT pos;

    if (!GetCursorPos(&pos))
        return;

    SetWindowPos(
        overlay,
        HWND_TOPMOST,
        pos.x - hotspotX,
        pos.y - hotspotY,
        0, 0,
        SWP_NOSIZE |
        SWP_NOACTIVATE |
        SWP_SHOWWINDOW
    );
}

// Switch GhostCursor ON.
static void activateGhost()
{
    if (active)
        return;

    if (!exclusionReady)
    {
        printf("[ERROR] Capture exclusion unavailable.\n");
        return;
    }

    // The replacement overlay is already configured
    // to be omitted from supported capture methods.
    if (!cursor_hide())
    {
        printf("[ERROR] System cursor hiding failed.\n");
        return;
    }

    active = true;

    InvalidateRect(overlay, nullptr, TRUE);
    ShowWindow(overlay, SW_SHOWNOACTIVATE);
    updatePosition();

    printf("[ON] GhostCursor mode enabled.\n");
    printf("[INFO] Local replacement cursor active.\n");
    printf("[INFO] Capture exclusion requested.\n");
}

// Restore ordinary Windows cursor behavior.
static void deactivateGhost()
{
    active = false;

    if (overlay)
        ShowWindow(overlay, SW_HIDE);

    cursor_restore();

    printf("[OFF] Normal cursor restored.\n");
}

static LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wp,
    LPARAM lp)
{
    switch (msg)
    {
    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
        paintOverlay(hwnd);
        return 0;

    case WM_TIMER:
        if (wp == TIMER_TRACK)
            updatePosition();
        return 0;

    case WM_HOTKEY:
        switch (wp)
        {
        case HOTKEY_HIDE:
            activateGhost();
            break;

        case HOTKEY_SHOW:
            deactivateGhost();
            break;

        case HOTKEY_EXIT:
            DestroyWindow(hwnd);
            break;
        
	case HOTKEY_SCREENSHOT:
	    screenshot_fullscreen(hwnd);
	    break;
}
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        active = false;
        cursor_restore();
        KillTimer(hwnd, TIMER_TRACK);

        UnregisterHotKey(hwnd, HOTKEY_HIDE);
        UnregisterHotKey(hwnd, HOTKEY_SHOW);
        UnregisterHotKey(hwnd, HOTKEY_EXIT);
	UnregisterHotKey(hwnd, HOTKEY_SCREENSHOT);

        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

int main()
{
    SetProcessDPIAware();

    printf("\n");
    printf("===================================\n");
    printf("  GhostCursor v0.3 - Local Overlay\n");
    printf("===================================\n\n");

    // Save the original pointer image BEFORE
    // replacing system cursor shapes.
    HCURSOR systemArrow =
        LoadCursor(nullptr, IDC_ARROW);

    if (!systemArrow)
    {
        printf("[ERROR] Cannot load system arrow.\n");
        return 1;
    }

    originalArrow = CopyIcon(systemArrow);

    if (!originalArrow)
    {
        printf("[ERROR] Cannot copy cursor image.\n");
        return 1;
    }

    ICONINFO info = {};

    if (GetIconInfo(originalArrow, &info))
    {
        hotspotX = (int)info.xHotspot;
        hotspotY = (int)info.yHotspot;

        if (info.hbmColor)
            DeleteObject(info.hbmColor);

        if (info.hbmMask)
            DeleteObject(info.hbmMask);
    }

    cursorWidth = GetSystemMetrics(SM_CXCURSOR);
    cursorHeight = GetSystemMetrics(SM_CYCURSOR);

    if (cursorWidth < 1) cursorWidth = 32;
    if (cursorHeight < 1) cursorHeight = 32;

    HINSTANCE instance = GetModuleHandle(nullptr);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "GhostCursorOverlayClass";

    if (!RegisterClassA(&wc))
    {
        printf("[ERROR] RegisterClass failed: %lu\n",
            (unsigned long)GetLastError());

        DestroyIcon(originalArrow);
        return 1;
    }

    DWORD exStyle =
        WS_EX_LAYERED |
        WS_EX_TRANSPARENT |
        WS_EX_TOOLWINDOW |
        WS_EX_NOACTIVATE |
        WS_EX_TOPMOST;

    overlay = CreateWindowExA(
        exStyle,
        wc.lpszClassName,
        "GhostCursor Local Pointer",
        WS_POPUP,
        0, 0,
        cursorWidth, cursorHeight,
        nullptr, nullptr,
        instance, nullptr
    );

    if (!overlay)
    {
        printf("[ERROR] Cannot create overlay: %lu\n",
            (unsigned long)GetLastError());

        DestroyIcon(originalArrow);
        return 1;
    }

    // Magenta pixels become fully transparent.
    if (!SetLayeredWindowAttributes(
            overlay,
            TRANSPARENT_KEY,
            0,
            LWA_COLORKEY))
    {
        printf("[ERROR] Layered window setup failed.\n");
        DestroyWindow(overlay);
        DestroyIcon(originalArrow);
        return 1;
    }

    // Keep our pointer overlay visible locally,
    // but request exclusion from screen captures.
    if (SetWindowDisplayAffinity(
            overlay,
            WDA_EXCLUDEFROMCAPTURE))
    {
        DWORD affinity = 0;

        if (GetWindowDisplayAffinity(
                overlay, &affinity) &&
            affinity == WDA_EXCLUDEFROMCAPTURE)
        {
            exclusionReady = true;
            printf("[OK] Display affinity configured.\n");
        }
    }

    if (!exclusionReady)
    {
        printf("[ERROR] Display exclusion failed.\n");
        printf("[INFO] No cursors will be hidden.\n");

        DestroyWindow(overlay);
        DestroyIcon(originalArrow);
        return 1;
    }


    // Register hotkeys separately to identify conflicts.
    struct HotkeyEntry
    {
        int id;
        UINT key;
        const char* name;
    };

    const HotkeyEntry hotkeys[] = {
        {HOTKEY_HIDE,       'H', "Ctrl+Alt+H"},
        {HOTKEY_SHOW,       'R', "Ctrl+Alt+R"},
        {HOTKEY_EXIT,       'Q', "Ctrl+Alt+Q"},
	{HOTKEY_SCREENSHOT, 'S', "Ctrl+Shift+Alt+S"}
    };

    bool hotkeysOK = true;

    for (const auto& hk : hotkeys)
    {

	UINT modifiers = MOD_CONTROL | MOD_ALT | MOD_NOREPEAT;

	if (hk.id == HOTKEY_SCREENSHOT)
	{
	    modifiers |= MOD_SHIFT;

	}	

        if (RegisterHotKey(
                overlay,
                hk.id,
	        modifiers,
                hk.key))
        {
            printf("[OK] Registered %s\n", hk.name);
        }
        else
        {
            DWORD error = GetLastError();

            printf(
                "[ERROR] Failed to register %s "
                "(Windows error: %lu)\n",
                hk.name,
                (unsigned long)error
            );

            hotkeysOK = false;
        }
    }

    if (!hotkeysOK)
    {
        printf("[ERROR] Hotkey conflict detected.\n");

        // WM_DESTROY also unregisters all four hotkeys.
        DestroyWindow(overlay);
        DestroyIcon(originalArrow);
        return 1;
    }



    SetTimer(overlay, TIMER_TRACK, 10, nullptr);

    printf("\n[READY] GhostCursor background controller.\n");
    printf("Ctrl+Alt+H : Enable GhostCursor mode\n");
    printf("Ctrl+Alt+R : Restore normal cursor\n");
    printf("Ctrl+Shift+Alt+S : Silent screenshot + clipboard\n");
    printf("Ctrl+Alt+Q : Exit safely\n\n");

    MSG msg = {};

    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Final recovery in case the window was
    // closed through an unexpected normal path.
    cursor_restore();

    DestroyIcon(originalArrow);

    printf("[EXIT] GhostCursor closed.\n");
    return 0;
}
