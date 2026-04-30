/*
 * EyeBreak - 定时护眼提醒工具
 *
 * 每 20 分钟通过 Windows 托盘气泡通知提醒用户休息。
 * 锁屏时暂停并重置计时，解锁后恢复计时。
 * 支持开机自启（HKCU\Run）、单实例运行、中英文界面切换。
 *
 * Build:
 *   rc.exe /I include res\resources.rc
 *   cl.exe /MT /O2 /W3 /utf-8 /I include /Fe:dist\EyeBreak.exe src\eye_break.c build\resources.res
 *       /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib advapi32.lib ole32.lib wtsapi32.lib
 *
 * Debug (with file logging):
 *   Add /DEYEBREAK_DEBUG to cl.exe flags
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wtsapi32.h>
#include <stdio.h>
#include <stdarg.h>
#include "resources.h"

/* ----------------------------------------------------------------
 *  Compile-time switches
 * ---------------------------------------------------------------- */

/* Uncomment to enable debug logging to eyebreak.log */
/* #define EYEBREAK_DEBUG */

/* ----------------------------------------------------------------
 *  Constants
 * ---------------------------------------------------------------- */

/* Menu command IDs */
#define IDM_EXIT           1001
#define IDM_TEST_NOTIFY    1002
#define IDM_STOP_REMINDER  1003
#define IDM_START_REMINDER 1004
#define IDM_AUTO_START     1005
#define IDM_NO_AUTO_START  1006
#define IDM_LANG_SWITCH    1007

/* Timer IDs */
#define TIMER_ID_MAIN      1
#define TIMER_ID_AUTO_CHECK 2

/* Timeouts (ms) */
#define AUTO_CHECK_DELAY_MS  2000

/* Reminder interval: 20 minutes in seconds */
#define REMINDER_SECONDS    (20 * 60)

/* Custom window messages */
#define WM_USER_TRAY       (WM_USER + 1)

/* WTS session notifications - guard for older SDKs */
#ifndef WM_WTSSESSION_CHANGE
#define WM_WTSSESSION_CHANGE 0x02B1
#endif
#ifndef WTS_SESSION_LOCK
#define WTS_SESSION_LOCK    0x7
#endif
#ifndef WTS_SESSION_UNLOCK
#define WTS_SESSION_UNLOCK  0x8
#endif

/* Strings */
#define APP_NAME           L"EyeBreak"
#define TOOLTIP_TEXT       L"EyeBreak"
#define MUTEX_NAME         L"EyeBreak_SingleInstance_Mutex"
#define LOG_FILENAME       L"log\\eyebreak.log"
#define WINDOW_CLASS       L"EyeBreakClass"

/* Registry */
#define REG_RUN_KEY        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define REG_VAL_NAME       L"EyeBreak"
#define REG_SETTINGS_KEY   L"Software\\EyeBreak"
#define REG_VAL_LANG       L"Language"

/* ----------------------------------------------------------------
 *  Language system
 *
 *  All user-visible strings are indexed into two tables (CN / EN).
 *  S(id) macro selects the current language string.
 *  Language preference persists in HKCU\Software\EyeBreak\Language.
 * ---------------------------------------------------------------- */

enum LangStringID {
    S_MENU_TEST,
    S_MENU_STOP,
    S_MENU_START,
    S_MENU_AUTOSTART_ON,
    S_MENU_AUTOSTART_OFF,
    S_MENU_EXIT,
    S_MENU_LANG_EN,          /* "Switch to English" (shown when in CN mode) */
    S_MENU_LANG_CN,          /* "切换到中文" (shown when in EN mode) */
    S_TIP_STOPPED,
    S_NOTIFY_TITLE,
    S_NOTIFY_TEXT_CN,
    S_NOTIFY_TEXT_EN,
    S_ERR_MUTEX,
    S_ERR_DUP,
    S_ERR_REGCLASS,
    S_ERR_CREATEWND,
    S_ERR_TRAY,
    S_ERR_REGOPEN,
    S_ERR_REGWRITE,
    S_WARN_INTERCEPTED_CN,
    S_WARN_INTERCEPTED_EN,
    S__COUNT
};

