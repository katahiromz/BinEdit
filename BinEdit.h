// BinEdit.h --- Binary edit control
// Author: katahiromz
// License: MIT
//
// A self-contained Win32 custom control that shows/edits a byte buffer as a
// classic "hex dump" (address + hex bytes + decoded text column), editable with
// scrollbars, caret, keyboard/mouse navigation, and full IME support.
//
#pragma once

#include <windows.h>
#include <imm.h>
#include <vector>
#include <string>

// テキスト列のデコードモード
enum class BinEditTextMode
{
    ANSI  = 0, // Windows ANSI コードページ (CP_ACP), DBCS対応
    UTF8  = 1, // UTF-8
    UTF16 = 2, // UTF-16LE (サロゲートペア対応)
    SJIS  = 3  // Shift_JIS (CP932)
};

// 親ウィンドウへ送信される通知コード (WM_COMMAND)
#define BEN_CHANGE 0x0001

// BinEdit 固有のスタイルビット
#define BES_NOHEADER 0x00000001 // アドレス・16進ヘッダーの非表示
#define BES_READONLY 0x00000002 // 読み取り専用モード

// 1行あたりのバイト数
#define BYTES_PER_LINE 16

class BinEdit
{
public:
    using data_type = std::vector<BYTE>;

    enum PANE
    {
        PANE_ADDRESS = 0,
        PANE_HEX     = 1,
        PANE_TEXT    = 2
    };

    explicit BinEdit(HWND hwnd);
    ~BinEdit();

    operator HWND() const { return m_hwnd; }

    static const WCHAR* ClassName();
    static BOOL RegisterWindowClass(HINSTANCE hInstance);
    static HWND Create(HWND hParent, int controlId,
                       int x, int y, int width, int height,
                       HINSTANCE hInstance, DWORD extraStyle = 0);
    static BinEdit* FromHwnd(HWND hwnd);

    // ---- データ操作 --------------------------------------------------
    void SetData(data_type data);
    const data_type& GetData() const { return m_data; }
    void SetDataSrc(data_type* data_src = nullptr);
    std::wstring GetDumpText() const;

    // ---- デコードモード設定 ------------------------------------------
    void SetTextMode(BinEditTextMode mode);
    BinEditTextMode GetTextMode() const { return m_textMode; }

    // ---- ヘッダー・読み取り専用・キャレット設定 ----------------------
    void SetShowHeader(bool show);
    bool GetShowHeader() const { return m_showHeader; }

    void SetReadOnly(bool readOnly);
    bool GetReadOnly() const { return m_readOnly; }

    HWND GetHwnd() const { return m_hwnd; }
    DWORD GetCaretOffset() const { return m_caretOffset; }
    void SetCaretOffset(DWORD offset);

    // ---- 挿入/上書きモード -------------------------------------------
    bool GetInsertMode() const { return m_insertMode; }
    void SetInsertMode(bool insert);

    // ---- 選択範囲操作 ------------------------------------------------
    bool HasSelection() const;
    void GetSelection(DWORD& start, DWORD& end) const;
    void SetSelection(DWORD start, DWORD end);
    void ClearSelection();

    // ---- クリップボード操作 ------------------------------------------
    bool Copy();
    bool Cut();
    bool Paste();
    bool CopyDumpText();

    // ---- DPI & ダイアログ --------------------------------------------
    UINT GetDpi() const { return m_dpi; }
    bool GoToOffsetDialog();

protected:
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

