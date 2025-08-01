﻿// ccal_gui.c
// Simple Windows GUI calculator using the calc.c expression evaluator
// Provides digit and operator buttons and input box; keyboard also supported

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include "resource.h"
#include "remove_format.h"

// Declare the external evaluate function implemented in calc.c.
extern double evaluate_expr_string(const char* expr, int* error);

// GLOBAL ELEMENTS:
//////////////////////////////////////////////////////////////////////////////

// Control IDs for window elements.
#define ID_INPUT   1
#define ID_OUTPUT  2
#define ID_EQUAL   3

HWND hwnd, hInput, hOutput;  // handles to input and output controls
WNDPROC DefaultEditProc;

#define BTN_COUNT 20
HWND hButtons[BTN_COUNT];  // store up to 20 buttons

// Global variables.
int dec = 0;    // no duplicate decimals 
int equ = 0;    // allow continue equation

// SUPPORT FUNCTIONS:
//////////////////////////////////////////////////////////////////////////////

// Append text to an edit control, optionally adding a space after
void AppendText(HWND hWnd, const char* text, int add_space) {
    int len = GetWindowTextLength(hWnd);
    // set cursor to end of text
    SendMessage(hWnd, EM_SETSEL, len, len);
    // insert new text at cursor position
    SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)text);
    if (add_space)
        SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)" ");
}

// Create a button with given label and position, parent window and ID
void AddButton(HWND parent, const char* label, int x, int y, int id) {
    CreateWindow("BUTTON", label,
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        x, y, 40, 30,
        parent, (HMENU)(intptr_t)id, NULL, NULL);
}

// Set focus on input area to always allow keyboard use.
void FocusOnInput() {
    SetFocus(hInput);
    AppendText(hInput, "", 0);
}

// Set focus on output area to allow copy.
void FocusOnOutput() {
    SetFocus(hOutput);
    // AppendText(hInput, "", 0);
}

// Copy logic to get results copied to clipboard on ctrl + c.
void copyResults() {
    // ctrl+C from system (focused window)
    char val[255];
    FocusOnOutput();
    GetWindowText(hOutput, val, sizeof(val));
    if (val[0] != '\0' && strcmp(val, "Error") != 0) {
        const size_t len = strlen(val) + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem) {
            memcpy(GlobalLock(hMem), val, len);
            GlobalUnlock(hMem);
            OpenClipboard(hwnd);
            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
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
    FocusOnInput();
}

// Set decimal switch variables to handle calculation and round accordingly.
void baseDecimal() { 
    dec = 0;
    equ = 1;

    // reset formatting state before new evaluation
    hasDec = 0;
    maxDec = 0;
    offDec = 0;
}

// Handle initial input and keypress.
LRESULT CALLBACK InputProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CHAR) {
        char c = (char)wParam;

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
            }
            return 0; // handled
        }
    }
    return CallWindowProc(DefaultEditProc, hWnd, msg, wParam, lParam);
}

