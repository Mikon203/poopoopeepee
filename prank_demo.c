#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#define TRAIL_MAX 8
#define TIMER_PHYSICS 1
#define TIMER_LIFETIME 2

typedef struct {
    HWND hwnd;
    double x, y;
} TrailNode;

static TrailNode trail[TRAIL_MAX];
static int trail_index = 0;
static HWND main_hwnd = NULL;
static double pos_x = 100.0, pos_y = 100.0;
static double vel_x = 6.0, vel_y = 0.0;
static const double gravity = 0.6;
static const double bounce_damping = -0.8;

// Display flipping routine (safe for GitHub Actions compilation)
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
    (void)orientation; // Wine sandbox placeholder
#endif
}

static LRESULT CALLBACK TrailWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_CLOSE || (msg == WM_KEYDOWN && wp == VK_ESCAPE)) {
        flip_screen(DMDO_0); // Restore orientation on exit
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static VOID CALLBACK PhysicsTimerProc(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
    (void)hwnd; (void)msg; (void)idEvent; (void)dwTime;

    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int win_w = 280, win_h = 100;

    // Apply gravity and velocity
    vel_y += gravity;
    pos_x += vel_x;
    pos_y += vel_y;

    // Bounce off floor/ceiling
    if (pos_y >= (screen_h - win_h)) {
        pos_y = (double)(screen_h - win_h);
        vel_y *= bounce_damping;
    }
    if (pos_y <= 0) {
        pos_y = 0;
        vel_y *= bounce_damping;
    }

    // Bounce off walls
    if (pos_x >= (screen_w - win_w) || pos_x <= 0) {
        vel_x *= -1.0;
        if (pos_x < 0) pos_x = 0;
        if (pos_x > (screen_w - win_w)) pos_x = (double)(screen_w - win_w);
    }

    // Move main window
    SetWindowPos(main_hwnd, NULL, (int)pos_x, (int)pos_y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Spawn trail copy window
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(main_hwnd, GWLP_HINSTANCE);
    HWND ghost = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        L"TrailPrankClass",
        L"Ghost",
        WS_POPUP | WS_VISIBLE,
        (int)pos_x, (int)pos_y,
        win_w, win_h,
        NULL, NULL, hInstance, NULL
    );
    
    // Make trail windows semi-transparent
    SetLayeredWindowAttributes(ghost, 0, 150, LWA_ALPHA);

    // Manage trail memory rotation and old ghost destruction
    if (trail[trail_index].hwnd && IsWindow(trail[trail_index].hwnd)) {
        DestroyWindow(trail[trail_index].hwnd);
    }
    trail[trail_index].hwnd = ghost;
    trail[trail_index].x = pos_x;
    trail[trail_index].y = pos_y;
    
    trail_index = (trail_index + 1) % TRAIL_MAX;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = TrailWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_HIGHLIGHT + 1);
    wc.lpszClassName = L"TrailPrankClass";
    if (!RegisterClassW(&wc)) return 1;

    // Flip screen orientation on start (if running on a native Windows machine)
    flip_screen(DMDO_180);

    main_hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        L"TrailPrankClass",
        L"Chill Windows — Gravity Trail",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        (int)pos_x, (int)pos_y,
        280, 100,
        NULL, NULL, hInstance, NULL
    );

    if (!main_hwnd) return 1;

    CreateWindowExW(
        0, L"STATIC", L"Bouncing with gravity & trail!\nPress ESC to restore.",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 25, 260, 45,
        main_hwnd, NULL, hInstance, NULL
    );

    SetTimer(main_hwnd, TIMER_PHYSICS, 15, PhysicsTimerProc);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    KillTimer(main_hwnd, TIMER_PHYSICS);
    return 0;
}
