# Chill Windows

A short, opt-in Windows visual prank simulation written in C. Its opening epilepsy warning is a genuine Windows `MessageBoxW` dialog. Alert windows use normal Windows title bars and close buttons, and their contents are clearly labeled as simulated.

## What it does

- Shows an epilepsy and motion warning before starting; choose **No** to exit.
- Runs the animation for 45 seconds at most, with **Escape** available to exit sooner.
- Cycles slow pastel rainbow bands inside the main app window.
- Rotates live text inside that window after the opening stage. It does not take screenshots, change display orientation, or cover the desktop.
- Shows one persistent silly alert and a trail of up to eight extra native Windows windows. Extra alerts bounce with gravity and clear after five seconds.
- Closing the origin alert stops further spawns. Closing the main window or pressing **Escape** exits.

The alerts are part of the app, not operating-system error reports. It does not alter files or settings, install itself, start at login, or use the network. Run it only on a device you own or with the owner's permission. Do not continue if animated colors or motion may affect you.

## Download

GitHub Actions builds `prank_demo.exe` on a Windows runner. Download the `prank-demo-windows` artifact from the latest successful build's **Artifacts** section.

## Build locally

From a Visual Studio Developer Command Prompt:

```bat
cl /nologo /W4 /O2 /DUNICODE /D_UNICODE prank_demo.c user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS /out:prank_demo.exe
```
