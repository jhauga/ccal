// ccal_gui.c
// Simple Windows GUI calculator using the calc.c expression evaluator
// Provides digit and operator buttons and input box; keyboard also supported
// These additional educational comments walk through the Win32 plumbing and
// explain how the GUI keeps user input and the shared evaluator in sync.

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "resource.h"
#include "remove_format.h"

// Declare the external evaluate function implemented in calc.c.
// With this declaration the GUI can reuse the tested parser without having
// to duplicate math logic, which keeps the maintenance burden lower.
extern double evaluate_expr_string(const char* expr, int* error);

// GLOBAL ELEMENTS:
//////////////////////////////////////////////////////////////////////////////

// Control IDs for window elements.
#define ID_INPUT   1
#define ID_OUTPUT  2
#define ID_EQUAL   3
#define ID_HISTORY 4

// History panel constants
#define MAX_HISTORY 100
#define HISTORY_MIN_WIDTH 700  // minimum window width to show history
#define HISTORY_WIDTH 400
#define HISTORY_ITEM_HEIGHT 20
#define HISTORY_COLUMNS 4
#define HISTORY_ROWS_PER_COL 10

// History entry structure
typedef struct {
    char equation[256];
    int active;
} HistoryEntry;

HistoryEntry gHistory[MAX_HISTORY];
int gHistoryCount = 0;
int gHistoryHoverIndex = -1;

HWND hwnd, hInput, hOutput, hHistory;  // handles to input, output, and history controls
WNDPROC DefaultEditProc;
// Subclassing relies on saving the original procedure so we can hand control
// back to Windows when our custom logic has finished processing a message.

#define BTN_COUNT 20
HWND hButtons[BTN_COUNT];  // store up to 20 buttons
// Every Win32 control is just another window, and storing handles lets the
// message procedure address each button without re-querying the OS.

// Global variables.
int dec = 0;    // no duplicate decimals
int equ = 0;    // allow continue equation
// These flags mimic the state machine in the CLI version so we can detect
// whether the current token stream should accept another decimal or chain the
// previous result into a new operation.

static int gPendingFullFormat = 0;
static char gStoredEquation[512];
// The formatting flags decouple display cleanup from keystrokes. We defer a
// full pass when an edit might have ripple effects, which keeps typing fluid.

// Forward declarations for formatting helpers
static void FormatEntireEquation(HWND hWnd);
static void FormatCurrentNumberSegment(HWND hWnd);
static void ApplyPendingFullFormat(HWND hWnd, int formatSegment);
static void AddToHistory(const char* equation);
static void DrawHistory(HDC hdc, RECT* rect);
static int GetHistoryItemAtPoint(int x, int y, RECT* clientRect);
static void UpdateHistoryVisibility(int width);
void FocusOnInput();

// SUPPORT FUNCTIONS:
//////////////////////////////////////////////////////////////////////////////

// Append text to an edit control, optionally adding a space after
// Win32 edit controls maintain their own caret, so we first select the end and
// then replace the selection to append text. This avoids flicker compared to
// fetching and re-setting the entire string each time.
void AppendText(HWND hWnd, const char* text, int add_space) {
    int len = GetWindowTextLength(hWnd);
    // set cursor to end of text
    SendMessage(hWnd, EM_SETSEL, len, len);
    // insert new text at cursor position
    SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)text);
    if (add_space)
        SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)" ");
}

static int IsOperatorChar(char ch) {
    // The GUI accepts both keyboard-friendly symbols and the letters used in
    // the CLI (like p for power), keeping parity across front-ends.
    return (ch == '+' || ch == '-' || ch == '/' || ch == '*' ||
            ch == 'x' || ch == 'X' || ch == '^' || ch == 'p' || ch == 'P');
}

static int IsEquationInputChar(char ch) {
    // Brackets are allowed because the evaluator supports multiple nesting
    // styles. We also keep spaces so users can format expressions by hand.
    return (isdigit((unsigned char)ch) || ch == '.' ||
            ch == '(' || ch == ')' || ch == '[' || ch == ']' ||
            ch == '{' || ch == '}' || ch == ' ' || IsOperatorChar(ch));
}

static int FormatSegment(const char* clean, char* formatted, size_t formattedSize,
                         int caretCount, int* caretOffset) {
    // This utility accepts a digits-only string and rebuilds it with commas
    // every three places. The caret parameters let callers keep the cursor in
    // roughly the same numeric column after formatting.
    if (!clean || !formatted || formattedSize == 0)
        return -1;

    if (caretOffset)
        *caretOffset = -1;

    int cleanLen = (int)strlen(clean);
    const char* dotPtr = strchr(clean, '.');
    int intLen = dotPtr ? (int)(dotPtr - clean) : cleanLen;

    int pos = 0;

    // The integer portion is grouped first so we can sprinkle commas without
    // having to move any fractional characters later.
    for (int idx = 0; idx < intLen; ++idx) {
        if ((size_t)(pos + 1) >= formattedSize)
            return -1;
        if (caretOffset && caretCount >= 0 && idx == caretCount)
            *caretOffset = pos;
        formatted[pos++] = clean[idx];
        int digitsRemaining = intLen - idx - 1;
        if (digitsRemaining > 0 && digitsRemaining % 3 == 0) {
            if ((size_t)(pos + 1) >= formattedSize)
                return -1;
            formatted[pos++] = ',';
        }
    }

    if (caretOffset && caretCount >= 0 && caretCount == intLen)
        *caretOffset = pos;

    if (dotPtr) {
        // Fractional digits are copied verbatim. Display precision comes from
        // the evaluator, so the GUI simply mirrors that exact string.
        for (int idx = intLen; idx < cleanLen; ++idx) {
            if ((size_t)(pos + 1) >= formattedSize)
                return -1;
            if (caretOffset && caretCount >= 0 && idx == caretCount)
                *caretOffset = pos;
            formatted[pos++] = clean[idx];
        }
    }

    if (caretOffset && caretCount >= 0 && caretCount >= cleanLen)
        *caretOffset = pos;

    if (pos >= (int)formattedSize)
        return -1;

    formatted[pos] = '\0';
    return pos;
}

