#ifndef GHOSTCURSOR_CURSOR_H
#define GHOSTCURSOR_CURSOR_H

#include <windows.h>
#include <stdbool.h>

bool cursor_hide(void);
void cursor_restore(void);
bool cursor_is_hidden(void);

#endif