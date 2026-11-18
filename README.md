# Windows MessageBox prank demo

A tiny, opt-in C demo of genuine Windows system dialogs using the Win32 `MessageBoxW` API.

This is a simulation, not a system alert. It does not alter system settings, touch files, run at startup, or use the network.

## Build

The GitHub Actions workflow builds `prank_demo.exe` on a Windows runner. Download it from the completed workflow run's **Artifacts** section.

To build locally with the Visual Studio Developer Command Prompt:

```bat
cl /nologo /W4 /O2 /DUNICODE /D_UNICODE prank_demo.c user32.lib /link /out:prank_demo.exe
```

Run it only on a device you own or with the owner's permission. It asks before showing any demo dialogs; choose **No** to quit. During the demo, choose **Cancel** on any dialog to stop.