// Format the number segment surrounding the caret with grouping commas.
static void FormatCurrentNumberSegment(HWND hWnd) {
    char text[512];
    int len = GetWindowText(hWnd, text, sizeof(text));
    if (len <= 0)
        return;

    // Win32 edit controls expose a zero-based selection range that doubles as
    // the caret location when start and end match. We rely on that to rebuild
    // the surroundings while keeping the caret anchored.

    DWORD selStart = 0;
    DWORD selEnd = 0;
    SendMessage(hWnd, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);
    if (selStart != selEnd)
        return;  // avoid reformatting when a range is selected

    int caret = (int)selEnd;
    int segStart = caret;
    while (segStart > 0) {
        char ch = text[segStart - 1];
        if (isdigit((unsigned char)ch) || ch == '.' || ch == ',')
            segStart--;
        else
            break;
    }

    int segEnd = caret;
    while (segEnd < len) {
        char ch = text[segEnd];
        if (isdigit((unsigned char)ch) || ch == '.' || ch == ',')
            segEnd++;
        else
            break;
    }

    if (segEnd <= segStart)
        return;

    // The scan above isolates the "current number." Now we create temporary
    // buffers so we can remove commas, reformat, and splice the segment back.

    char originalSegment[256];
    int originalLen = 0;
    for (int i = segStart; i < segEnd && originalLen < (int)sizeof(originalSegment) - 1; ++i)
        originalSegment[originalLen++] = text[i];
    originalSegment[originalLen] = '\0';

    char cleanSegment[256];
    int cleanLen = 0;
    for (int i = segStart; i < segEnd && cleanLen < (int)sizeof(cleanSegment) - 1; ++i) {
        if (text[i] != ',')
            cleanSegment[cleanLen++] = text[i];
    }
    cleanSegment[cleanLen] = '\0';
    if (cleanLen == 0)
        return;

    // track caret position relative to numeric characters (excluding commas)
    int caretNumeric = 0;
    for (int i = segStart; i < (int)selStart; ++i) {
        if (text[i] != ',')
            caretNumeric++;
    }

    int caretCount = (selStart >= segStart && selStart <= segEnd) ? caretNumeric : -1;

    char formatted[256];
    int caretOffset = -1;
    int formattedLen = FormatSegment(cleanSegment, formatted, sizeof(formatted), caretCount, &caretOffset);
    if (formattedLen < 0)
        return;
    if (strcmp(formatted, originalSegment) == 0)
        return;  // nothing changed

    char newText[512];

    // rebuild full text with formatted segment
    if (segStart + formattedLen + (len - segEnd) >= (int)sizeof(newText))
        return;  // ensure buffer capacity
    int prefixLen = segStart;
    int suffixLen = len - segEnd;
    memcpy(newText, text, prefixLen);
    memcpy(newText + prefixLen, formatted, formattedLen);
    memcpy(newText + prefixLen + formattedLen, text + segEnd, suffixLen);
    newText[prefixLen + formattedLen + suffixLen] = '\0';

    // Writing back the full buffer in one go prevents partial updates that can
    // confuse Win32's undo stack or caret management.
    SetWindowText(hWnd, newText);

    // compute new caret position aligned with original numeric offset
    int newCaretPos = prefixLen;
    if (caretOffset >= 0)
        newCaretPos = prefixLen + caretOffset;
    SendMessage(hWnd, EM_SETSEL, newCaretPos, newCaretPos);
}

static int DetectRemovedDelimiter(const char* before, const char* after, int* removalIndex, char* removedChar) {
    // We use a simple two-pointer scan to spot the first difference between the
    // buffers. For this UI we only care about single-character removals, which
    // keeps the implementation compact.
    if (!before || !after)
        return 0;

    int i = 0;
    int j = 0;
    while (before[i] && after[j]) {
        if (before[i] == after[j]) {
            ++i;
            ++j;
            continue;
        }
        break;
    }

    if (before[i] == '\0')
        return 0;

    if (removalIndex)
        *removalIndex = i;
    if (removedChar)
        *removedChar = before[i];
    return 1;
}

