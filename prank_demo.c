#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

#define ALERT_COUNT 6
#define RUN_MILLISECONDS 30000

typedef struct {
    HWND hwnd;
    WNDPROC old_proc;
    DWORD thread_id;
    HANDLE worker;
    double x, y, vx, vy;
    int slot;
} DialogState;

static DialogState dialogs[ALERT_COUNT];
static ULONGLONG started_at = 0;
static volatile LONG stopping = 0;
static __declspec(thread) DialogState *thread_dialog = NULL;

static void close_all_alerts(void)
{
    if (InterlockedCompareExchange(&stopping, 1, 1) == 1) return;
    InterlockedExchange(&stopping, 1);

    for (int i = 0; i < ALERT_COUNT; ++i) {
        HWND dialog = dialogs[i].hwnd;
        if (dialog && IsWindow(dialog)) {
            PostMessageW(dialog, WM_CLOSE, 0, 0);
        }
    }
}

static LRESULT CALLBACK moving_dialog_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    DialogState *state = (DialogState *)GetPropW(hwnd, L"ChillWindowsDialogState");
    if (msg == WM_KEYDOWN && wp == VK_ESCAPE) {
        close_all_alerts();
        return 0;
    }
    if (msg == WM_NCDESTROY && state) {
        RemovePropW(hwnd, L"ChillWindowsDialogState");
        if (state->hwnd == hwnd) state->hwnd = NULL;
    }
    return state && state->old_proc
        ? CallWindowProcW(state->old_proc, hwnd, msg, wp, lp)
        : DefWindowProcW(hwnd, msg, wp, lp);
}

static DWORD WINAPI dialog_motion_worker(LPVOID parameter)
{
    DialogState *state = (DialogState *)parameter;
    
    while (!InterlockedCompareExchange(&stopping, 0, 0)) {
        HWND dialog = state->hwnd;
        if (!dialog || !IsWindow(dialog)) break;

        RECT wr;
        GetWindowRect(dialog, &wr);
        int ww = wr.right - wr.left;
        int wh = wr.bottom - wr.top;
        
        int max_x = GetSystemMetrics(SM_CXSCREEN) - ww;
        int max_y = GetSystemMetrics(SM_CYSCREEN) - wh;
        if (max_x < 0) max_x = 0;
        if (max_y < 0) max_y = 0;

        state->x += state->vx;
        state->y += state->vy;
        
        if (state->x < 0) { state->x = 0; state->vx = 4.5; }
        if (state->x > max_x) { state->x = max_x; state->vx = -4.5; }
        if (state->y < 0) { state->y = 0; state->vy = 3.8; }
        if (state->y > max_y) { state->y = max_y; state->vy = -3.8; }

        SetWindowPos(dialog, NULL, (int)state->x, (int)state->y,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        Sleep(15);
    }
    return 0;
}

static LRESULT CALLBACK message_box_hook(int code, WPARAM wp, LPARAM lp)
{
    if (code == HCBT_ACTIVATE && thread_dialog) {
        HWND hwnd = (HWND)wp;
        DialogState *state = thread_dialog;
        state->hwnd = hwnd;
        dialogs[state->slot].hwnd = hwnd;

        SetWindowTextW(hwnd, L"Chill Windows — Prank Active");
        SetDlgItemTextW(hwnd, IDOK, L"What?");
        SetDlgItemTextW(hwnd, IDCANCEL, L"Stop!");

        RECT wr;
        GetWindowRect(hwnd, &wr);
        int ww = wr.right - wr.left;
        int wh = wr.bottom - wr.top;
        int screen_w = GetSystemMetrics(SM_CXSCREEN) - ww;
        int screen_h = GetSystemMetrics(SM_CYSCREEN) - wh;
        if (screen_w < 0) screen_w = 0;
        if (screen_h < 0) screen_h = 0;

        state->x = (double)((state->slot * 200) % (screen_w > 0 ? screen_w : 1));
        state->y = (double)((state->slot * 150) % (screen_h > 0 ? screen_h : 1));
        state->vx = (state->slot % 2 == 0) ? 4.5 : -4.5;
        state->vy = (state->slot % 2 == 0) ? 3.8 : -3.8;

        SetPropW(hwnd, L"ChillWindowsDialogState", state);
        state->old_proc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)moving_dialog_proc);
        state->worker = CreateThread(NULL, 0, dialog_motion_worker, state, 0, NULL);
        
        SetWindowPos(hwnd, NULL, (int)state->x, (int)state->y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
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
        L"Error 404: Seriousness not found.",
        L"Your keyboard has requested a tiny vacation.",
        L"Warning: excessive chill detected.",
        L"This is completely harmless, but look at it go!",
        L"Bouncy warning boxes deployed successfully!",
        L"Press Escape or wait a bit to stop."
    };
    
    MessageBoxW(NULL, texts[state->slot], L"Chill Windows", MB_OKCANCEL | MB_ICONINFORMATION);

    if (state->worker) {
        WaitForSingleObject(state->worker, 1500);
        CloseHandle(state->worker);
        state->worker = NULL;
    }
    if (state->hwnd && IsWindow(state->hwnd)) {
        RemovePropW(state->hwnd, L"ChillWindowsDialogState");
        state->hwnd = NULL;
    }
    dialogs[state->slot].hwnd = NULL;
    UnhookWindowsHookEx(hook);
    thread_dialog = NULL;
    return 0;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show)
{
    (void)instance; (void)previous; (void)command_line; (void)show;

    int choice = MessageBoxW(
        NULL,
        L"Motion Warning: This program bounces warning boxes across your entire desktop. Press Escape at any time to abort.\n\nReady to launch?",
        L"Chill Windows — Warning",
        MB_YESNO | MB_ICONWARNING
    );
    if (choice != IDYES) return 0;

    started_at = GetTickCount64();

    // Spawn the bouncing alert threads across the desktop
    for (int i = 0; i < ALERT_COUNT; ++i) {
        dialogs[i].slot = i;
        HANDLE thread = CreateThread(NULL, 0, show_alert, &dialogs[i], 0, NULL);
        if (thread) CloseHandle(thread);
    }

    // Run main timer loop
    while (!InterlockedCompareExchange(&stopping, 0, 0)) {
        if (GetTickCount64() - started_at >= RUN_MILLISECONDS) {
            close_all_alerts();
            break;
        }
        Sleep(100);
    }

    return 0;
}
