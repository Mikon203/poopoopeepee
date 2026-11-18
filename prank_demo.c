#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wingdi.h>
#include <stdint.h>

#ifndef DMDO_0
#define DMDO_0 0
#define DMDO_90 1
#define DMDO_180 2
#define DMDO_270 3
#endif

#define TRAIL_MAX 35

typedef struct {
    HWND hwnd;
} TrailNode;

static TrailNode trail[TRAIL_MAX];
static int trail_index = 0;
static HWND main_hwnd = NULL;
static volatile LONG running = 1;

static void flip_screen(int orientation) {
#ifndef __WINE__
    DEVMODEW dm = {0};
    dm.dmSize = sizeof(dm);
    if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm)) {
        if ((orientation == DMDO_90 || orientation == DMDO_270) && 
            (dm.dmDisplayOrientation == DMDO_0 || dm.dmDisplayOrientation == DMDO_180)) {
            DWORD temp = dm.dmPelsWidth;
            dm.dmPelsWidth = dm.dmPelsHeight;
            dm.dmPelsHeight = temp;
        } else if ((orientation == DMDO_0 || orientation == DMDO_180) && 
                   (dm.dmDisplayOrientation == DMDO_90 || dm.dmDisplayOrientation == DMDO_270)) {
            DWORD temp = dm.dmPelsWidth;
            dm.dmPelsWidth = dm.dmPelsHeight;
            dm.dmPelsHeight = temp;
        }
        dm.dmDisplayOrientation = orientation;
        dm.dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT;
        ChangeDisplaySettingsExW(NULL, &dm, NULL, CDS_RESET, NULL);
    }
#else
    (void)orientation;
#endif
}

static LRESULT CALLBACK PrankWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_CLOSE || (msg == WM_KEYDOWN && wp == VK_ESCAPE)) {
        InterlockedExchange(&running, 0);
        flip_screen(DMDO_0);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// Background thread handles physics & trail creation safely away from the message loop
static DWORD WINAPI PhysicsThreadProc(LPVOID lpParam)
{
    (void)lpParam;
    
    // Wait for main window to initialize
    while (!main_hwnd && running) {
        Sleep(10);
    }

    double pos_x = 150.0, pos_y = 50.0;
    double vel_x = 7.0, vel_y = 0.0;
    const double gravity = 0.5;
    const double bounce_damping = -1.0; // Perpetual bounce

    while (running) {
        int screen_w = GetSystemMetrics(SM_CXSCREEN);
        int screen_h = GetSystemMetrics(SM_CYSCREEN);
        int win_w = 240, win_h = 80;

        vel_y += gravity;
        pos_x += vel_x;
        pos_y += vel_y;

        if (pos_y >= (screen_h - win_h)) {
            pos_y = (double)(screen_h - win_h);
            vel_y *= bounce_damping;
        }
        if (pos_y <= 0) {
            pos_y = 0;
            vel_y *= bounce_damping;
        }

        if (pos_x >= (screen_w - win_w) || pos_x <= 0) {
            vel_x *= -1.0;
            if (pos_x < 0) pos_x = 0;
            if (pos_x > (screen_w - win_w)) pos_x = (double)(screen_w - win_w);
        }

        if (main_hwnd && IsWindow(main_hwnd)) {
            SetWindowPos(main_hwnd, NULL, (int)pos_x, (int)pos_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(main_hwnd, GWLP_HINSTANCE);
            HWND ghost = CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                L"RealBoxClass",
                L"Perpetual Trail",
                WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                (int)pos_x, (int)pos_y,
                win_w, win_h,
                NULL, NULL, hInstance, NULL
            );

            if (ghost) {
                CreateWindowExW(
                    0, L"STATIC", L"Never stops bouncing!",
                    WS_CHILD | WS_VISIBLE | SS_CENTER,
                    10, 18, 220, 25,
                    ghost, NULL, hInstance, NULL
                );

                if (trail[trail_index].hwnd && IsWindow(trail[trail_index].hwnd)) {
                    DestroyWindow(trail[trail_index].hwnd);
                }
                trail[trail_index].hwnd = ghost;
                trail_index = (trail_index + 1) % TRAIL_MAX;
            }
        }

        Sleep(15);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = PrankWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"RealBoxClass";
    if (!RegisterClassW(&wc)) return 1;

    // Flip screen display on startup
    flip_screen(DMDO_180);

    main_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        L"RealBoxClass",
        L"Chill Windows — Perpetual Trail",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        150, 50,
        240, 80,
        NULL, NULL, hInstance, NULL
    );

    if (!main_hwnd) return 1;

    CreateWindowExW(
        0, L"STATIC", L"Perpetual bounce active!\nPress ESC to exit.",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 18, 220, 30,
        main_hwnd, NULL, hInstance, NULL
    );

    // Start background physics thread safely
    HANDLE hThread = CreateThread(NULL, 0, PhysicsThreadProc, NULL, 0, NULL);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    InterlockedExchange(&running, 0);
    if (hThread) {
        WaitForSingleObject(hThread, 1000);
        CloseHandle(hThread);
    }
    
    for (int i = 0; i < TRAIL_MAX; ++i) {
        if (trail[i].hwnd && IsWindow(trail[i].hwnd)) {
            DestroyWindow(trail[i].hwnd);
        }
    }

    return 0;
}