static void FormatEntireEquation(HWND hWnd) {
    char text[512];
    int len = GetWindowText(hWnd, text, sizeof(text));
    if (len <= 0)
        return;

    // A full-format pass is more expensive, so we run it only when an edit can
    // affect multiple numbers—pastes, operator deletions, or other bulk edits.

    DWORD selStart = 0;
    DWORD selEnd = 0;
    SendMessage(hWnd, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);
    if (selStart != selEnd)
        return;

    int caretOld = (int)selEnd;
    char result[512];
    int resLen = 0;
    int newCaretPos = caretOld;

    // We rebuild the expression into `result`, copying characters one by one.
    // This sidesteps the need for dynamic allocation and keeps latency low.
    for (int i = 0; i < len; ) {
        char ch = text[i];
        if (isdigit((unsigned char)ch) || ch == '.' || ch == ',') {
            int segStart = i;
            char segment[256];
            int segLen = 0;
            // Loop until the next non-digit to isolate the numeric run.
            while (i < len) {
                char s = text[i];
                if (!(isdigit((unsigned char)s) || s == '.' || s == ','))
                    break;
                if (segLen < (int)sizeof(segment) - 1)
                    segment[segLen++] = s;
                ++i;
            }
            segment[segLen] = '\0';

            char clean[256];
            int cleanLen = 0;
            for (int k = 0; k < segLen && cleanLen < (int)sizeof(clean) - 1; ++k) {
                if (segment[k] != ',')
                    clean[cleanLen++] = segment[k];
            }
            clean[cleanLen] = '\0';

            int caretCount = -1;
            if (caretOld >= segStart && caretOld <= i) {
                caretCount = 0;
                // The caret scan repeats the per-number logic so the cursor
                // does not jump as we insert commas.
                for (int k = segStart; k < caretOld; ++k) {
                    if (text[k] != ',')
                        caretCount++;
                }
            }

            char formatted[256];
            int caretOffset = -1;
            int formattedLen = FormatSegment(clean, formatted, sizeof(formatted), caretCount, &caretOffset);
            if (formattedLen < 0 || resLen + formattedLen >= (int)sizeof(result))
                return;
            memcpy(result + resLen, formatted, formattedLen);
            if (caretOffset >= 0)
                newCaretPos = resLen + caretOffset;
            resLen += formattedLen;
        } else {
            if (ch == ' ') {
                int next = i + 1;
                char nextChar = 0;
                while (next < len) {
                    if (text[next] != ' ') {
                        nextChar = text[next];
                        break;
                    }
                    ++next;
                }
                // Collapsing redundant spaces keeps manual edits tidy without
                // stripping the optional spacing around operators entirely.
                if (nextChar == 0 || IsOperatorChar(nextChar)) {
                    ++i;
                    continue;
                }
                if (resLen == 0 || result[resLen - 1] == ' ') {
                    ++i;
                    continue;
                }
                if (resLen >= (int)sizeof(result) - 1)
                    return;
                result[resLen++] = ' ';
                ++i;
            } else {
                // Non-space, non-digit characters (operators, parentheses)
                // are copied across untouched to avoid altering the equation.
                if (resLen >= (int)sizeof(result) - 1)
                    return;
                result[resLen++] = ch;
                ++i;
            }
            if (i <= caretOld)
                newCaretPos = resLen;
        }
    }

    result[resLen] = '\0';
    SetWindowText(hWnd, result);
    if (newCaretPos < 0)
        newCaretPos = resLen;
    if (newCaretPos > resLen)
        newCaretPos = resLen;
    SendMessage(hWnd, EM_SETSEL, newCaretPos, newCaretPos);
}

static void ApplyPendingFullFormat(HWND hWnd, int formatSegment) {
    if (gPendingFullFormat) {
        FormatEntireEquation(hWnd);
        gPendingFullFormat = 0;
        gStoredEquation[0] = '\0';
        // Clearing the stored snapshot avoids reusing stale data on the next
        // edit cycle. We only cache the original text long enough to schedule
        // the delayed full-format pass.
    } else if (formatSegment) {
        // Segment-only formatting keeps keystrokes snappy because the edit
        // control typically holds short chunks of text during casual input.
        FormatCurrentNumberSegment(hWnd);
    }
}

// Add calculation to history
static void AddToHistory(const char* equation) {
    if (gHistoryCount < MAX_HISTORY) {
        strncpy(gHistory[gHistoryCount].equation, equation, sizeof(gHistory[gHistoryCount].equation) - 1);
        gHistory[gHistoryCount].equation[sizeof(gHistory[gHistoryCount].equation) - 1] = '\0';
        gHistory[gHistoryCount].active = 1;
        gHistoryCount++;
    } else {
        // Shift all entries down to make room for new one
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            gHistory[i] = gHistory[i + 1];
        }
        strncpy(gHistory[MAX_HISTORY - 1].equation, equation, sizeof(gHistory[MAX_HISTORY - 1].equation) - 1);
        gHistory[MAX_HISTORY - 1].equation[sizeof(gHistory[MAX_HISTORY - 1].equation) - 1] = '\0';
        gHistory[MAX_HISTORY - 1].active = 1;
    }
    if (hHistory)
        InvalidateRect(hHistory, NULL, TRUE);
}

// Get history item index at mouse position
static int GetHistoryItemAtPoint(int x, int y, RECT* clientRect) {
    int colWidth = clientRect->right / HISTORY_COLUMNS;
    int col = x / colWidth;
    int row = y / HISTORY_ITEM_HEIGHT;
    
    int availableHeight = clientRect->bottom - clientRect->top;
    int rowsPerCol = (availableHeight - 10) / HISTORY_ITEM_HEIGHT;  // Dynamic row calculation
    
    if (rowsPerCol < 1)
        rowsPerCol = 1;
    
    if (col >= HISTORY_COLUMNS || row >= rowsPerCol)
        return -1;
    
    // Items are displayed from newest (top-left) going down, then to next column
    int index = gHistoryCount - 1 - (col * rowsPerCol + row);
    
    if (index >= 0 && index < gHistoryCount)
        return index;
    
    return -1;
}

