
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <string>
#include <limits>

bool screenshot_fullscreen(HWND owner)
{
    // Capture the entire virtual desktop.
    const int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (w <= 0 || h <= 0)
        return false;

    // Limit allocation size and prevent overflow.
    const unsigned long long pixelBytes =
        4ULL * (unsigned long long)w * (unsigned long long)h;

    if (pixelBytes > 0xFFFFFFFFULL -
                     sizeof(BITMAPINFOHEADER))
        return false;

    HDC screen = GetDC(nullptr);
    if (!screen)
        return false;

    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, w, h);

    if (!memory || !bitmap)
    {
        if (bitmap) DeleteObject(bitmap);
        if (memory) DeleteDC(memory);
        ReleaseDC(nullptr, screen);
        return false;
    }

    HGDIOBJ old = SelectObject(memory, bitmap);

    if (!old || old == HGDI_ERROR)
    {
        DeleteObject(bitmap);
        DeleteDC(memory);
        ReleaseDC(nullptr, screen);
        return false;
    }

    BOOL captured = BitBlt(
        memory, 0, 0, w, h,
        screen, x, y,
        SRCCOPY | CAPTUREBLT
    );

    SelectObject(memory, old);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);

    if (!captured)
    {
        DeleteObject(bitmap);
        return false;
    }

    // CF_DIB: bitmap header followed by pixel data.
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = (DWORD)pixelBytes;

    const SIZE_T dibSize =
        sizeof(BITMAPINFOHEADER) + (SIZE_T)pixelBytes;

    HGLOBAL dib = GlobalAlloc(GMEM_MOVEABLE, dibSize);

    if (!dib)
    {
        DeleteObject(bitmap);
        return false;
    }

    BYTE* data = static_cast<BYTE*>(GlobalLock(dib));

    if (!data)
    {
        GlobalFree(dib);
        DeleteObject(bitmap);
        return false;
    }

    // Read bitmap pixels from GDI.
    HDC dc = GetDC(nullptr);

    int result = 0;

    if (dc)
    {
        result = GetDIBits(
            dc,
            bitmap,
            0,
            (UINT)h,
            data + sizeof(BITMAPINFOHEADER),
            &info,
            DIB_RGB_COLORS
        );

        ReleaseDC(nullptr, dc);
    }

    DeleteObject(bitmap);

    if (result != h)
    {
        GlobalUnlock(dib);
        GlobalFree(dib);
        return false;
    }

    memcpy(data, &info.bmiHeader,
           sizeof(BITMAPINFOHEADER));

    // Save BMP next to the executable.
    char exePath[MAX_PATH] = {};

    DWORD n = GetModuleFileNameA(
        nullptr, exePath, MAX_PATH);

    bool saved = false;

    if (n > 0 && n < MAX_PATH)
    {
        std::string folder(exePath);
        size_t slash = folder.find_last_of("\\/");

        if (slash != std::string::npos)
        {
            folder.resize(slash);
            folder += "\\screenshots";

            if (CreateDirectoryA(folder.c_str(), nullptr) ||
                GetLastError() == ERROR_ALREADY_EXISTS)
            {
                SYSTEMTIME t;
                GetLocalTime(&t);

                char name[100];

                std::snprintf(
                    name, sizeof(name),
                    "\\GhostCursor_%04u%02u%02u_"
                    "%02u%02u%02u_%03u.bmp",
                    (unsigned)t.wYear,
                    (unsigned)t.wMonth,
                    (unsigned)t.wDay,
                    (unsigned)t.wHour,
                    (unsigned)t.wMinute,
                    (unsigned)t.wSecond,
                    (unsigned)t.wMilliseconds
                );

                std::string path = folder + name;

                BITMAPFILEHEADER fileHeader = {};
                fileHeader.bfType = 0x4D42; // BM
                fileHeader.bfOffBits =
                    sizeof(BITMAPFILEHEADER) +
                    sizeof(BITMAPINFOHEADER);
                fileHeader.bfSize =
                    (DWORD)(sizeof(BITMAPFILEHEADER) +
                            dibSize);

                FILE* file = std::fopen(path.c_str(), "wb");

                if (file)
                {
                    saved =
                        std::fwrite(
                            &fileHeader,
                            sizeof(fileHeader),
                            1, file) == 1 &&
                        std::fwrite(
                            data, 1, dibSize, file) == dibSize;

                    if (std::fclose(file) != 0)
                        saved = false;

                    if (!saved)
                        DeleteFileA(path.c_str());
                }
            }
        }
    }

    GlobalUnlock(dib);

    // Put the image directly on the Windows clipboard.
    bool copied = false;

    // Retry briefly if another app temporarily owns it.
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        if (OpenClipboard(owner))
        {
            if (EmptyClipboard())
            {
                if (SetClipboardData(CF_DIB, dib))
                {
                    copied = true;

                    // Clipboard now owns dib.
                    dib = nullptr;
                }
            }

            CloseClipboard();
            break;
        }

        Sleep(30);
    }

    if (dib)
        GlobalFree(dib);

    if (!saved)
        std::fprintf(stderr,
            "[WARNING] Could not save screenshot.\n");

    if (!copied)
        std::fprintf(stderr,
            "[WARNING] Could not copy screenshot.\n");

    return saved && copied;
}
