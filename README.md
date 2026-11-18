# Chill Windows

A short, opt-in Windows visual prank simulation written in C. It uses genuine Win32 APIs for its opening warning and draws the animated scene live inside its own normal app window.

## What it does

- Shows an epilepsy and motion warning before starting; choose **No** to exit.
- Runs for 45 seconds at most, with **Escape** available to exit sooner.
- Cycles slow pastel rainbow bands inside the app window.
- Rotates the live drawing inside that window after the opening stage. It does not take screenshots, change display orientation, or cover the desktop.
- Shows one persistent silly alert and a trail of up to eight extra simulated alerts. Extra alerts bounce with gravity and clear after five seconds.
- Labels every fake alert as **SIMULATED ALERT**.

It does not alter files or settings, install itself, start at login, or use the network. Run it only on a device you own or with the owner's permission. Do not continue if animated colors or motion may affect you.

## Download

GitHub Actions builds `prank_demo.exe` on a Windows runner. Download the `prank-demo-windows` artifact from the latest successful build's **Artifacts** section.

## Build locally

From a Visual Studio Developer Command Prompt:

```bat
cl /nologo /W4 /O2 /DUNICODE /D_UNICODE prank_demo.c user32.lib gdi32.lib /link /SUBSYSTEM:WINDOWS /out:prank_demo.exe
```