// Draw history panel
static void DrawHistory(HDC hdc, RECT* rect) {
    // Fill background
    HBRUSH bgBrush = CreateSolidBrush(RGB(245, 245, 245));
    FillRect(hdc, rect, bgBrush);
    DeleteObject(bgBrush);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 0, 0));
    
    int colWidth = rect->right / HISTORY_COLUMNS;
    int availableHeight = rect->bottom - rect->top;
    int rowsPerCol = (availableHeight - 10) / HISTORY_ITEM_HEIGHT;  // Dynamic row calculation
    
    if (rowsPerCol < 1)
        rowsPerCol = 1;
    
    // Draw items from newest to oldest, left to right, top to bottom
    for (int i = 0; i < gHistoryCount; i++) {
        int itemIndex = gHistoryCount - 1 - i;
        int col = i / rowsPerCol;
        int row = i % rowsPerCol;
        
        if (col >= HISTORY_COLUMNS)
            break;
        
        RECT itemRect;
        itemRect.left = col * colWidth + 5;
        itemRect.top = row * HISTORY_ITEM_HEIGHT + 5;
        itemRect.right = (col + 1) * colWidth - 5;
        itemRect.bottom = (row + 1) * HISTORY_ITEM_HEIGHT - 2;
        
        // Highlight on hover
        if (itemIndex == gHistoryHoverIndex) {
            HBRUSH hoverBrush = CreateSolidBrush(RGB(220, 235, 255));
            FillRect(hdc, &itemRect, hoverBrush);
            DeleteObject(hoverBrush);
            
            HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 150, 200));
            HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
            Rectangle(hdc, itemRect.left - 1, itemRect.top - 1, itemRect.right + 1, itemRect.bottom + 1);
            SelectObject(hdc, oldPen);
            DeleteObject(borderPen);
        }
        
        DrawText(hdc, gHistory[itemIndex].equation, -1, &itemRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
}

// Update history panel visibility based on window width
static void UpdateHistoryVisibility(int width) {
    if (hHistory) {
        if (width >= HISTORY_MIN_WIDTH) {
            ShowWindow(hHistory, SW_SHOW);
        } else {
            ShowWindow(hHistory, SW_HIDE);
        }
    }
}

