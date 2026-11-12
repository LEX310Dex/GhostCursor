<div align="center">

# 👻 GhostCursor

### Keep your cursor. Share your screen without it.

**A lightweight Windows utility that keeps a local replacement pointer visible while excluding it from supported full-screen capture pipelines.**

![Windows](https://img.shields.io/badge/Windows-10%202004%2B%20%7C%2011-0078D4?logo=windows&logoColor=white)
![Language](https://img.shields.io/badge/C%20%2B%20C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Build](https://img.shields.io/badge/Build-MSYS2%20UCRT64-blue)
![Status](https://img.shields.io/badge/Status-Prototype-orange)

**No OBS · No virtual camera · No preview window**

</div>

---

## ✨ What it does

When GhostCursor mode is on, the Windows system cursor is replaced with a transparent cursor. A small click-through overlay draws a local pointer and requests exclusion from compatible screen captures via `SetWindowDisplayAffinity(WDA_EXCLUDEFROMCAPTURE)`.

**On your monitor:** a pointer you can use normally.  
**In compatible full-screen shares:** the replacement pointer is omitted.

> **Tested by the project creator:** Google Meet, Microsoft Teams, Chrome screen sharing, and Zoom on their Windows setup. These are informal compatibility tests, not guarantees across all versions, capture backends, or computers.

## ⚡ Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl + Alt + H` | Enable local-only pointer mode |
| `Ctrl + Alt + R` | Restore standard system cursor |
| `Ctrl + Alt + Q` | Restore cursor and quit |
| `Ctrl + Shift + Alt + S` | Silent full-desktop screenshot: clipboard + timestamped BMP file |

Captured screenshots are stored under `build/screenshots/` by default. Clipboard image contents can be pasted using `Ctrl + V` in applications that accept images.

## 🖥️ How it works

```text
                    Windows desktop
                          │
             ┌────────────┴────────────┐
             │                         │
     System cursor              Local pointer overlay
     made transparent           layered + click-through
             │                         │
             │                   WDA_EXCLUDEFROMCAPTURE
             │                         │
             └────────────┬────────────┘
                          │
                Your monitor: pointer visible
                Supported capture: pointer omitted
```

Unlike recording tools, GhostCursor does **not** replace the meeting application's screen-sharing stream. Instead, it relies on Windows composition and display-affinity behavior. Some capture methods may ignore the exclusion request; test before relying on it.

## 🚀 Build from source

**Requirements:** Windows 10 version 2004+ or Windows 11, and MSYS2 UCRT64 GCC/G++ with Win32 headers.

Open PowerShell in the repository root:

```powershell
New-Item -ItemType Directory -Force .\build | Out-Null

gcc -std=c11 -Wall -Wextra -O2 -Iinclude -c src\cursor.c -o build\cursor.o

g++ -std=c++17 -Wall -Wextra -O2 -Iinclude `
    src\ghost_overlay.cpp src\screenshot.cpp build\cursor.o `
    -o build\GhostCursor.exe -luser32 -lgdi32

.\build\GhostCursor.exe
```

> Run the commands using your MSYS2 UCRT64 toolchain (add `C:\msys64\ucrt64\bin` to your PATH or run from the UCRT64 shell). Build the existing source files from this repository, not the obsolete `src/main.c` prototype.

## 🧪 Validate before a real meeting

1. Start `GhostCursor.exe` and confirm that display affinity and hotkeys register successfully.
2. Press `Ctrl + Alt + H`. You should retain a visible local pointer.
3. In a **private test meeting**, share your entire screen and inspect the feed from a *second participant/device*.
4. Press `Ctrl + Shift + Alt + S`, then paste into Paint with `Ctrl + V`. Confirm a BMP was saved in `build/screenshots/`.
5. Press `Ctrl + Alt + R` to restore the system cursor, or `Ctrl + Alt + Q` to exit.

## ⚠️ Important limitations

- **Prototype, not a security or privacy boundary.** Microsoft documents that display affinity is not a DRM or guaranteed content-protection system. Apps using other capture methods may still record overlay pixels.
- System cursor replacement affects Windows globally in the current desktop session. A crash or force-quit can leave the cursor transparent; see [Recovery & safety](docs/RECOVERY.md).
- The local overlay currently reproduces an **arrow pointer**, not every cursor type (I-beam, resize, application-specific cursors).
- Multiple monitors, DPI mixes, fullscreen-exclusive games, remote desktops, and protected apps need more testing.
- Screen captures can contain private information; images stay local unless you share or upload them.
- Hotkeys may conflict with other applications. In the tested build, screenshot uses `Ctrl + Shift + Alt + S` because `Ctrl + Alt + S` was already registered.

## 📁 Project structure

```text
GhostCursor/
├── include/cursor.h       # cursor state API
├── src/cursor.c           # transparent system cursor + restore
├── src/ghost_overlay.cpp  # hotkeys, local overlay, capture exclusion
├── src/screenshot.cpp     # silent screenshot + Windows clipboard
├── docs/RECOVERY.md       # emergency cursor recovery
├── .gitignore             # ignore binaries, screenshots, user data
└── README.md
```

## 🛣️ Roadmap

- [x] Windows cursor toggle and restore
- [x] Local-only pointer overlay
- [x] Private compatibility tests: Google Meet / Teams / Chrome / Zoom
- [x] Silent screenshot to clipboard and BMP
- [ ] System tray and launch-with-Windows mode
- [ ] More cursor shapes and high-DPI / multi-monitor testing
- [ ] Crash watchdog + automatic restoration
- [ ] Automated builds and compatibility test matrix

## 🤝 Contributions

Issues, platform test results, and pull requests are welcome. Please include Windows version, capture application/version, DPI/monitor setup, and reproduction steps. Never attach private screenshots or logs containing personal information.

## 📄 License

MIT — see [LICENSE](LICENSE).