static const WCHAR *g_szCN[S__COUNT] = {
    L"发送测试通知",                          /* S_MENU_TEST      */
    L"停止提醒",                              /* S_MENU_STOP       */
    L"开始提醒",                              /* S_MENU_START      */
    L"✓ 开机自启",                            /* S_MENU_AUTOSTART_ON*/
    L"开机自启",                              /* S_MENU_AUTOSTART_OFF*/
    L"退出",                                  /* S_MENU_EXIT       */
    L"Switch to English",                     /* S_MENU_LANG_EN    */
    L"切换到中文",                            /* S_MENU_LANG_CN    */
    L"EyeBreak - 已停止",                      /* S_TIP_STOPPED     */
    L"EyeBreak",                              /* S_NOTIFY_TITLE    */
    L"已连续用眼 20 分钟。\n\n请：① 远眺 20 秒 ② 用力紧闭双眼 10 秒", /* S_NOTIFY_TEXT_CN  */
    L"You've been using the screen for 20 min.\n\nPlease: ① Look 20 ft away for 20 sec ② Close eyes firmly for 10 sec", /* S_NOTIFY_TEXT_EN */
    L"无法创建单实例锁。",                      /* S_ERR_MUTEX       */
    L"EyeBreak 正在运行。\n请先从系统托盘退出当前实例。", /* S_ERR_DUP        */
    L"注册窗口类失败。",                        /* S_ERR_REGCLASS    */
    L"创建隐藏窗口失败。",                      /* S_ERR_CREATEWND   */
    L"无法添加托盘图标。",                      /* S_ERR_TRAY        */
    L"无法写入注册表。\n\n可能原因：\n• 安全软件拦截\n• 权限不足\n\n请在安全软件中将 EyeBreak 加入信任列表。", /* S_ERR_REGOPEN */
    L"开机自启写入失败。\n请检查安全软件是否拦截了注册表修改。", /* S_ERR_REGWRITE */
    L"开机自启设置未能生效。\n\n"
    L"检测到注册表启动项在写入后被自动移除，\n"
    L"这通常是由安全软件（如联想电脑管家、360、火绒等）的\n"
    L"「启动项管理」功能导致的。\n\n"
    L"建议操作：\n"
    L"1. 打开安全软件，找到「启动项管理」或「开机加速」\n"
    L"2. 将 EyeBreak 设为「允许启动」或加入信任列表\n"
    L"3. 返回右键菜单重新开启开机自启",            /* S_WARN_INTERCEPTED_CN */
    L"Auto-start was not applied.\n\n"
    L"The registry entry was removed after being written.\n"
    L"This is typically caused by security software\n"
    L"(Lenovo PC Manager, 360, Huorong, etc.) managing startup items.\n\n"
    L"To fix:\n"
    L"1. Open your security software\n"
    L"2. Find Startup Manager / Boot Accelerator\n"
    L"3. Add EyeBreak to the allowed list\n"
    L"4. Re-enable auto-start from the tray menu"    /* S_WARN_INTERCEPTED_EN */
};

static const WCHAR *g_szEN[S__COUNT] = {
    L"Test Notification",
    L"Stop Reminder",
    L"Start Reminder",
    L"* Auto-start (ON)",
    L"Auto-start (OFF)",
    L"Exit",
    L"切换到中文",
    L"Switch to English",
    L"EyeBreak - Stopped",
    L"EyeBreak",
    L"已连续用眼 20 分钟。\n\n请：① 远眺 20 秒 ② 用力紧闭双眼 10 秒",
    L"You've been using the screen for 20 min.\n\nPlease: ① Look 20 ft away for 20 sec ② Close eyes firmly for 10 sec",
    L"Failed to create mutex.",
    L"EyeBreak is already running.\nExit the existing instance from the system tray first.",
    L"RegisterClass failed.",
    L"CreateWindow failed.",
    L"Failed to add tray icon.",
    L"Cannot open registry for writing.\n\n"
    L"Possible cause:\n- Security software blocking registry access\n"
    L"- Insufficient permissions\n\n"
    L"Try adding EyeBreak to your security software trust list.",
    L"Failed to write auto-start registry value.\n"
    L"Check if security software is blocking registry changes.",
    L"开机自启设置未能生效。\n\n"
    L"检测到注册表启动项在写入后被自动移除，\n"
    L"这通常是由安全软件（如联想电脑管家、360、火绒等）的\n"
    L"「启动项管理」功能导致的。\n\n"
    L"建议操作：\n"
    L"1. 打开安全软件，找到「启动项管理」或「开机加速」\n"
    L"2. 将 EyeBreak 设为「允许启动」或加入信任列表\n"
    L"3. 返回右键菜单重新开启开机自启",
    L"Auto-start was not applied.\n\n"
    L"The registry entry was removed after being written.\n"
    L"This is typically caused by security software\n"
    L"(Lenovo PC Manager, 360, Huorong, etc.) managing startup items.\n\n"
    L"To fix:\n"
    L"1. Open your security software\n"
    L"2. Find Startup Manager / Boot Accelerator\n"
    L"3. Add EyeBreak to the allowed list\n"
    L"4. Re-enable auto-start from the tray menu"
};