// Main window procedure to handle events
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        // create buttons
        case WM_CREATE: {
            // create input edit control
            hInput = CreateWindow("EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                10, 10, 380, 25,
                hwnd, (HMENU)ID_INPUT, NULL, NULL);
            
            // behave more like a label
            SendMessage(hOutput, EM_SETREADONLY, TRUE, 0);

            // create output static text control
            hOutput = CreateWindow("STATIC", "",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 40, 380, 25,
                hwnd, (HMENU)ID_OUTPUT, NULL, NULL);

            // create digit buttons 0-9 in four rows of 3 buttons each
            int row = 120;
            int x = 45;
            for (int i = 0; i <= 11; ++i) {
                char label[2] = { '0' + i, 0 };

                // add buttons
                if (i == 0) {
                    AddButton(hwnd, label,
                        90, 200,
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
                        45, 200,
                        10 + i
                    );
                }
                else if (i == 11) {
                    AddButton(hwnd, ".",
                        135, 200,
                        10 + i
                    );
                }

                // increment placement variables accordingly
                if (i != 0 && i % 3 == 0) {
                    row += 40;
                }
                if (i != 0) {
                    if (x >= 135) {
                        x = 45;
                    }
                    else {
                        x += 45;
                    }
                }
            }

            // create operator buttons
            AddButton(hwnd, "+", 190, 80, 30);
            AddButton(hwnd, "-", 230, 80, 31);
            AddButton(hwnd, "x", 190, 120, 32);
            AddButton(hwnd, "/", 230, 120, 33);
            AddButton(hwnd, "^", 230, 160, 34);
            
            // create "=" and "+/-" button to evaluate and negate evaluation
            AddButton(hwnd, "+/-", 190, 160, 35);
            AddButton(hwnd, "=",   190, 200, ID_EQUAL);

            DefaultEditProc = (WNDPROC)SetWindowLongPtr(hInput, GWLP_WNDPROC, (LONG_PTR)InputProc);

            break;
        }
        // handle button press
        case WM_COMMAND: {
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
                FocusOnInput();
            }
            else if (LOWORD(wParam) >= 30 && LOWORD(wParam) <= 34) {
                // operator buttons clicked: append operator
                dec = 0;
                const char* ops[] = { "+", "-", "x", "/", "^"};
                char val[255];
                GetWindowText(hOutput, val, sizeof(val));
                if (val[0] == '\0' || strcmp(val, "Error") == 0) {
                    AppendText(hInput, ops[LOWORD(wParam) - 30], 0);
                    FocusOnInput();
                }
                else {
                    if (val == "Error" || equ == 0) {
                        AppendText(hInput, ops[LOWORD(wParam) - 30], 0);
                        FocusOnInput();
                    }
                    else {
                        equ = 0;
                        SetWindowText(hInput, strcat(val, ops[LOWORD(wParam) - 30]));
                        FocusOnInput();
                    }
                }
            }
            else if (LOWORD(wParam) == ID_EQUAL) {
                // "=" clicked: evaluate expression in input box
                baseDecimal();

                char buffer[256];
                GetWindowText(hInput, buffer, sizeof(buffer));
                remove_format(buffer);  // strip format
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
                    max_decimals(&maxDecLocal);
                    // ready output rendering
                    FormatOutput(buffer, result, result_str);
                    SetWindowText(hOutput, result_str);  // show result
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
                    AppendText(hInput, ops, 0);
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
                    max_decimals(&maxDecLocal);
                    // ready output rendering
                    FormatOutput(val, result, neg_str);                    
                    SetWindowText(hOutput, neg_str);
                    SetWindowText(hInput, neg_str);  // optional: push it back to input too
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
                FocusOnInput();
            }
            break;
        }
        // handle keyboard input
        case WM_CHAR: {
            // handle keyboard input for digits, decimal point, operators, and brackets
            char c = (char)wParam;
            if (isdigit(c) || c == '.' || c == ')' || c == ']' || c == '}') {
                // append number or closing bracket directly
                char text[2] = { c, 0 };
                AppendText(hInput, text, 0);
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
                if (val[0] == '\0' || strcmp(val, "Error") == 0) {
                    AppendText(hInput, text, 0);
                    FocusOnInput();
                }
                else {
                    if (val == "Error" || equ == 0) {
                        AppendText(hInput, text, 0);
                        FocusOnInput();
                    }
                    else {
                        equ = 0;
                        SetWindowText(hInput, strcat(val, text));
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
                    max_decimals(&maxDecLocal);
                    // ready output rendering
                    FormatOutput(buffer, result, result_str);
                    SetWindowText(hOutput, result_str);  // show result
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
            UnregisterHotKey(hwnd, 1);
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
    RegisterClassEx(&wc);

    // create main window
    HWND hwnd = CreateWindow("CalcGUI", "ccal",
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, // width, height
                NULL, NULL, hInst, NULL);
    
    if (!hwnd) return 1;

    // show and update window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}