// History window procedure
LRESULT CALLBACK HistoryProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rect;
            GetClientRect(hWnd, &rect);
            DrawHistory(hdc, &rect);
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_MOUSEMOVE: {
            RECT rect;
            GetClientRect(hWnd, &rect);
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int newHoverIndex = GetHistoryItemAtPoint(x, y, &rect);
            
            if (newHoverIndex != gHistoryHoverIndex) {
                gHistoryHoverIndex = newHoverIndex;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            
            // Track mouse leave to clear hover state when mouse exits window
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hWnd;
            TrackMouseEvent(&tme);
            
            return 0;
        }
        case WM_MOUSELEAVE: {
            if (gHistoryHoverIndex != -1) {
                gHistoryHoverIndex = -1;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return 0;
        }
        case WM_LBUTTONDOWN: {
            RECT rect;
            GetClientRect(hWnd, &rect);
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            int clickedIndex = GetHistoryItemAtPoint(x, y, &rect);
            
            if (clickedIndex >= 0 && clickedIndex < gHistoryCount) {
                // Extract just the equation part (before the '=')
                char equation[256];
                strncpy(equation, gHistory[clickedIndex].equation, sizeof(equation) - 1);
                equation[sizeof(equation) - 1] = '\0';
                
                // Find the '=' and truncate there
                char* equalSign = strstr(equation, " = ");
                if (equalSign)
                    *equalSign = '\0';
                
                SetWindowText(hInput, equation);
                SetWindowText(hOutput, "");
                FocusOnInput();
            }
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Create a button with given label and position, parent window and ID
void AddButton(HWND parent, const char* label, int x, int y, int id) {
    // Buttons are child windows too, so we pass the parent handle and position
    // in client coordinates to keep layout relative to the calculator frame.
    CreateWindow("BUTTON", label,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, 40, 30,
        parent, (HMENU)(intptr_t)id, NULL, NULL);
}

// Set focus on input area to always allow keyboard use.
void FocusOnInput() {
    SetFocus(hInput);
    // Appending an empty string positions the caret at the end, which is a
    // small Win32 trick to avoid calling EM_SETSEL explicitly every time.
    AppendText(hInput, "", 0);
}

// Set focus on output area to allow copy.
void FocusOnOutput() {
    SetFocus(hOutput);
    // We keep this helper symmetrical with FocusOnInput so clipboard routines
    // can briefly switch focus without repeating boilerplate code.
    // AppendText(hInput, "", 0);
}

// Copy logic to get results copied to clipboard on ctrl + c.
void copyResults() {
    // ctrl+C from system (focused window)
    // We briefly move focus to the output field so the clipboard sees the
    // value the user expects, then leave focus management to callers.
    char val[255];
    FocusOnOutput();
    GetWindowText(hOutput, val, sizeof(val));
    if (val[0] != '\0' && strcmp(val, "Error") != 0) {
        const size_t len = strlen(val) + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem) {
            // Clipboards in Win32 keep ownership until something else replaces
            // the data, so we allocate movable global memory and hand the OS a
            // copy that survives after this function returns.
            memcpy(GlobalLock(hMem), val, len);
            GlobalUnlock(hMem);
            OpenClipboard(hwnd);
            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
            // Once ownership is transferred, Windows will free the memory when
            // another clipboard operation replaces it.
            CloseClipboard();
        }
    }
}

// Clear input on c button or keyboard c.
void clearInput() {
    dec = 0;
    equ = 0;
    SetWindowText(hInput, "");
    SetWindowText(hOutput, "");
    gPendingFullFormat = 0;
    gStoredEquation[0] = '\0';
    // Resetting UI and formatting state together prevents stale commas or
    // decimal flags from leaking into the next calculation.
    FocusOnInput();
}

// Set decimal switch variables to handle calculation and round accordingly.
void baseDecimal() {
    dec = 0;
    equ = 1;

    // The evaluator exposes shared globals for decimal tracking, so the GUI
    // clears them here to keep CLI and GUI rounding behavior identical.
    // reset formatting state before new evaluation
    hasDec = 0;
    maxDec = 0;
    offDec = 0;
}

// Handle initial input and keypress.
LRESULT CALLBACK InputProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CHAR) {
        char c = (char)wParam;

        // WM_CHAR arrives after Windows translates keyboard state into a
        // character, which makes it ideal for intercepting backspace logic and
        // filtering the text the control will see.

        if (c == 0x08 || c == 0x7F) {
            DWORD selStart = 0;
            DWORD selEnd = 0;
            SendMessage(hWnd, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);
            if (selStart == selEnd) {
                char text[512];
                int len = GetWindowText(hWnd, text, sizeof(text));
                if (c == 0x08 && selStart > 0 && text[selStart - 1] == ',') {
                    SendMessage(hWnd, EM_SETSEL, selStart - 1, selStart - 1);
                    return 0;
                }
                if (c == 0x7F && selStart < (DWORD)len && text[selStart] == ',') {
                    SendMessage(hWnd, EM_SETSEL, selStart + 1, selStart + 1);
                    return 0;
                }
            }
        }

        if (!(GetKeyState(VK_CONTROL) & 0x8000) && (c == 'd' || c == 'D')) {
            // 'd' or 'D' is pressed: clear input
            clearInput();  // clear equation and calculation
            return 0;
        }
        else if (msg == WM_CHAR && wParam == 13) { // enter key
            // reset formatting state before new evaluation
            baseDecimal();

            char buffer[256];
            GetWindowText(hWnd, buffer, sizeof(buffer));
            remove_format(buffer);  // strip format
            int error;
            double result = evaluate_expr_string(buffer, &error);

            if (error) {
                SetWindowText(hOutput, "Error");
            }
            else {
                char result_str[64];
                int maxDecLocal = 0;
                max_decimals(&maxDecLocal);
                // ready output rendering
                FormatOutput(buffer, result, result_str);
                SetWindowText(hOutput, result_str);
                
                // Add to history
                char historyEntry[512];
                snprintf(historyEntry, sizeof(historyEntry), "%s = %s", buffer, result_str);
                AddToHistory(historyEntry);
            }
            return 0; // handled
        }

        char beforeText[512];
        int trackRemoval = (c == 0x08 || c == 0x7F);
        // We only capture the buffer for backspace/delete to minimize the cost
        // of this routine. Regular character input can skip the comparison.
        if (trackRemoval)
            GetWindowText(hWnd, beforeText, sizeof(beforeText));

        LRESULT result = CallWindowProc(DefaultEditProc, hWnd, msg, wParam, lParam);

        if (trackRemoval) {
            char afterText[512];
            GetWindowText(hWnd, afterText, sizeof(afterText));
            int removalIndex = -1;
            char removedChar = 0;
            // Comparing the buffer before and after the default handler fires
            // tells us whether a delimiter was eaten, letting us restore
            // formatting-friendly characters on the fly.
            if (DetectRemovedDelimiter(beforeText, afterText, &removalIndex, &removedChar)) {
                if (removedChar == ',') {
                    // Reinsert the comma and trigger a local reformat so the
                    // numeric chunk stays grouped instead of collapsing.
                    SendMessage(hWnd, EM_SETSEL, removalIndex, removalIndex);
                    SendMessage(hWnd, EM_REPLACESEL, TRUE, (LPARAM)",");
                    int newCaret = removalIndex;
                    if (c != 0x08)
                        ++newCaret;
                    SendMessage(hWnd, EM_SETSEL, newCaret, newCaret);
                    FormatCurrentNumberSegment(hWnd);
                }
                else if (IsOperatorChar(removedChar) || removedChar == ' ') {
                    // Removing operators affects spacing between terms. Mark
                    // the equation for a full pass to avoid stray spaces.
                    SendMessage(hWnd, EM_SETSEL, removalIndex, removalIndex);
                    SendMessage(hWnd, EM_REPLACESEL, TRUE, (LPARAM)" ");
                    SendMessage(hWnd, EM_SETSEL, removalIndex + 1, removalIndex + 1);
                    GetWindowText(hWnd, gStoredEquation, sizeof(gStoredEquation));
                    gPendingFullFormat = 1;
                }
                else {
                    FormatCurrentNumberSegment(hWnd);
                }
            } else {
                FormatCurrentNumberSegment(hWnd);
            }
            return result;
        }

        int formatSegment = (isdigit((unsigned char)c) != 0) || c == '.';
        if (IsEquationInputChar(c))
            ApplyPendingFullFormat(hWnd, formatSegment);
        else if (formatSegment)
            // Some characters like Enter do not reach the edit control, but we
            // still run the segment formatter when a digit slips through.
            FormatCurrentNumberSegment(hWnd);
        return result;
    }
    else if (msg == WM_PASTE || msg == WM_CUT || msg == WM_CLEAR) {
        LRESULT result = CallWindowProc(DefaultEditProc, hWnd, msg, wParam, lParam);
        // Clipboard edits often inject multiple characters, so we normalize the
        // entire expression to keep formatting predictable.
        FormatEntireEquation(hWnd);
        gPendingFullFormat = 0;
        return result;
    }
    return CallWindowProc(DefaultEditProc, hWnd, msg, wParam, lParam);
}