/* Select string by current language */
#define S(id)  (g_bLangEnglish ? g_szEN[id] : g_szCN[id])

/* ----------------------------------------------------------------
 *  Globals
 * ---------------------------------------------------------------- */

static HINSTANCE          g_hInst            = NULL;
static HWND                g_hWnd             = NULL;
static HICON               g_hIcon            = NULL;
static NOTIFYICONDATAW     g_nid              = { 0 };
static BOOL                g_bReminderEnabled = TRUE;
static BOOL                g_bAutoStart       = FALSE;
static BOOL                g_bPendingAutoCheck = FALSE;
static BOOL                g_bLangEnglish     = FALSE;  /* Default: Chinese */
static int                 g_nTickCount       = 0;
static WCHAR               g_szExePath[MAX_PATH]  = { 0 };
static WCHAR               g_szLogPath[MAX_PATH]  = { 0 };
static UINT                g_msgTaskbarCreated = 0;   /* Registered "TaskbarCreated" message for Explorer restart recovery */

/* ----------------------------------------------------------------
 *  Logging (debug only)
 * ---------------------------------------------------------------- */

#ifdef EYEBREAK_DEBUG

static void WriteLog(LPCWSTR fmt, ...)
{
    FILE *fp;
    if (_wfopen_s(&fp, g_szLogPath, L"a") != 0) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    fwprintf_s(fp, L"[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    va_list args;
    va_start(args, fmt);
    vfwprintf_s(fp, fmt, args);
    va_end(args);

    fwprintf_s(fp, L"\n");
    fclose(fp);
}

#define LOG(fmt, ...) WriteLog(fmt, ##__VA_ARGS__)

#else

#define LOG(fmt, ...) ((void)0)

#endif /* EYEBREAK_DEBUG */

/*
 * InitLog - Determine log file path and create/truncate it.
 * Log file is written to log/ subdirectory next to the executable.
 */
static void InitLog(void)
{
    /* Base: exe directory */
    WCHAR *p = wcsrchr(g_szExePath, L'\\');
    if (p && p > g_szExePath) {
        wcscpy_s(g_szLogPath, MAX_PATH, g_szExePath);
        wcscpy_s(p + 1, MAX_PATH - (p + 1 - g_szLogPath), LOG_FILENAME);
    } else {
        wcscpy_s(g_szLogPath, MAX_PATH, LOG_FILENAME);
    }

    /* Ensure log/ directory exists */
    {
        WCHAR logDir[MAX_PATH];
        wcscpy_s(logDir, MAX_PATH, g_szLogPath);
        WCHAR *slash = wcsrchr(logDir, L'\\');
        if (slash) *slash = L'\0';
        CreateDirectoryW(logDir, NULL);
    }

#ifdef EYEBREAK_DEBUG
    FILE *fp;
    errno_t err = _wfopen_s(&fp, g_szLogPath, L"w");
    if (err == 0) {
        fclose(fp);
        LOG(L"=== EyeBreak start ===");
        LOG(L"log: %s", g_szLogPath);
        LOG(L"exe: %s", g_szExePath);
    } else {
        wchar_t msg[256];
        swprintf_s(msg, 256, L"Cannot create log file:\n%s\nError: %d", g_szLogPath, err);
        MessageBoxW(NULL, msg, APP_NAME, MB_ICONERROR | MB_OK);
    }
#else
    FILE *fp;
    if (_wfopen_s(&fp, g_szLogPath, L"w") != 0) {
        WCHAR *p = wcsrchr(g_szExePath, L'\\');
        if (p && p > g_szExePath) {
            wcscpy_s(g_szLogPath, MAX_PATH, g_szExePath);
            wcscpy_s(p + 1, MAX_PATH - (p + 1 - g_szExePath), LOG_FILENAME);
        }
    } else {
        fclose(fp);
        DeleteFileW(g_szLogPath);
        g_szLogPath[0] = L'\0';
    }
#endif
}

/* ----------------------------------------------------------------
 *  Forward declarations
 * ---------------------------------------------------------------- */

static BOOL      InitInstance(HINSTANCE hInstance);
static LRESULT   CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void      CreateTrayIcon(HWND hWnd);
static void      DestroyTrayIcon(void);
static void      ShowTrayMenu(HWND hWnd);
static void      SendReminder(void);
static void      UpdateTrayTooltip(void);
static HMENU     BuildMenu(void);
static BOOL      IsAutoStartEnabled(void);
static void      SetAutoStart(BOOL enable);
static void      SaveLanguagePref(BOOL english);
static BOOL      LoadLanguagePref(void);

/* ----------------------------------------------------------------
 *  WinMain
 * ---------------------------------------------------------------- */

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPWSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    /* 1. Resolve executable path */
    if (GetModuleFileNameW(NULL, g_szExePath, MAX_PATH) == 0) {
        wcscpy_s(g_szExePath, MAX_PATH, L"EyeBreak.exe");
    }

    /* 2. Initialize logging */
    InitLog();
    LOG(L"wWinMain enter");

    /* 3. Load language preference (default Chinese) */
    g_bLangEnglish = LoadLanguagePref();

    /* 4. Single-instance enforcement via named mutex */
    HANDLE hMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (!hMutex) {
        MessageBoxW(NULL, S(S_ERR_MUTEX), APP_NAME, MB_ICONERROR | MB_OK);
        return 1;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(NULL, S(S_ERR_DUP), APP_NAME, MB_ICONINFORMATION | MB_OK);
        CloseHandle(hMutex);
        return 0;
    }

    g_hInst = hInstance;

    /* 5. Register window class */
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize        = sizeof(WNDCLASSEXW);
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.lpszClassName = WINDOW_CLASS;
    if (!RegisterClassExW(&wcex)) {
        MessageBoxW(NULL, S(S_ERR_REGCLASS), APP_NAME, MB_ICONERROR | MB_OK);
        CloseHandle(hMutex);
        return 1;
    }

    /* 6. Create hidden message window */
    if (!InitInstance(hInstance)) {
        MessageBoxW(NULL, S(S_ERR_CREATEWND), APP_NAME, MB_ICONERROR | MB_OK);
        CloseHandle(hMutex);
        return 1;
    }

    /* 7. Register TaskbarCreated message (Explorer restart recovery) */
    g_msgTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    /* 8. System tray icon */
    LOG(L"CreateTrayIcon");
    CreateTrayIcon(g_hWnd);

    /* 8. Register for session lock/unlock notifications */
    LOG(L"WTSRegisterSessionNotification");
    if (!WTSRegisterSessionNotification(g_hWnd, NOTIFY_FOR_THIS_SESSION)) {
        LOG(L"WTSRegisterSessionNotification failed (non-fatal)");
    }

    /* 9. Read initial auto-start state from registry */
    g_bAutoStart = IsAutoStartEnabled();
    LOG(L"AutoStart: %s", g_bAutoStart ? L"ON" : L"OFF");
    LOG(L"Language: %s", g_bLangEnglish ? L"English" : L"Chinese");

    /* 10. Start main timer (1 s tick) and set initial tooltip */
    UpdateTrayTooltip();
    SetTimer(g_hWnd, TIMER_ID_MAIN, 1000, NULL);

    /* 11. Message loop */
    LOG(L"Message loop");
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    /* Cleanup */
    LOG(L"Cleanup");
    DestroyTrayIcon();
    KillTimer(g_hWnd, TIMER_ID_MAIN);
    KillTimer(g_hWnd, TIMER_ID_AUTO_CHECK);
    CloseHandle(hMutex);

    return (int)msg.wParam;
}

