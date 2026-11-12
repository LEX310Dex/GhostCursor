
#include "cursor.h"
#include <stdio.h>
#include <string.h>

static bool hidden = false;

/* Standard Windows system cursor IDs */
static const DWORD cursor_ids[] = {
    32512, /* Arrow */
    32513, /* Text */
    32514, /* Wait */
    32515, /* Crosshair */
    32516, /* Up arrow */
    32642, /* Resize NW-SE */
    32643, /* Resize NE-SW */
    32644, /* Resize W-E */
    32645, /* Resize N-S */
    32646, /* Move */
    32648, /* Unavailable */
    32649, /* Hand */
    32650, /* App starting */
    32651  /* Help */
};

static HCURSOR create_invisible_cursor(void)
{
    /* 32x32 monochrome cursor, 128 bytes per mask.
       AND=1, XOR=0 means preserve background pixels. */
    BYTE and_mask[128];
    BYTE xor_mask[128];

    memset(and_mask, 0xFF, sizeof(and_mask));
    memset(xor_mask, 0x00, sizeof(xor_mask));

    return CreateCursor(
        GetModuleHandle(NULL),
        0, 0,
        32, 32,
        and_mask,
        xor_mask
    );
}

bool cursor_hide(void)
{
    if (hidden)
        return true;

    size_t count = sizeof(cursor_ids) / sizeof(cursor_ids[0]);

    for (size_t i = 0; i < count; i++)
    {
        HCURSOR invisible = create_invisible_cursor();

        if (!invisible)
        {
            fprintf(stderr,
                "[ERROR] CreateCursor failed: %lu\n",
                (unsigned long)GetLastError());
            cursor_restore();
            return false;
        }

        if (!SetSystemCursor(invisible, cursor_ids[i]))
        {
            DWORD error = GetLastError();
            DestroyCursor(invisible);

            fprintf(stderr,
                "[ERROR] SetSystemCursor failed: %lu\n",
                (unsigned long)error);

            cursor_restore();
            return false;
        }

        /* SetSystemCursor destroys the cursor on success. */
    }

    hidden = true;
    return true;
}

void cursor_restore(void)
{
    if (!SystemParametersInfo(
            SPI_SETCURSORS, 0, NULL, 0))
    {
        fprintf(stderr,
            "[ERROR] Cursor restoration failed: %lu\n",
            (unsigned long)GetLastError());
        return;
    }

    hidden = false;
}

bool cursor_is_hidden(void)
{
    return hidden;
}