    // イベントハンドラー
    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    void OnDestroy(HWND hwnd);
    void OnSize(HWND hwnd, UINT state, int cx, int cy);
    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc);
    void OnPaint(HWND hwnd);
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
    void OnChar(HWND hwnd, TCHAR ch, int cRepeat);
    void OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
    void OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
    void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys);
    void OnSetFocus(HWND hwnd, HWND hwndOldFocus);
    void OnKillFocus(HWND hwnd, HWND hwndNewFocus);
    void OnEnable(HWND hwnd, BOOL fEnable);
    BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg);
    void OnSetFont(HWND hwnd, HFONT hfont, BOOL fRedraw);
    HFONT OnGetFont(HWND hwnd);
    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
    void OnDpiChanged(WPARAM wParam, LPARAM lParam);
    void OnCut(HWND hwnd);
    void OnCopy(HWND hwnd);
    void OnPaste(HWND hwnd);
    void OnClear(HWND hwnd);

    // ドラッグ選択・自動スクロール
    bool AutoScrollDragIfNeeded(int y);
    void UpdateGutterDragSelection(int x, int y);
    void UpdateByteDragSelection(int x, int y);
    void OnDragAutoScrollTimer();

    // IMEサポート
    void UpdateImeCompositionWindow();
    bool InsertUnicodeText(const WCHAR* text, int cch);
    LRESULT OnImeComposition(WPARAM wParam, LPARAM lParam);
    LRESULT OnImeStartComposition();
    LRESULT OnImeEndComposition();
    LRESULT OnImeSetContext(WPARAM wParam, LPARAM lParam);
    LRESULT OnImeNotify(WPARAM wParam, LPARAM lParam);
    LRESULT OnImeChar(WPARAM wParam, LPARAM lParam);

    // レイアウト＆描画補助
    void EnsureCaretVisible();
    void ShowContextMenu(int screenX, int screenY);
    void UpdateScrollInfo();
    int GetMaxTopLine() const;
    int GetMaxScrollX() const;
    int GetContentWidth() const;
    int GetTotalLines() const;
    void InvalidateAll();
    void NotifyChanged();
    void RecalcLayout();
    void CreateEditFont();
    void MeasureFontMetrics();
    void UpdateCaretShape();
    void RecreateCaret();
    void DrawHeader(HDC hdc);

    // キャッシュ構築＆ヒットテスト
    void RebuildDecodeCache();
    // pos..pos+oldLen (旧バッファ) が pos..pos+newLen (新バッファ) に置き換わった直後に呼ぶ。
    // 編集点付近だけを再デコードし、可能な限り RebuildDecodeCache() のフルスキャンを避ける。
    // 安全に再同期できない場合は自動的に RebuildDecodeCache() にフォールバックする。
    void UpdateDecodeCacheAfterEdit(DWORD editPos, DWORD oldLen, DWORD newLen);
    bool HitTest(int x, int y, DWORD& byteOffset, PANE& pane, bool& hiNibble) const;

    // キャレット移動・編集
    void MoveCaretBy(long deltaBytes);
    void MoveCaretHome(bool wholeBuffer);
    void MoveCaretEnd(bool wholeBuffer);
    void ApplySelectionAfterMove(bool extend);
    bool DeleteSelection();
    bool IsEditable() const;
    void SelectAll();

    void InsertByteAt(DWORD pos, BYTE value);
    void DeleteByteAt(DWORD pos);
    void DeleteRange(DWORD pos, DWORD count);
    void ReplaceRange(DWORD pos, DWORD oldCount, const BYTE* newBytes, DWORD newCount);

    void EncodeCharToBytes(WCHAR ch, data_type& outBytes) const;
    DWORD DecodedLengthAt(DWORD pos) const;

    static UINT GetBinEditBytesFormat();
    static std::wstring BytesToHexString(const BYTE* data, DWORD count);
    static bool ParseHexString(const WCHAR* text, data_type& out);
    std::wstring SelectionAsText(DWORD start, DWORD end) const;
    void EncodeTextToBytes(const WCHAR* text, int cch, data_type& out) const;

    bool SetClipboardBytes(const BYTE* data, DWORD count, bool alsoHexText);
    bool SetClipboardUnicodeText(const std::wstring& text);

    DWORD size() const { return static_cast<DWORD>(m_data_src->size()); }
    data_type::iterator begin() { return m_data_src->begin(); }
    data_type::iterator end() { return m_data_src->end(); }

    struct DecodedChar
    {
        DWORD offset;
        DWORD length;       // 消費されたソースバイト数
        std::wstring glyph; // 描画用のUTF-16文字
        bool printable;
    };

protected:
    HWND m_hwnd = nullptr;
    data_type m_data;
    data_type* m_data_src = nullptr;
    BinEditTextMode m_textMode = BinEditTextMode::UTF8;

    HFONT m_hFont = nullptr;
    bool m_ownFont = false;
    int m_charWidth = 8;
    int m_lineHeight = 16;
    UINT m_dpi = 96;
    int m_headerHeight = 0;
    int m_headerGap = 0;
    bool m_showHeader = true;
    bool m_readOnly = false;
    HPEN m_hHeaderLinePen = nullptr; // ヘッダー区切り線用 (OnPaint 毎の CreatePen/DeleteObject を回避するためキャッシュ)

    int m_topLine = 0;
    int m_visibleLines = 1;
    int m_scrollX = 0;
    int m_contentWidth = 0;

    DWORD m_caretOffset = 0;
    DWORD m_anchorOffset = 0;
    bool m_caretHiNibble = true;
    PANE m_focusPane = PANE_HEX;
    bool m_hasFocus = false;
    bool m_trackingMouse = false;
    bool m_gutterDrag = false;
    DWORD m_gutterAnchorLine = 0;
    bool m_insertMode = true;
    bool m_suppressImeChar = false;

    // レイアウト位置情報
    int m_addrColX = 0;
    int m_hexColX = 0;
    int m_textColX = 0;
    int m_clientWidth = 0;
    int m_clientHeight = 0;

    std::vector<DecodedChar> m_decoded;
    std::vector<int> m_byteToDecodedIndex;
};