/* ----------------------------------------------------------------
 *  Window / Message handling
 * ---------------------------------------------------------------- */

static BOOL InitInstance(HINSTANCE hInstance)
{
    (void)hInstance;

    g_hWnd = CreateWindowExW(
        0, WINDOW_CLASS, APP_NAME,
        0,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) return FALSE;

    ShowWindow(g_hWnd, SW_HIDE);
    UpdateWindow(g_hWnd);
    return TRUE;
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

    case WM_TIMER:
        if (wParam == TIMER_ID_MAIN) {
            g_nTickCount++;
            UpdateTrayTooltip();

            if (g_bReminderEnabled && g_nTickCount >= REMINDER_SECONDS) {
                LOG(L"Reminder fired (%d s)", g_nTickCount);
                SendReminder();
                g_nTickCount = 0;
            }
        }
        else if (wParam == TIMER_ID_AUTO_CHECK && g_bPendingAutoCheck) {
            /*
             * Delayed auto-start verification.
             * Security software may delete the registry value asynchronously
             * after RegSetValueExW returns success. We wait 2 seconds then
             * re-read to detect such interception.
             */
            KillTimer(g_hWnd, TIMER_ID_AUTO_CHECK);
            g_bPendingAutoCheck = FALSE;

            BOOL actualState = IsAutoStartEnabled();
            if (!actualState && g_bAutoStart) {
                g_bAutoStart = FALSE;
                LOG(L"Auto-start verification FAILED (intercepted)");
                MessageBoxW(NULL,
                    g_bLangEnglish ? S(S_WARN_INTERCEPTED_EN) : S(S_WARN_INTERCEPTED_CN),
                    S(S_NOTIFY_TITLE), MB_ICONWARNING | MB_OK);
            } else {
                LOG(L"Auto-start verification OK");
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_EXIT:
            PostMessageW(hWnd, WM_CLOSE, 0, 0);
            break;
        case IDM_TEST_NOTIFY:
            LOG(L"Test notification");
            SendReminder();
            break;
        case IDM_STOP_REMINDER:
            g_bReminderEnabled = FALSE;
            g_nTickCount = 0;
            KillTimer(g_hWnd, TIMER_ID_MAIN);
            LOG(L"Reminder stopped");
            UpdateTrayTooltip();
            break;
        case IDM_START_REMINDER:
            g_bReminderEnabled = TRUE;
            g_nTickCount = 0;
            SetTimer(g_hWnd, TIMER_ID_MAIN, 1000, NULL);
            LOG(L"Reminder started");
            UpdateTrayTooltip();
            break;
        case IDM_AUTO_START:
            SetAutoStart(TRUE);
            break;
        case IDM_NO_AUTO_START:
            SetAutoStart(FALSE);
            break;
        case IDM_LANG_SWITCH:
            g_bLangEnglish = !g_bLangEnglish;
            SaveLanguagePref(g_bLangEnglish);
            LOG(L"Language switched to: %s", g_bLangEnglish ? L"English" : L"Chinese");
            UpdateTrayTooltip();
            break;
        }
        break;

    case WM_USER_TRAY:
        if (lParam == WM_RBUTTONUP) {
            ShowTrayMenu(hWnd);
        }
        break;

    case WM_WTSSESSION_CHANGE:
        if (!g_bReminderEnabled) break;
        if (wParam == WTS_SESSION_LOCK) {
            LOG(L"Session LOCK, kill timer");
            KillTimer(g_hWnd, TIMER_ID_MAIN);
            g_nTickCount = 0;
        }
        else if (wParam == WTS_SESSION_UNLOCK) {
            LOG(L"Session UNLOCK, restart timer");
            g_nTickCount = 0;
            SetTimer(g_hWnd, TIMER_ID_MAIN, 1000, NULL);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        /*
         * Explorer restart recovery: TaskbarCreated is broadcast when
         * Explorer.exe restarts (e.g. after crash, Windows restart).
         * All previous tray icons are lost — re-add ours to restore
         * the system-tray presence and interaction.
         */
        if (msg == g_msgTaskbarCreated) {
            LOG(L"TaskbarCreated received, restoring tray icon");
            CreateTrayIcon(g_hWnd);
            UpdateTrayTooltip();
            break;
        }
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

/* ----------------------------------------------------------------
 *  System tray
 * ---------------------------------------------------------------- */

static void CreateTrayIcon(HWND hWnd)
{
    g_hIcon = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_TRAY));
    if (!g_hIcon) {
        g_hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize           = sizeof(g_nid);
    g_nid.hWnd             = hWnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.hIcon            = g_hIcon;
    g_nid.uCallbackMessage = WM_USER_TRAY;
    wcscpy_s(g_nid.szTip, _countof(g_nid.szTip), TOOLTIP_TEXT);

    if (!Shell_NotifyIconW(NIM_ADD, &g_nid)) {
        MessageBoxW(NULL, S(S_ERR_TRAY), APP_NAME, MB_ICONWARNING | MB_OK);
    }
}

static void DestroyTrayIcon(void)
{
    WTSUnRegisterSessionNotification(g_hWnd);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_hIcon) {
        DestroyIcon(g_hIcon);
        g_hIcon = NULL;
    }
}

/* ----------------------------------------------------------------
 *  Tooltip
 * ---------------------------------------------------------------- */

static void UpdateTrayTooltip(void)
{
    WCHAR tip[128];
    if (!g_bReminderEnabled) {
        wcscpy_s(tip, _countof(tip), S(S_TIP_STOPPED));
    } else {
        int remaining = REMINDER_SECONDS - g_nTickCount;
        if (remaining < 0) remaining = 0;
        int min = remaining / 60;
        int sec = remaining % 60;
        swprintf_s(tip, _countof(tip), L"EyeBreak - %02d:%02d", min, sec);
    }
    wcscpy_s(g_nid.szTip, _countof(g_nid.szTip), tip);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

/* ----------------------------------------------------------------
 *  Context menu
 * ---------------------------------------------------------------- */

/*
 * BuildMenu - Rebuild the tray context menu each time it's shown.
 *
 * Auto-start state and language are read fresh each time so the
 * menu always reflects current reality.
 */
static HMENU BuildMenu(void)
{
    g_bAutoStart = IsAutoStartEnabled();

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return NULL;

    AppendMenuW(hMenu, MF_STRING, IDM_TEST_NOTIFY,    S(S_MENU_TEST));

    if (g_bReminderEnabled) {
        AppendMenuW(hMenu, MF_STRING, IDM_STOP_REMINDER,  S(S_MENU_STOP));
    } else {
        AppendMenuW(hMenu, MF_STRING, IDM_START_REMINDER, S(S_MENU_START));
    }

    if (g_bAutoStart) {
        AppendMenuW(hMenu, MF_STRING, IDM_NO_AUTO_START, S(S_MENU_AUTOSTART_ON));
    } else {
        AppendMenuW(hMenu, MF_STRING, IDM_AUTO_START,   S(S_MENU_AUTOSTART_OFF));
    }

    /* Language switch item — shows the target language name */
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_LANG_SWITCH,
        g_bLangEnglish ? S(S_MENU_LANG_CN) : S(S_MENU_LANG_EN));

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_EXIT,            S(S_MENU_EXIT));
    return hMenu;
}