// Main window procedure to handle events
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // Win32 apps are message-driven. Each case below translates low-level
    // events into calculator actions, which keeps UI and evaluator concerns
    // separated.
    switch (msg) {
        // create buttons
        case WM_CREATE: {
            // Layout happens once when the window is created. We construct the
            // input edit control first so other helpers like FocusOnInput work
            // later in initialization.
            // Use consistent margin for left, right, and bottom
            int margin = 10;
            
            // create input edit control
            hInput = CreateWindow("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                margin, 10, 260, 25,
                hwnd, (HMENU)ID_INPUT, NULL, NULL);

            // create output static text control
            hOutput = CreateWindow("STATIC", "",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                margin, 40, 260, 25,
                hwnd, (HMENU)ID_OUTPUT, NULL, NULL);

            // behave more like a label
            SendMessage(hOutput, EM_SETREADONLY, TRUE, 0);

            // create digit buttons 0-9 in four rows of 3 buttons each
            int row = 120;
            int x = margin + 35;  // start buttons with proper left margin
            for (int i = 0; i <= 11; ++i) {
                char label[2] = { '0' + i, 0 };

                // The button grid mirrors a traditional calculator layout,
                // which keeps muscle memory intact for keyboard and mouse use.
                // add buttons
                if (i == 0) {
                    AddButton(hwnd, label,
                        margin + 80, 200,
                        10 + i
                    );
                }
                else if (i <= 9) {
                    AddButton(hwnd, label,
                        x, 280 - row,
                        10 + i
                    );
                }
                else if (i == 10) {
                    AddButton(hwnd, "c",
                        margin + 35, 200,
                        10 + i
                    );
                }
                else if (i == 11) {
                    AddButton(hwnd, ".",
                        margin + 125, 200,
                        10 + i
                    );
                }

                // increment placement variables accordingly
                if (i != 0 && i % 3 == 0) {
                    row += 40;
                }
                if (i != 0) {
                    if (x >= margin + 125) {
                        x = margin + 35;
                    }
                    else {
                        x += 45;
                    }
                }
            }

            // create operator buttons
            AddButton(hwnd, "+", margin + 180, 80, 30);
            AddButton(hwnd, "-", margin + 220, 80, 31);
            AddButton(hwnd, "x", margin + 180, 120, 32);
            AddButton(hwnd, "/", margin + 220, 120, 33);
            AddButton(hwnd, "^", margin + 220, 160, 34);

            // Operator buttons feed simple single-character strings so the
            // edit control can reuse the same formatting path as keyboard input.

            // create "=" and "+/-" button to evaluate and negate evaluation
            AddButton(hwnd, "+/-", margin + 180, 160, 35);
            AddButton(hwnd, "=",   margin + 180, 200, ID_EQUAL);

            DefaultEditProc = (WNDPROC)SetWindowLongPtr(hInput, GWLP_WNDPROC, (LONG_PTR)InputProc);

            // Create history panel (initially hidden)
            hHistory = CreateWindow("HistoryPanel", "",
                WS_CHILD | WS_BORDER,
                280, 10, HISTORY_WIDTH, 285,
                hwnd, (HMENU)ID_HISTORY, NULL, NULL);

            break;
        }
        // handle button press
        case WM_COMMAND: {
            // WM_COMMAND consolidates button clicks and accelerator actions.
            // The LOWORD identifies which control sent the message.
            if (HIWORD(wParam) == 0 && LOWORD(wParam) == WM_COPY) {
                char val[255];
                copyResults();
                return 0;
            }
            char key = (char)wParam;
            if (LOWORD(wParam) >= 10 && LOWORD(wParam) <= 19) {
                // digit buttons clicked: append digit to input
                char c[2] = { '0' + LOWORD(wParam) - 10, 0 };
                AppendText(hInput, c, 0);
                ApplyPendingFullFormat(hInput, 1);
                // Formatting immediately after a button press keeps commas
                // visible even when the user switches between mouse and keys.
                FocusOnInput();
            }
            else if (LOWORD(wParam) >= 30 && LOWORD(wParam) <= 34) {
                // operator buttons clicked: append operator
                dec = 0;
                const char* ops[] = { "+", "-", "x", "/", "^"};
                char val[255];
                GetWindowText(hOutput, val, sizeof(val));
                // When the last action produced a result, we need to decide
                // whether to chain the new operator or start fresh. The
                // `equ` flag tracks that decision for both mouse and keyboard.
                if (val[0] == '\0' || strcmp(val, "Error") == 0) {
                    AppendText(hInput, ops[LOWORD(wParam) - 30], 0);
                    ApplyPendingFullFormat(hInput, 0);
                    FocusOnInput();
                }
                else {
                    // Comparing strings with == in C checks pointer identity,
                    // so a strcmp would be safer here. The current code still
                    // works because val originates from our buffer, but it is a
                    // good reminder when adapting the pattern elsewhere.
                    if (val == "Error" || equ == 0) {
                        AppendText(hInput, ops[LOWORD(wParam) - 30], 0);
                        ApplyPendingFullFormat(hInput, 0);
                        FocusOnInput();
                    }
                    else {
                        equ = 0;
                        // strcat modifies the destination in place, so we
                        // reuse the output buffer to build the next equation.
                        // In production code it is safer to watch for buffer
                        // limits or use strncat.
                        SetWindowText(hInput, strcat(val, ops[LOWORD(wParam) - 30]));
                        ApplyPendingFullFormat(hInput, 0);
                        FocusOnInput();
                    }
                }
            }
            else if (LOWORD(wParam) == ID_EQUAL) {
                // "=" clicked: evaluate expression in input box
                // Reset the shared decimal counters so FormatOutput knows the
                // highest precision seen in the current equation.
                baseDecimal();

                char buffer[256];
                GetWindowText(hInput, buffer, sizeof(buffer));
                remove_format(buffer);  // strip format
                // The evaluator expects plain digits, so commas introduced for
                // readability must be stripped before parsing.
                int error;
                double result = evaluate_expr_string(buffer, &error);
                if (error) {
                    SetWindowText(hOutput, "Error");  // show error message
                    FocusOnInput();
                }
                else {
                    char result_str[64];
                    // round accordingly process
                    int maxDecLocal = 0;
                    // max_decimals synchronizes output precision with the
                    // parser's internal scan so GUI and CLI show the same
                    // rounded digits.
                    max_decimals(&maxDecLocal);
                    // ready output rendering
                    FormatOutput(buffer, result, result_str);
                    SetWindowText(hOutput, result_str);  // show result
                    
                    // Add to history
                    char historyEntry[512];
                    snprintf(historyEntry, sizeof(historyEntry), "%s = %s", buffer, result_str);
                    AddToHistory(historyEntry);
                    
                    FocusOnInput();
                }
            }
            else if (LOWORD(wParam) == 20) {
                // "c" btn is clicked: clear input
                clearInput();
            }
            else if (LOWORD(wParam) == 21) {
                // "." add decimal ascii decimal value
                if (dec == 0) {
                    dec = 1;
                    char ops[2] = { 46, 0 };
                    // Tracking decimal insertion avoids inputs like 1.2.3,
                    // which the parser would reject with an error.
                    AppendText(hInput, ops, 0);
                    ApplyPendingFullFormat(hInput, 1);
                    FocusOnInput();
                }
            }
            else if (LOWORD(wParam) == 35) {
                // negate current value;
                baseDecimal(); // ready for rounding decimal accordingly
                char val[255];
                GetWindowText(hOutput, val, sizeof(val));
                remove_format(val);  // strip format

                char* end;
                double result = strtod(val, &end);

                if (val[0] != '\0' && *end == '\0') {
                    result = -result;
                    char neg_str[64];
                    // round accordingly process
                    int maxDecLocal = 0;
                    // strtod gives us a double, and reusing FormatOutput keeps
                    // formatting consistent with other code paths.
                    max_decimals(&maxDecLocal);
                    // ready output rendering
                    FormatOutput(val, result, neg_str);
                    SetWindowText(hOutput, neg_str);
                    SetWindowText(hInput, neg_str);  // optional: push it back to input too
                    ApplyPendingFullFormat(hInput, 0);
                    FocusOnInput();
                }
            }

            break;
        }
        // allow copy computed equation to clipboard
        case WM_KEYDOWN: {
            if ((GetKeyState(VK_CONTROL) & 0x8000) && wParam == 'C') {
                char val[255];
                copyResults();
                return 0;
            }
            else {
                // focus on input when window is active
                // By re-focusing the edit control we keep keyboard entry
                // consistent even after clicking buttons elsewhere in the
                // window, which mimics dedicated hardware calculators.
                FocusOnInput();
            }
            break;
        }
        // ensure ctrl + c gets equated results
        case WM_HOTKEY: {
            if (wParam == 1) {
                // same clipboard copy logic
                char val[255];
                GetWindowText(hOutput, val, sizeof(val));
                if (val[0] != '\0' && strcmp(val, "Error") != 0) {
                    const size_t len = strlen(val) + 1;
                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
                    if (hMem) {
                        // Hotkeys deliver their own notifications even if the
                        // focus changes, so we duplicate the clipboard code to
                        // support global Ctrl+C behavior.
                        memcpy(GlobalLock(hMem), val, len);
                        GlobalUnlock(hMem);
                        OpenClipboard(hwnd);
                        EmptyClipboard();
                        SetClipboardData(CF_TEXT, hMem);
                        CloseClipboard();
                    }
                }
                return 0;
            }
            break;
        }
        // allow ctrl + c when prompt focus
        case WM_SETFOCUS: {
            // Registering the hotkey on focus ensures the accelerator belongs
            // to our window only while it is active, matching user intent.
            RegisterHotKey(hwnd, 1, MOD_CONTROL, 'C');
            break;
        }
        // allow ctrl + c when other program focus
        case WM_ACTIVATE: {
            if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) {
                // Window is active — register hotkey
                RegisterHotKey(hwnd, 1, MOD_CONTROL, 'C');
            } else if (LOWORD(wParam) == WA_INACTIVE) {
                // Window is not active — unregister hotkey
                UnregisterHotKey(hwnd, 1);
            }
            else {
                // focus on input when window is active
                // For partial activation states we still restore the caret to
                // give users instant access to typing after system prompts.
                FocusOnInput();
            }
            break;
        }
        // set minimum window size
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            // Calculate minimum window size for client area of 280x300
            RECT minRect = {0, 0, 310, 250};
            AdjustWindowRect(&minRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX, FALSE);
            mmi->ptMinTrackSize.x = minRect.right - minRect.left;
            mmi->ptMinTrackSize.y = minRect.bottom - minRect.top;
            return 0;
        }
        // handle window resize
        case WM_SIZE: {
            if (hInput && hOutput) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                
                // Use equal margins on left, right, and bottom
                int margin = 10;
                
                // Update history panel visibility
                UpdateHistoryVisibility(width);
                
                // Calculate available width for calculator
                int calcWidth = 280;  // fixed calculator width
                int availableWidth = width - (2 * margin);
                
                if (width >= HISTORY_MIN_WIDTH && hHistory) {
                    // Show history panel on the right
                    int historyWidth = width - calcWidth - (3 * margin);
                    if (historyWidth > HISTORY_WIDTH)
                        historyWidth = HISTORY_WIDTH;
                    
                    MoveWindow(hInput, margin, 10, calcWidth, 25, TRUE);
                    MoveWindow(hOutput, margin, 40, calcWidth, 25, TRUE);
                    MoveWindow(hHistory, calcWidth + (2 * margin), 10, historyWidth, height - 20, TRUE);
                } else {
                    // No history panel, use available width for input/output
                    if (availableWidth < calcWidth)
                        availableWidth = calcWidth;
                    MoveWindow(hInput, margin, 10, availableWidth, 25, TRUE);
                    MoveWindow(hOutput, margin, 40, availableWidth, 25, TRUE);
                }
                
                // Buttons remain fixed position
            }
            break;
        }
        // handle keyboard input
        case WM_CHAR: {
            // handle keyboard input for digits, decimal point, operators, and brackets
            char c = (char)wParam;
            // This switch mirrors the button logic so keyboard and mouse users
            // both exercise the same evaluation pathway.
            if (isdigit((unsigned char)c) || c == '.' || c == ')' || c == ']' || c == '}') {
                // append number or closing bracket directly
                char text[2] = { c, 0 };
                AppendText(hInput, text, 0);
                ApplyPendingFullFormat(hInput, (isdigit((unsigned char)c) || c == '.'));
                FocusOnInput();
            }
            else if (c == '+' || c == '-' || c == '/' ||
                     c == '^' || c == 'p' || c == 'P' ||
                     c == 'x' || c == 'X' || c == '*' ||
                     c == '(' || c == '[' || c == '{') {
                // append operator or opening bracket plus a trailing space
                dec = 0;
                char text[3] = { c, ' ', 0 };
                char val[255];
                GetWindowText(hOutput, val, sizeof(val));
                // The trailing space matches the parser's expectation that
                // operators and operands occasionally be separated, improving
                // readability of long expressions entered manually.
                if (val[0] == '\0' || strcmp(val, "Error") == 0) {
                    AppendText(hInput, text, 0);
                    ApplyPendingFullFormat(hInput, 0);
                    FocusOnInput();
                }
                else {
                    if (val == "Error" || equ == 0) {
                        AppendText(hInput, text, 0);
                        ApplyPendingFullFormat(hInput, 0);
                        FocusOnInput();
                    }
                    else {
                        equ = 0;
                        SetWindowText(hInput, strcat(val, text));
                        ApplyPendingFullFormat(hInput, 0);
                        FocusOnInput();
                    }
                }
            }
            else if (c == 13) {
                // "=" clicked: evaluate expression in input box
                char buffer[256];
                GetWindowText(hInput, buffer, sizeof(buffer));
                remove_format(buffer);  // strip format
                int error;
                double result = evaluate_expr_string(buffer, &error);
                if (error)
                    SetWindowText(hOutput, "Error");  // show error message
                else {
                    char result_str[64];
                    int maxDecLocal = 0;
                    // Reusing the same formatting code for keyboard Enter and
                    // the "=" button keeps pathways identical for testing.
                    max_decimals(&maxDecLocal);
                    // ready output rendering
                    FormatOutput(buffer, result, result_str);
                    SetWindowText(hOutput, result_str);  // show result
                    
                    // Add to history
                    char historyEntry[512];
                    snprintf(historyEntry, sizeof(historyEntry), "%s = %s", buffer, result_str);
                    AddToHistory(historyEntry);
                    
                    dec = 0;
                    equ = 1;
                }
            }
            else {
                // focus on input when window is active
                FocusOnInput();
            }
            return 0;  // indicate message processed
        }
        // exit gui on error
        case WM_DESTROY: {
            // Always unregister hotkeys you registered, otherwise Windows will
            // keep them bound and the next instance will fail to claim them.
            UnregisterHotKey(hwnd, 1);
            
            // Child windows are automatically destroyed, but we clear the handle
            if (hHistory) {
                hHistory = NULL;
            }
            
            PostQuitMessage(0);  // exit message loop
            break;
        }
    }

    // default window procedure for unhandled messages
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// MAIN FUNCTION:
//////////////////////////////////////////////////////////////////////////////

