// main.cpp - minimal demo host for BinEdit
//
// Shows the control docked in a window, with three radio buttons to switch
// between ANSI / UTF-8 / UTF-16 text-column decoding, and a File > Open
// command to load an arbitrary file into the editor.
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <cstdio>
#include "BinEdit.h"
#include "resource.h"

HWND g_hMainWnd = nullptr;
HWND g_hEdit = nullptr;

BOOL LoadFile(HWND hwnd, PCWSTR path)
{
    FILE *fout = _wfopen(path, L"rb");
    if (!fout)
        return FALSE;

    char buf[2048];
    std::vector<BYTE> data;
    for (;;)
    {
        size_t count = fread(buf, 1, sizeof(buf), fout);
        if (!count)
            break;
        data.insert(data.end(), &buf[0], &buf[count]);
    }

    fclose(fout);

    auto ctl = BinEdit::FromHwnd(g_hEdit);
    if (ctl)
        ctl->SetData(std::move(data));
    return TRUE;
}

void OpenFileDialog(HWND hOwner)
{
    WCHAR path[MAX_PATH] = {};
    OPENFILENAMEW ofn = { sizeof(ofn) };
    ofn.hwndOwner = hOwner;
    ofn.lpstrFile = path;
    ofn.nMaxFile = _countof(path);
    ofn.lpstrFilter = L"All files\0*.*\0";
    ofn.lpstrTitle = L"Open File";
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&ofn))
        return;

    LoadFile(hOwner, path);
}

BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    HINSTANCE hInst = lpCreateStruct->hInstance;

    BinEdit::RegisterWindowClass(hInst);
    g_hEdit = BinEdit::Create(hwnd, ID_EDIT, 0, 0, 100, 100, hInst);

    // sample data so the control isn't empty on first run
    char text[] = "Hello, BinEdit! \x00\x01\x02\xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF";
    constexpr size_t textLen = sizeof(text) - 1; // -1 to drop the final NUL terminator
    std::vector<BYTE> sample(reinterpret_cast<const BYTE*>(text), reinterpret_cast<const BYTE*>(text) + textLen);

    auto ctl = BinEdit::FromHwnd(g_hEdit);
    if (ctl)
        ctl->SetData(sample);

    DragAcceptFiles(hwnd, TRUE);
    SetFocus(g_hEdit);
    return TRUE;
}

void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    if (state == WA_ACTIVE || state == WA_CLICKACTIVE)
        SetFocus(g_hEdit);
}

void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    if (g_hEdit)
        MoveWindow(g_hEdit, 0, 0, cx, cy, TRUE);
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    auto ctl = BinEdit::FromHwnd(g_hEdit);
    if (!ctl)
        return;

    switch (id)
    {
    case ID_ANSI:
        ctl->SetTextMode(BinEditTextMode::ANSI);
        break;
    case ID_UTF8 :
        ctl->SetTextMode(BinEditTextMode::UTF8);
        break;
    case ID_UTF16:
        ctl->SetTextMode(BinEditTextMode::UTF16);
        break;
    case ID_TOGGLE_HEADER:
        ctl->SetShowHeader(!ctl->GetShowHeader());
        break;
    case ID_FILE_OPEN:
        OpenFileDialog(hwnd);
        break;
    case ID_EXIT:
        DestroyWindow(hwnd);
        break;
    case ID_EDIT:
        if (codeNotify == BEN_CHANGE)
        {
            ;
        }
        break;
    }
}

void OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR path[MAX_PATH];
    DragQueryFileW(hdrop, 0, path, _countof(path));
    LoadFile(hwnd, path);
}

void OnDestroy(HWND hwnd)
{
    DestroyWindow(g_hEdit);
    g_hEdit = nullptr;
    PostQuitMessage(0);
}

void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    auto ctl = BinEdit::FromHwnd(g_hEdit);
    if (!ctl)
        return;

    switch (ctl->GetTextMode())
    {
    case BinEditTextMode::ANSI:
        CheckMenuRadioItem(hMenu, ID_ANSI, ID_UTF16, ID_ANSI, MF_BYCOMMAND);
        break;
    case BinEditTextMode::UTF8:
        CheckMenuRadioItem(hMenu, ID_ANSI, ID_UTF16, ID_UTF8, MF_BYCOMMAND);
        break;
    case BinEditTextMode::UTF16:
        CheckMenuRadioItem(hMenu, ID_ANSI, ID_UTF16, ID_UTF16, MF_BYCOMMAND);
        break;
    }

    CheckMenuItem(hMenu, ID_TOGGLE_HEADER,
                  MF_BYCOMMAND | (ctl->GetShowHeader() ? MF_CHECKED : MF_UNCHECKED));
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
        HANDLE_MSG(hwnd, WM_ACTIVATE, OnActivate);
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

BOOL InitInst(HINSTANCE hInstance, INT nCmdShow)
{
    InitCommonControls();

    const WCHAR* kClass = L"BinEditDemoMain";

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kClass;
    wc.lpszMenuName = MAKEINTRESOURCEW(1);
    if (!RegisterClassExW(&wc))
        return FALSE;

    HWND hwnd = CreateWindowExW(0, kClass, L"BinEdit demo",
                                 WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT, CW_USEDEFAULT, 780, 560,
                                 nullptr, nullptr, hInstance, nullptr);
    if (!hwnd)
        return FALSE;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    return TRUE;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    if (!InitInst(hInstance, nCmdShow))
    {
        MessageBoxW(nullptr, L"InitInst failed", nullptr, MB_ICONERROR);
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
