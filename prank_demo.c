#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#define APP_CLASS L"ChillWindowsHost"
#define ALERT_COUNT 5
#define TIMER_ID 1
#define RUN_MILLISECONDS 45000

typedef struct {
    HWND hwnd;
    WNDPROC old_proc;
    DWORD thread_id;
    double x, y, vx, vy;
    int slot;
} DialogState;

static HWND app_window = NULL;
static DialogState dialogs[ALERT_COUNT];
static ULONGLONG started_at = 0;
static volatile LONG stopping = 0;
static __declspec(thread) DialogState *thread_dialog = NULL;

static void close_all_alerts(void)
{
    InterlockedExchange(&stopping, 1);
    for (int i = 0; i < ALERT_COUNT; ++i) {
        HWND dialog = dialogs[i].hwnd;
        if (dialog) PostMessageW(dialog, WM_CLOSE, 0, 0);
    }
}

static LRESULT CALLBACK moving_dialog_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    DialogState *state = (DialogState *)GetPropW(hwnd, L"ChillWindowsDialogState");
    if (msg == WM_TIMER && wp == TIMER_ID && state && !InterlockedCompareExchange(&stopping, 0, 0)) {
        HWND owner = GetWindow(hwnd, GW_OWNER);
        if (!owner) owner = app_window;
        if (owner && IsWindow(owner)) {
            RECT bounds, wr;
            GetClientRect(owner, &bounds);
            POINT origin = { bounds.left, bounds.top };
            ClientToScreen(owner, &origin);
            GetWindowRect(hwnd, &wr);
            int ww = wr.right - wr.left, wh = wr.bottom - wr.top;
            int max_x = bounds.right - ww;
            int max_y = bounds.bottom - wh;
            if (max_x < 0) max_x = 0;
            if (max_y < 0) max_y = 0;

            state->x += state->vx;
            state->y += state->vy;
            if (state->x < 0) { state->x = 0; state->vx = 3.0; }
            if (state->x > max_x) { state->x = max_x; state->vx = -3.0; }
            if (state->y < 0) { state->y = 0; state->vy = 2.5; }
            if (state->y > max_y) { state->y = max_y; state->vy = -2.5; }

            SetWindowPos(hwnd, NULL, origin.x + (int)state->x, origin.y + (int)state->y,
                         0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        return 0;
    }
    if (msg == WM_KEYDOWN && wp == VK_ESCAPE) {
        close_all_alerts();
        return 0;
    }
    if (msg == WM_NCDESTROY && state) {
        KillTimer(hwnd, TIMER_ID);
        RemovePropW(hwnd, L"ChillWindowsDialogState");
        if (state->hwnd == hwnd) state->hwnd = NULL;
        if (state->old_proc)
            return CallWindowProcW(state->old_proc, hwnd, msg, wp, lp);
    }
    return state && state->old_proc
        ? CallWindowProcW(state->old_proc, hwnd, msg, wp, lp)
        : DefWindowProcW(hwnd, msg, wp, lp);
}

static LRESULT CALLBACK message_box_hook(int code, WPARAM wp, LPARAM lp)
{
    if (code == HCBT_ACTIVATE && thread_dialog) {
        HWND hwnd = (HWND)wp;
        DialogState *state = thread_dialog;
        state->hwnd = hwnd;
        dialogs[state->slot].hwnd = hwnd;

        /* Keep the native MessageBox recognizable as this app's joke. */
        SetWindowTextW(hwnd, L"Chill Windows — goofy prompt");
        SetDlgItemTextW(hwnd, IDOK, L"What?");
        SetDlgItemTextW(hwnd, IDCANCEL, L"OK!");

        RECT bounds, wr;
        GetClientRect(app_window, &bounds);
        POINT origin = { bounds.left, bounds.top };
        ClientToScreen(app_window, &origin);
        GetWindowRect(hwnd, &wr);
        int ww = wr.right - wr.left, wh = wr.bottom - wr.top;
        int usable_w = bounds.right - ww;
        int usable_h = bounds.bottom - wh;
        if (usable_w < 0) usable_w = 0;
        if (usable_h < 0) usable_h = 0;
        state->x = (state->slot * 83) % (usable_w + 1);
        state->y = (state->slot * 57) % (usable_h + 1);
        state->vx = (state->slot % 2) ? -3.0 : 3.0;
        state->vy = (state->slot % 2) ? 2.5 : -2.5;

        SetPropW(hwnd, L"ChillWindowsDialogState", state);
        state->old_proc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                                     (LONG_PTR)moving_dialog_proc);
        SetTimer(hwnd, TIMER_ID, 30, NULL);
        SetWindowPos(hwnd, NULL, origin.x + (int)state->x, origin.y + (int)state->y,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    return CallNextHookEx(NULL, code, wp, lp);
}

static DWORD WINAPI show_alert(LPVOID parameter)
{
    DialogState *state = (DialogState *)parameter;
    state->thread_id = GetCurrentThreadId();
    thread_dialog = state;
    HHOOK hook = SetWindowsHookExW(WH_CBT, message_box_hook, NULL, state->thread_id);
    if (!hook) return 1;

    static const wchar_t *texts[] = {
        L"Snack levels have reached a mathematically silly value.",
        L"Error 404: Seriousness not found. Please try laughing.",
        L"Your keyboard has requested a tiny vacation.",
        L"Warning: excessive chill detected in this window.",
        L"The computer misplaced its hat. It is looking under the desk."
    };
    const wchar_t *body = texts[state->slot];
    MessageBoxW(app_window, body, L"Chill Windows", MB_OKCANCEL | MB_ICONINFORMATION);

    if (state->hwnd && IsWindow(state->hwnd)) {
        KillTimer(state->hwnd, TIMER_ID);
        RemovePropW(state->hwnd, L"ChillWindowsDialogState");
        if (state->old_proc)
            SetWindowLongPtrW(state->hwnd, GWLP_WNDPROC, (LONG_PTR)state->old_proc);
        state->hwnd = NULL;
    }
    dialogs[state->slot].hwnd = NULL;
    UnhookWindowsHookEx(hook);
    thread_dialog = NULL;
    return 0;
}

static void start_alerts(void)
{
    for (int i = 0; i < ALERT_COUNT; ++i) {
        dialogs[i].slot = i;
        HANDLE thread = CreateThread(NULL, 0, show_alert, &dialogs[i], 0, NULL);
        if (thread) CloseHandle(thread);
    }
}

static LRESULT CALLBACK app_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        app_window = hwnd;
        started_at = GetTickCount64();
        SetTimer(hwnd, TIMER_ID, 250, NULL);
        return 0;
    case WM_TIMER:
        if (wp == TIMER_ID && GetTickCount64() - started_at >= RUN_MILLISECONDS)
            close_all_alerts();
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) {
            close_all_alerts();
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_CLOSE:
        close_all_alerts();
        DestroyWindow(hwnd);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(35, 40, 50));
        DrawTextW(dc,
            L"Chill Windows\n\nFive native Windows message boxes are bouncing inside this window.\n"
            L"Choose What? or OK! to dismiss one. Press Escape or close this window to stop all.",
            -1, &rc, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        app_window = NULL;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show)
{
    (void)previous;
    (void)command_line;

    int choice = MessageBoxW(
        NULL,
        L"Epilepsy and motion warning: this app moves several dialog windows. It does not flash, capture the screen, or change display settings. Do not continue if motion may affect you. You can stop by pressing Escape or closing the app window.\n\nStart?",
        L"Chill Windows — warning",
        MB_YESNO | MB_ICONWARNING
    );
    if (choice != IDYES) return 0;

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = app_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = APP_CLASS;
    if (!RegisterClassW(&wc)) return 1;

    HWND hwnd = CreateWindowExW(
        0, APP_CLASS, L"Chill Windows",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 820, 540,
        NULL, NULL, instance, NULL
    );
    if (!hwnd) return 1;
    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);
    start_alerts();

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    close_all_alerts();
    return 0;
}
