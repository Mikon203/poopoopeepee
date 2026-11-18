#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <stdint.h>

#define APP_CLASS L"ChillWindowsSimulation"
#define ALERT_CLASS L"ChillWindowsAlert"
#define TIMER_ID 1
#define MAX_CARDS 9
#define CARD_W 310
#define CARD_H 132
#define RUN_SECONDS 45.0
#define SPAWN_SECONDS 0.65

typedef struct {
    double x, y, vx, vy, born;
    int alive, origin;
    HWND hwnd;
    const wchar_t *message;
    const wchar_t *title;
} Card;

static Card cards[MAX_CARDS];
static int card_count = 0;
static ULONGLONG started_at = 0;
static ULONGLONG last_tick = 0;
static ULONGLONG last_spawn = 0;
static int finished = 0;
static int origin_closed = 0;
static HWND main_hwnd = NULL;

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

static LRESULT CALLBACK alert_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Card *card = (Card *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)lp;
        card = (Card *)cs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)card);
    }
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(RGB(250, 250, 252));
        FillRect(dc, &rc, bg);
        DeleteObject(bg);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(45, 78, 125));
        RECT label = { 14, 10, rc.right - 14, 34 };
        draw_centered(dc, L"SIMULATED ALERT — JUST FOR FUN", label,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SetTextColor(dc, RGB(30, 35, 45));
        RECT body = { 14, 40, rc.right - 14, rc.bottom - 12 };
        if (card && card->message)
            DrawTextW(dc, card->message, -1, &body, DT_LEFT | DT_WORDBREAK | DT_VCENTER);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
        if (wp == VK_ESCAPE && main_hwnd) DestroyWindow(main_hwnd);
        return 0;
    case WM_CLOSE:
        if (card) {
            card->alive = 0;
            card->hwnd = NULL;
            if (card->origin) origin_closed = 1;
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (card && card->hwnd == hwnd) {
            card->alive = 0;
            card->hwnd = NULL;
            if (card->origin) origin_closed = 1;
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static void add_card(HWND hwnd, int origin)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    int cw = rc.right - rc.left;
    int ch = rc.bottom - rc.top;
    if (cw < CARD_W + 20 || ch < CARD_H + 80) return;

    int slot = -1;
    for (int i = 0; i < card_count; ++i) {
        if (!cards[i].alive) { slot = i; break; }
    }
    if (slot < 0 && card_count < MAX_CARDS) slot = card_count++;
    if (slot < 0) return;

    unsigned int seed = (unsigned int)GetTickCount64() ^ (unsigned int)(slot * 2654435761u);
    Card *c = &cards[slot];
    c->x = 10 + (seed % (unsigned int)(cw - CARD_W - 10));
    c->y = 70 + ((seed >> 8) % (unsigned int)(ch - CARD_H - 70));
    c->vx = (double)((int)((seed >> 16) % 241) - 120);
    if (c->vx > -55 && c->vx < 55) c->vx = c->vx < 0 ? -90 : 90;
    c->vy = -110.0 - (double)((seed >> 4) % 100);
    c->born = seconds_since_start();
    c->alive = 1;
    c->origin = origin;
    c->title = titles[seed % (sizeof(titles) / sizeof(titles[0]))];
    c->message = messages[(seed >> 12) % (sizeof(messages) / sizeof(messages[0]))];
    c->hwnd = NULL;

    POINT origin_pt = { 0, 0 };
    ClientToScreen(hwnd, &origin_pt);
    wchar_t caption[160];
    wsprintfW(caption, L"Chill Windows — simulated: %s", c->title);
    c->hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        ALERT_CLASS,
        caption,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        origin_pt.x + (int)c->x,
        origin_pt.y + (int)c->y,
        CARD_W, CARD_H,
        hwnd, NULL, GetModuleHandleW(NULL), c
    );
    if (!c->hwnd) {
        c->alive = 0;
        return;
    }
    ShowWindow(c->hwnd, SW_SHOWNOACTIVATE);
    SetWindowPos(c->hwnd, HWND_TOP, origin_pt.x + (int)c->x, origin_pt.y + (int)c->y,
                 CARD_W, CARD_H, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    UpdateWindow(c->hwnd);
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

static void paint_scene(HWND hwnd, HDC dc, RECT client, double elapsed)
{
    int width = client.right - client.left;
    int height = client.bottom - client.top;

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

    /* Rotate live text in this app window; floating alerts use normal Windows chrome. */
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
    SetTextColor(dc, RGB(40, 40, 55));
    HFONT font = CreateFontW(42, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HGDIOBJ old_font = SelectObject(dc, font);
    RECT logo = { width / 2 - 230, height / 2 - 60, width / 2 + 230, height / 2 + 60 };
    draw_centered(dc, L"CHILL WINDOWS", logo, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, old_font);
    DeleteObject(font);
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
    POINT client_origin = { 0, 0 };
    ClientToScreen(hwnd, &client_origin);

    for (int i = 0; i < card_count; ++i) {
        Card *c = &cards[i];
        if (!c->alive || !c->hwnd) continue;
        if (!c->origin && elapsed - c->born >= 5.0) {
            c->alive = 0;
            DestroyWindow(c->hwnd);
            c->hwnd = NULL;
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
        SetWindowPos(c->hwnd, HWND_TOP, client_origin.x + (int)c->x, client_origin.y + (int)c->y,
                     CARD_W, CARD_H, SWP_NOACTIVATE);
    }

    /* Stage 1: origin only. Stage 2: gentle trail capped at eight extra windows. */
    if (!origin_closed && elapsed >= 5.0 &&
        now - last_spawn >= (ULONGLONG)(SPAWN_SECONDS * 1000.0)) {
        int live = 0;
        for (int i = 0; i < card_count; ++i)
            if (cards[i].alive && !cards[i].origin) ++live;
        if (live < 8) add_card(hwnd, 0);
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
        main_hwnd = hwnd;
        add_card(hwnd, 1);
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
        main_hwnd = NULL;
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

    WNDCLASSW alert_wc = {0};
    alert_wc.lpfnWndProc = alert_proc;
    alert_wc.hInstance = instance;
    alert_wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    alert_wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    alert_wc.lpszClassName = ALERT_CLASS;
    if (!RegisterClassW(&alert_wc)) return 1;

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