static void ShowTrayMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = BuildMenu();
    if (!hMenu) return;

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

/* ----------------------------------------------------------------
 *  Reminder notification
 * ---------------------------------------------------------------- */

/*
 * SendReminder - Show a balloon tooltip on the tray icon.
 */
static void SendReminder(void)
{
    g_nid.uFlags      = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    wcscpy_s(g_nid.szInfoTitle, _countof(g_nid.szInfoTitle), S(S_NOTIFY_TITLE));
    wcscpy_s(g_nid.szInfo,      _countof(g_nid.szInfo),
             g_bLangEnglish ? S(S_NOTIFY_TEXT_EN) : S(S_NOTIFY_TEXT_CN));
    g_nid.uTimeout    = 10000;
    g_nid.dwInfoFlags = NIIF_INFO;

    Shell_NotifyIconW(NIM_MODIFY, &g_nid);

    /* Restore normal state */
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

/* ----------------------------------------------------------------
 *  Auto-start (Registry HKCU\Run)
 * ---------------------------------------------------------------- */

static BOOL IsAutoStartEnabled(void)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return FALSE;
    }

    DWORD type;
    DWORD dataLen = sizeof(WCHAR) * MAX_PATH;
    WCHAR value[MAX_PATH] = { 0 };
    LONG ret = RegQueryValueExW(hKey, REG_VAL_NAME, NULL, &type, (LPBYTE)value, &dataLen);
    RegCloseKey(hKey);

    if (ret == ERROR_SUCCESS && _wcsicmp(value, g_szExePath) == 0) {
        return TRUE;
    }
    return FALSE;
}

