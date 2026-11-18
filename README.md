# Chill Windows

A short, opt-in Windows joke program written in C. It uses genuine Win32 `MessageBoxW` dialogs for its warning and alerts.

## What it does

- Shows an epilepsy and motion warning before starting; choose **No** to exit.
- Opens at most five native message boxes. Their buttons read **What?** and **OK!**.
- Moves the dialogs inside the app window. Choose either button to dismiss a dialog.
- Press **Escape** or close the main app window to stop the alerts. The run is limited to 45 seconds.
- Uses no desktop capture, fullscreen overlay, display-orientation change, file changes, startup persistence, or network access.

The messages are jokes from this app, not Windows system error reports. Run it only on a device you own or with the owner's permission. Do not continue if motion may affect you.

## Download

GitHub Actions builds `prank_demo.exe` on a Windows runner. Download the `prank-demo-windows` artifact from the latest successful build's **Artifacts** section.

## Build locally

From a Visual Studio Developer Command Prompt:

```bat
cl /nologo /W4 /O2 /DUNICODE /D_UNICODE prank_demo.c user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS /out:prank_demo.exe
```
