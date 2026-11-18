#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdint.h>

#define APP_CLASS L"ChillWindowsSimulation"
#define TIMER_ID 1
#define MAX_CARDS 9
#define CARD_W 310
#define CARD_H 132
#define RUN_SECONDS 45.0
#define SPAWN_SECONDS 0.65

typedef struct {
    double x, y, vx, vy, born;
    int alive, origin;
    const wchar_t *message;
    const wchar_t *title;
} Card;

static Card cards[MAX_CARDS];
static int card_count = 0;
static ULONGLONG started_at = 0;
static ULONGLONG last_tick = 0;
static ULONGLONG last_spawn = 0;
static int finished = 0;

static const wchar_t *titles[] = {
    L"Snack subsystem",
    L"Totally real-ish alert",
    L"Important nonsense",
    L"Cat picture service",
    L"Department of silly errors",
    L"Chill.exe"
};
static const wchar_t *messages[] = {
    L"Snack levels are critically delicious. Please remain calm.",
    L"Error 404: Seriousness not found.",
    L"The computer has misplaced its tiny hat.",
    L"Too much chill detected. Cooling fans are vibing.",
    L"Warning: This window contains 100% premium nonsense.",
    L"Cat picture delivery is running fashionably late."
};

static double seconds_since_start(void)
{
    return (GetTickCount64() - started_at) / 1000.0;
}

static void add_card(int origin)
{
    if (card_count >= MAX_CARDS) return;
    RECT rc;
    GetClientRect(GetActiveWindow(), &rc);
    int cw = rc.right - rc.left;
    int ch = rc.bottom - rc.top;
    if (cw < CARD_W + 20 || ch < CARD_H + 20) return;

    unsigned int seed = (unsigned int)GetTickCount64() ^ (unsigned int)(card_count * 2654435761u);
    Card *c = &cards[card_count++];
    c->x = 15 + (seed % (unsigned int)(cw - CARD_W - 15));
    c->y = 70 + ((seed >> 8) % (unsigned int)(ch - CARD_H - 70));
    c->vx = (double)((int)((seed >> 16) % 241) - 120);
    if (c->vx > -55 && c->vx < 55) c->vx = c->vx < 0 ? -90 : 90;
    c->vy = -110.0 - (double)((seed >> 4) % 100);
    c->born = seconds_since_start();
    c->alive = 1;
    c->origin = origin;
    c->title = titles[seed % (sizeof(titles) / sizeof(titles[0]))];
    c->message = messages[(seed >> 12) % (sizeof(messages) / sizeof(messages[0]))];
}

static COLORREF rainbow_color(double hue)
{
    double h = fmod(hue, 360.0) / 60.0;
    double x = 1.0 - fabs(fmod(h, 2.0) - 1.0);
    double r = 0, g = 0, b = 0;
    if (h < 1) { r = 1; g = x; }
    else if (h < 2) { r = x; g = 1; }
    else if (h < 3) { g = 1; b = x; }
    else if (h < 4) { g = x; b = 1; }
    else if (h < 5) { r = x; b = 1; }
    else { r = 1; b = x; }
    /* Pastel colors keep the text easy to read and avoid strobe-like flashes. */
    return RGB((BYTE)(190 + r * 65), (BYTE)(190 + g * 65), (BYTE)(190 + b * 65));
}

static void draw_centered(HDC dc, const wchar_t *text, RECT r, UINT flags)
{
    DrawTextW(dc, text, -1, &r, flags);
}

