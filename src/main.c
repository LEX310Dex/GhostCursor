#include <windows.h>
#include <stdio.h>
#include "cursor.h"

#define HOTKEY_TOGGLE 1
#define HOTKEY_RESTORE 2
#define HOTKEY_EXIT 3

int main(void)
{
    printf("\n");
    printf("====================================\n");
    printf("       GHOSTCURSOR v0.1\n");
    printf("====================================\n\n");

    if (!RegisterHotKey(NULL, HOTKEY_TOGGLE,
                        MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'H'))
    {
        printf("[ERROR] Could not register Ctrl+Alt+H\n");
        return 1;
    }

    if (!RegisterHotKey(NULL, HOTKEY_RESTORE,
                        MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'R'))
    {
        UnregisterHotKey(NULL, HOTKEY_TOGGLE);
        printf("[ERROR] Could not register Ctrl+Alt+R\n");
        return 1;
    }

    if (!RegisterHotKey(NULL, HOTKEY_EXIT,
                        MOD_CONTROL | MOD_ALT | MOD_NOREPEAT, 'Q'))
    {
        UnregisterHotKey(NULL, HOTKEY_TOGGLE);
        UnregisterHotKey(NULL, HOTKEY_RESTORE);
        printf("[ERROR] Could not register Ctrl+Alt+Q\n");
        return 1;
    }

    printf("[READY] GhostCursor is running.\n\n");
    printf("Ctrl + Alt + H : Toggle cursor visibility\n");
    printf("Ctrl + Alt + R : Restore cursor\n");
    printf("Ctrl + Alt + Q : Exit safely\n\n");

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        if (msg.message == WM_HOTKEY)
        {
            switch (msg.wParam)
            {
                case HOTKEY_TOGGLE:
                    if (cursor_is_hidden())
                    {
                        cursor_restore();
                        printf("[INFO] Cursor restored.\n");
                    }
                    else if (cursor_hide())
                    {
                        printf("[INFO] Cursor hidden.\n");
                    }
                    else
                    {
                        printf("[ERROR] Could not hide cursor.\n");
                    }
                    break;

                case HOTKEY_RESTORE:
                    cursor_restore();
                    printf("[INFO] Cursor restored.\n");
                    break;

                case HOTKEY_EXIT:
                    PostQuitMessage(0);
                    break;
            }
        }
    }

    // Cleanup
    cursor_restore();

    UnregisterHotKey(NULL, HOTKEY_TOGGLE);
    UnregisterHotKey(NULL, HOTKEY_RESTORE);
    UnregisterHotKey(NULL, HOTKEY_EXIT);

    printf("\n[EXIT] GhostCursor closed safely.\n");

    return 0;
}