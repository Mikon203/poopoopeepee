#define UNICODE
#define _UNICODE
#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show)
{
    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show;

    int answer = MessageBoxW(
        NULL,
        L"This is a harmless demo of genuine Windows dialogs. It will show at most five labeled demo prompts. Choose No to exit now.",
        L"MessageBox Demo — consent",
        MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_TOPMOST
    );

    if (answer != IDYES) {
        return 0;
    }

    static const wchar_t *messages[] = {
        L"DEMO 1/5: The computer has detected an unusually high level of silliness.",
        L"DEMO 2/5: Critical snack levels are low. This is just a joke.",
        L"DEMO 3/5: The 'any' key has been located. It was hiding near the keyboard.",
        L"DEMO 4/5: Everything is fine. This message is part of the demo.",
        L"DEMO 5/5: Demo complete. No changes were made to your computer."
    };

    for (int i = 0; i < (int)(sizeof(messages) / sizeof(messages[0])); ++i) {
        answer = MessageBoxW(
            NULL,
            messages[i],
            L"MessageBox Demo — simulation",
            MB_OKCANCEL | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST
        );

        if (answer == IDCANCEL) {
            break;
        }
    }

    MessageBoxW(
        NULL,
        L"The demo has ended. Thanks for trying it!",
        L"MessageBox Demo — finished",
        MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND
    );

    return 0;
}