static void draw_card(HDC dc, const Card *c, double elapsed)
{
    RECT r = { (LONG)c->x, (LONG)c->y, (LONG)c->x + CARD_W, (LONG)c->y + CARD_H };
    HBRUSH white = CreateSolidBrush(RGB(250, 250, 252));
    HBRUSH blue = CreateSolidBrush(RGB(40, 100, 180));
    FillRect(dc, &r, white);
    FrameRect(dc, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));

    RECT header = { r.left + 1, r.top + 1, r.right - 1, r.top + 28 };
    FillRect(dc, &header, blue);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    RECT title_rc = { header.left + 9, header.top + 2, header.right - 7, header.bottom - 1 };
    wchar_t label[128];
    wsprintfW(label, L"SIMULATED ALERT  |  %s%s", c->title, c->origin ? L" (origin)" : L"");
    draw_centered(dc, label, title_rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    SetTextColor(dc, RGB(30, 35, 45));
    RECT msg = { r.left + 13, r.top + 38, r.right - 13, r.bottom - 32 };
    DrawTextW(dc, c->message, -1, &msg, DT_LEFT | DT_WORDBREAK | DT_VCENTER);
    if (!c->origin) {
        int remain = (int)(5.0 - (elapsed - c->born));
        wchar_t timer[64];
        wsprintfW(timer, L"Auto-clears in %d sec", remain < 0 ? 0 : remain);
        RECT foot = { r.left + 12, r.bottom - 24, r.right - 12, r.bottom - 5 };
        SetTextColor(dc, RGB(95, 95, 105));
        draw_centered(dc, timer, foot, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
    DeleteObject(white);
    DeleteObject(blue);
}

static void paint_scene(HWND hwnd, HDC dc, RECT client, double elapsed)
{
    int width = client.right - client.left;
    int height = client.bottom - client.top;

    /* Slow pastel rainbow bands stay inside this app window. */
    int band_h = height / 6 + 1;
    for (int i = 0; i < 6; ++i) {
        RECT band = { 0, i * band_h, width, (i + 1) * band_h };
        HBRUSH brush = CreateSolidBrush(rainbow_color(elapsed * 14.0 + i * 60.0));
        FillRect(dc, &band, brush);
        DeleteObject(brush);
    }

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(35, 35, 45));
    RECT info = { 14, 8, width - 14, 53 };
    wchar_t status[180];
    int stage = elapsed < 5.0 ? 1 : elapsed < 15.0 ? 2 : 3;
    wsprintfW(status, L"CHILL WINDOWS  |  SIMULATION  |  Stage %d  |  %.0f sec left  |  Escape to exit",
              stage, RUN_SECONDS - elapsed);
    draw_centered(dc, status, info, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    /* Rotate only the live scene drawn in this app's client area. No screenshot,
       desktop capture, display-mode change, or system-wide overlay is used. */
    double angle = elapsed < 15.0 ? 0.0 : fmod((elapsed - 15.0) * 24.0, 360.0);
    double radians = angle * 3.14159265358979323846 / 180.0;
    float cs = (float)cos(radians);
    float sn = (float)sin(radians);
    XFORM transform;
    transform.eM11 = cs;
    transform.eM12 = sn;
    transform.eM21 = -sn;
    transform.eM22 = cs;
    transform.eDx = (float)(width * 0.5 - width * 0.5 * cs + height * 0.5 * sn);
    transform.eDy = (float)(height * 0.5 - width * 0.5 * sn - height * 0.5 * cs);

    int old_mode = SetGraphicsMode(dc, GM_ADVANCED);
    XFORM old_xform;
    GetWorldTransform(dc, &old_xform);
    SetWorldTransform(dc, &transform);

    for (int i = 0; i < card_count; ++i) {
        if (cards[i].alive) draw_card(dc, &cards[i], elapsed);
    }

    SetWorldTransform(dc, &old_xform);
    SetGraphicsMode(dc, old_mode);
    (void)hwnd;
}

static void update_scene(HWND hwnd)
{
    double elapsed = seconds_since_start();
    if (elapsed >= RUN_SECONDS) {
        finished = 1;
        KillTimer(hwnd, TIMER_ID);
        InvalidateRect(hwnd, NULL, TRUE);
        return;
    }

    ULONGLONG now = GetTickCount64();
    double dt = (now - last_tick) / 1000.0;
    last_tick = now;
    if (dt > 0.08) dt = 0.08;

    RECT rc;
    GetClientRect(hwnd, &rc);
    double max_x = (rc.right - rc.left) - CARD_W;
    double max_y = (rc.bottom - rc.top) - CARD_H;

    for (int i = 0; i < card_count; ++i) {
        Card *c = &cards[i];
        if (!c->alive) continue;
        if (!c->origin && elapsed - c->born >= 5.0) {
            c->alive = 0;
            continue;
        }

        c->vy += 230.0 * dt;
        c->x += c->vx * dt;
        c->y += c->vy * dt;
        if (c->x < 0) { c->x = 0; c->vx = fabs(c->vx); }
        if (c->x > max_x) { c->x = max_x; c->vx = -fabs(c->vx); }
        if (c->y < 60) { c->y = 60; c->vy = fabs(c->vy); }
        if (c->y > max_y) {
            c->y = max_y;
            c->vy = -fmax(fabs(c->vy), 170.0);
        }
    }

    /* Stage 1: origin. Stage 2: a gentle trail, capped at eight extra cards. */
    if (elapsed >= 5.0 && now - last_spawn >= (ULONGLONG)(SPAWN_SECONDS * 1000.0)) {
        int live = 0;
        for (int i = 0; i < card_count; ++i)
            if (cards[i].alive && !cards[i].origin) ++live;
        if (live < 8) add_card(0);
        last_spawn = now;
    }

    InvalidateRect(hwnd, NULL, FALSE);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        started_at = GetTickCount64();
        last_tick = started_at;
        last_spawn = started_at;
        add_card(1);
        SetTimer(hwnd, TIMER_ID, 33, NULL);
        return 0;
    case WM_TIMER:
        if (wp == TIMER_ID) update_scene(hwnd);
        return 0;
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE) DestroyWindow(hwnd);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);
        double elapsed = seconds_since_start();
        paint_scene(hwnd, dc, client, finished ? RUN_SECONDS : elapsed);
        if (finished) {
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(25, 25, 35));
            RECT done = { 0, client.bottom / 2 - 35, client.right, client.bottom / 2 + 35 };
            draw_centered(dc, L"Simulation complete. Close this window to exit.", done,
                          DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
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
        L"Epilepsy / motion warning: this app slowly shifts rainbow colors and rotates drawings inside its own window. It does not capture or change your desktop. Do not continue if animated colors or motion may affect you. Press Escape at any time to quit.\n\nStart the 45-second simulation?",
        L"Chill Windows — motion warning",
        MB_YESNO | MB_ICONWARNING | MB_SETFOREGROUND
    );
    if (choice != IDYES) return 0;

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = APP_CLASS;
    if (!RegisterClassW(&wc)) return 1;

    HWND hwnd = CreateWindowExW(
        0, APP_CLASS, L"Chill Windows — simulated alerts (Escape to exit)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 980, 680,
        NULL, NULL, instance, NULL
    );
    if (!hwnd) return 1;

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return 0;
}