// Entry point of Windows application
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow) {
    // register window class
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "CalcGUI";
    wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    wc.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    // RegisterClassEx wires up our message handler and appearance. Without it,
    // CreateWindow would fail because the class name would be unknown to the OS.
    RegisterClassEx(&wc);
    
    // Register history panel window class
    WNDCLASSEX histWc = { 0 };
    histWc.cbSize = sizeof(WNDCLASSEX);
    histWc.lpfnWndProc = HistoryProc;
    histWc.hInstance = hInst;
    histWc.lpszClassName = "HistoryPanel";
    histWc.hCursor = LoadCursor(NULL, IDC_ARROW);
    histWc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassEx(&histWc);

    // create main window
    // Calculate window size to accommodate client area with proper borders
    // Default width includes calculator (280px) + history panel (400px) + margins
    int initialWidth = 280 + HISTORY_WIDTH + 30;  // calculator + history + margins
    RECT rect = {0, 0, initialWidth, 300};
    AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX, FALSE);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;
    
    HWND hwnd = CreateWindow("CalcGUI", "ccal",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_MAXIMIZEBOX,
                CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
                NULL, NULL, hInst, NULL);

    if (!hwnd) return 1;

    // show and update window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // The message loop pulls events off the queue until WM_QUIT arrives,
    // letting WndProc handle user interactions synchronously.

    // message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        // TranslateMessage turns virtual-key presses into WM_CHAR, which our
        // input subclass relies on for formatting.
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up window classes
    UnregisterClass("HistoryPanel", hInst);
    UnregisterClass("CalcGUI", hInst);

    return 0;
}