static void SetAutoStart(BOOL enable)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_SET_VALUE, &hKey) != ERROR_SUCCESS) {
        MessageBoxW(NULL, S(S_ERR_REGOPEN), S(S_NOTIFY_TITLE), MB_ICONWARNING | MB_OK);
        return;
    }

    if (enable) {
        LONG ret = RegSetValueExW(hKey, REG_VAL_NAME, 0, REG_SZ,
            (const BYTE *)g_szExePath, (DWORD)(wcslen(g_szExePath) + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);

        if (ret != ERROR_SUCCESS) {
            g_bAutoStart = FALSE;
            MessageBoxW(NULL, S(S_ERR_REGWRITE), S(S_NOTIFY_TITLE), MB_ICONERROR | MB_OK);
            return;
        }

        g_bAutoStart = TRUE;
        g_bPendingAutoCheck = TRUE;
        SetTimer(g_hWnd, TIMER_ID_AUTO_CHECK, AUTO_CHECK_DELAY_MS, NULL);
        LOG(L"Auto-start requested, verify in %d ms", AUTO_CHECK_DELAY_MS);
    } else {
        LONG ret = RegDeleteValueW(hKey, REG_VAL_NAME);
        RegCloseKey(hKey);

        if (ret == ERROR_SUCCESS || ret == ERROR_FILE_NOT_FOUND) {
            g_bAutoStart = IsAutoStartEnabled();
            LOG(L"Auto-start disabled");
        } else {
            LOG(L"RegDeleteValue error=%ld", ret);
        }
    }
}

