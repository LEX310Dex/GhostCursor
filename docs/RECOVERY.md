# Recovery and safety

GhostCursor temporarily replaces standard Windows system cursors with transparent ones. **Always exit with `Ctrl + Alt + Q` or restore with `Ctrl + Alt + R`.** Do not force-kill an active session unless you can recover the cursor separately.

## Restore after a crash

Open PowerShell (keyboard navigation is possible even with an invisible cursor) and run:

```powershell
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class CursorRecovery {
    [DllImport("user32.dll", SetLastError=true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool SystemParametersInfo(
        uint action, uint param, IntPtr data, uint flags);
}
'@
[CursorRecovery]::SystemParametersInfo(0x57, 0, [IntPtr]::Zero, 0)
```

`0x57` is `SPI_SETCURSORS`: reload the current configured system cursor scheme. A `True` result means Windows accepted the request. If it fails, signing out and in may help.

## Screenshots and privacy

The screenshot feature copies the desktop image to the clipboard and writes a `.bmp` file to `build/screenshots/`. Those files are ignored by Git; **do not add them manually to commits**. Confirm no sensitive content is visible before sharing screenshot files.

## Capture guarantees

`WDA_EXCLUDEFROMCAPTURE` requests exclusion only for supported capture mechanisms. It must not be treated as an anti-recording, confidentiality, or security measure. Test every target app from a separate viewer account/device.