/* ----------------------------------------------------------------
 *  Language preference persistence
 * ---------------------------------------------------------------- */

/*
 * LoadLanguagePref - Read language from HKCU\Software\EyeBreak\Language.
 * Returns FALSE (Chinese) if the key doesn't exist — default to Chinese.
 */
static BOOL LoadLanguagePref(void)
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_SETTINGS_KEY, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return FALSE;  /* Default: Chinese */
    }

    DWORD type;
    DWORD data = 0;  /* 0 = CN, 1 = EN */
    DWORD dataSize = sizeof(DWORD);
    LONG ret = RegQueryValueExW(hKey, REG_VAL_LANG, NULL, &type, (LPBYTE)&data, &dataSize);
    RegCloseKey(hKey);

    return (ret == ERROR_SUCCESS) ? (data != 0) : FALSE;
}

/*
 * SaveLanguagePref - Write language preference to registry.
 * Creates the key if it doesn't exist.
 */
static void SaveLanguagePref(BOOL english)
{
    HKEY hKey;
    DWORD disp;
    LONG ret = RegCreateKeyExW(HKEY_CURRENT_USER, REG_SETTINGS_KEY, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, &hKey, &disp);
    if (ret != ERROR_SUCCESS) return;

    DWORD val = english ? 1 : 0;
    RegSetValueExW(hKey, REG_VAL_LANG, 0, REG_DWORD, (const BYTE *)&val, sizeof(DWORD));
    RegCloseKey(hKey);
}
