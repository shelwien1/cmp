// Windows API stub implementations for Linux compilation
// These are dummy implementations that allow linking but won't actually work
// This file exists solely to enable compilation and syntax checking on Linux

#include "windows.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Global variables for command-line arguments
int __argc = 0;
char** __argv = nullptr;

// ===== Module functions =====
HINSTANCE GetModuleHandle(LPCSTR) { return (HINSTANCE)0x400000; }

// ===== Window class functions =====
int RegisterClass(const WNDCLASS*) { return 1; }

// ===== Window functions =====
HWND CreateWindow(LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HANDLE, HINSTANCE, LPVOID) {
    return (HWND)0x12345;
}
int ShowWindow(HWND, int) { return 1; }
int SetFocus(HWND) { return 1; }
int GetWindowRect(HWND, RECT* lpRect) {
    if (lpRect) {
        lpRect->left = lpRect->top = 0;
        lpRect->right = 1920;
        lpRect->bottom = 1080;
    }
    return 1;
}
int SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return 1; }
int GetWindowPlacement(HWND, WINDOWPLACEMENT* lpwndpl) {
    if (lpwndpl) memset(lpwndpl, 0, sizeof(WINDOWPLACEMENT));
    return 1;
}
int SetWindowPlacement(HWND, const WINDOWPLACEMENT*) { return 1; }
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
int InvalidateRect(HWND, const RECT*, int) { return 1; }
int UpdateWindow(HWND) { return 1; }

// ===== Message loop functions =====
int GetMessage(MSG* lpMsg, HWND, UINT, UINT) {
    // Return 0 to exit message loop immediately
    if (lpMsg) memset(lpMsg, 0, sizeof(MSG));
    return 0;
}
int DispatchMessage(const MSG*) { return 0; }
int TranslateMessage(const MSG*) { return 0; }
void PostQuitMessage(int) { }

// ===== GDI functions =====
HDC GetDC(HWND) { return (HDC)0x1001; }
HDC CreateCompatibleDC(HDC) { return (HDC)0x1002; }
HBITMAP CreateDIBSection(HDC, const BITMAPINFO*, UINT, void** ppvBits, HANDLE, DWORD) {
    // Allocate a dummy buffer
    if (ppvBits) *ppvBits = malloc(1920 * 1080 * 4);
    return (HBITMAP)0x2001;
}
HGDIOBJ SelectObject(HDC, HGDIOBJ h) { return h; }
int BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD) { return 1; }
int DeleteObject(HGDIOBJ) { return 1; }
HDC BeginPaint(HWND, PAINTSTRUCT* lpPaint) {
    if (lpPaint) memset(lpPaint, 0, sizeof(PAINTSTRUCT));
    return (HDC)0x1003;
}
int EndPaint(HWND, const PAINTSTRUCT*) { return 1; }
HGDIOBJ ExtCreatePen(DWORD, DWORD, const LOGBRUSH*, DWORD, const DWORD*) {
    return (HGDIOBJ)0x3001;
}
int MoveToEx(HDC, int, int, void*) { return 1; }
int LineTo(HDC, int, int) { return 1; }
int Polyline(HDC, const POINT*, int) { return 1; }
int Rectangle(HDC, int, int, int, int) { return 1; }
int TextOut(HDC, int, int, LPCSTR, int) { return 1; }
int GetTextExtentPoint32(HDC, LPCSTR lpString, int c, SIZE* psizl) {
    if (psizl) {
        psizl->cx = c * 8;  // Assume 8 pixels per char
        psizl->cy = 16;     // Assume 16 pixel height
    }
    return 1;
}
int ReleaseDC(HWND, HDC) { return 1; }
int DeleteDC(HDC) { return 1; }
int SetBkMode(HDC, int) { return 1; }
UINT SetTextAlign(HDC, UINT align) { return align; }
DWORD SetBkColor(HDC, DWORD color) { return color; }
DWORD SetTextColor(HDC, DWORD color) { return color; }
int FillRect(HDC, const RECT*, HBRUSH) { return 1; }
HBRUSH CreateSolidBrush(DWORD) { return (HBRUSH)0x4001; }

// ===== Font functions =====
HFONT CreateFontIndirect(const LOGFONT*) { return (HFONT)0x5001; }
int ChooseFont(CHOOSEFONT*) { return 0; }  // Return 0 = user cancelled

// ===== Icon functions =====
HICON LoadIcon(HINSTANCE, LPCSTR) { return (HICON)0x6001; }
LONG SetClassLong(HWND, int, LONG dwNewLong) { return dwNewLong; }
LONG_PTR SetClassLongPtr(HWND, int, LONG_PTR dwNewLong) { return dwNewLong; }

// ===== System functions =====
int GetSystemMetrics(int nIndex) {
    switch(nIndex) {
        case 0: return 1920;  // SM_CXSCREEN
        case 1: return 1080;  // SM_CYSCREEN
        case 7: return 4;     // SM_CXFIXEDFRAME
        case 8: return 4;     // SM_CYFIXEDFRAME
        case 4: return 20;    // SM_CYCAPTION
        case 5: return 1;     // SM_CXBORDER
        default: return 0;
    }
}
int SystemParametersInfo(UINT uiAction, UINT, LPVOID pvParam, UINT) {
    if (uiAction == 48 && pvParam) {  // SPI_GETWORKAREA
        RECT* rc = (RECT*)pvParam;
        rc->left = rc->top = 0;
        rc->right = 1920;
        rc->bottom = 1040;  // Screen minus taskbar
    }
    return 1;
}

// ===== Keyboard/mouse functions =====
short GetKeyState(int) { return 0; }

// ===== Timer functions =====
UINT SetTimer(HWND, UINT, UINT, void*) { return 1; }
DWORD GetTickCount(void) {
    static DWORD ticks = 0;
    return ticks++;
}

// ===== Thread functions =====
HANDLE CreateThread(SECURITY_ATTRIBUTES*, SIZE_T, DWORD (*)(LPVOID), LPVOID, DWORD, DWORD*) {
    return (HANDLE)0x7001;
}
DWORD WaitForSingleObject(HANDLE, DWORD) { return 0; }
void Sleep(DWORD) { }
int CloseHandle(HANDLE) { return 1; }

// ===== File I/O functions =====
HANDLE CreateFileA(LPCSTR lpFileName, DWORD, DWORD, SECURITY_ATTRIBUTES*, DWORD, DWORD, HANDLE) {
    FILE* f = fopen(lpFileName, "rb");
    return f ? (HANDLE)f : INVALID_HANDLE_VALUE;
}
HANDLE CreateFileW(LPCWSTR, DWORD, DWORD, SECURITY_ATTRIBUTES*, DWORD, DWORD, HANDLE) {
    return INVALID_HANDLE_VALUE;
}
int ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
             LPDWORD lpNumberOfBytesRead, OVERLAPPED*) {
    if (!hFile || hFile == INVALID_HANDLE_VALUE) return 0;
    size_t n = fread(lpBuffer, 1, nNumberOfBytesToRead, (FILE*)hFile);
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = n;
    return 1;
}
int WriteFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToWrite,
              LPDWORD lpNumberOfBytesWritten, OVERLAPPED*) {
    if (!hFile || hFile == INVALID_HANDLE_VALUE) return 0;
    size_t n = fwrite(lpBuffer, 1, nNumberOfBytesToWrite, (FILE*)hFile);
    if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = n;
    return 1;
}
DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG* lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    if (!hFile || hFile == INVALID_HANDLE_VALUE) return 0xFFFFFFFF;

    long long offset = lDistanceToMove;
    if (lpDistanceToMoveHigh) {
        offset |= ((long long)*lpDistanceToMoveHigh) << 32;
    }

    int whence = SEEK_SET;
    if (dwMoveMethod == 1) whence = SEEK_CUR;
    else if (dwMoveMethod == 2) whence = SEEK_END;

    fseek((FILE*)hFile, offset, whence);
    long pos = ftell((FILE*)hFile);

    if (lpDistanceToMoveHigh) {
        *lpDistanceToMoveHigh = (pos >> 32) & 0xFFFFFFFF;
    }
    return pos & 0xFFFFFFFF;
}
DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) {
    if (!hFile || hFile == INVALID_HANDLE_VALUE) return 0xFFFFFFFF;

    FILE* f = (FILE*)hFile;
    long current = ftell(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, current, SEEK_SET);

    if (lpFileSizeHigh) *lpFileSizeHigh = 0;
    return size;
}
int CreateDirectoryW(LPCWSTR, SECURITY_ATTRIBUTES*) { return 1; }

// ===== Registry functions =====
LONG RegOpenKeyEx(HKEY, LPCSTR, DWORD, DWORD, HKEY* phkResult) {
    if (phkResult) *phkResult = (HKEY)0x8001;
    return 0;  // ERROR_SUCCESS
}
LONG RegQueryValueEx(HKEY, LPCSTR, LPDWORD, LPDWORD lpType, BYTE*, LPDWORD lpcbData) {
    if (lpType) *lpType = 3;  // REG_BINARY
    if (lpcbData) *lpcbData = 0;
    return 2;  // ERROR_FILE_NOT_FOUND
}
LONG RegSetValueEx(HKEY, LPCSTR, DWORD, DWORD, const BYTE*, DWORD) {
    return 0;  // ERROR_SUCCESS
}
LONG RegCloseKey(HKEY) { return 0; }
LONG RegCreateKeyEx(HKEY, LPCSTR, DWORD, LPSTR, DWORD, DWORD, SECURITY_ATTRIBUTES*,
                    HKEY* phkResult, LPDWORD lpdwDisposition) {
    if (phkResult) *phkResult = (HKEY)0x8002;
    if (lpdwDisposition) *lpdwDisposition = 1;  // REG_CREATED_NEW_KEY
    return 0;  // ERROR_SUCCESS
}

// ===== Error functions =====
static DWORD g_LastError = 0;
DWORD GetLastError(void) { return g_LastError; }
void SetLastError(DWORD dwErrCode) { g_LastError = dwErrCode; }
DWORD FormatMessage(DWORD, LPVOID, DWORD, DWORD, LPSTR lpBuffer, DWORD nSize, void*) {
    if (lpBuffer && nSize > 0) {
        strncpy(lpBuffer, "Error (stub)", nSize);
        return strlen(lpBuffer);
    }
    return 0;
}
DWORD FormatMessageW(DWORD, LPVOID, DWORD, DWORD, LPWSTR lpBuffer, DWORD nSize, void*) {
    if (lpBuffer && nSize > 0) {
        lpBuffer[0] = L'E';
        lpBuffer[1] = 0;
        return 1;
    }
    return 0;
}

// ===== String functions =====
int WideCharToMultiByte(UINT, DWORD, LPCWSTR lpWideCharStr, int cchWideChar,
                        LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR, int*) {
    if (!lpMultiByteStr) return cchWideChar;
    int n = (cchWideChar < cbMultiByte) ? cchWideChar : cbMultiByte;
    for (int i = 0; i < n; i++) {
        lpMultiByteStr[i] = (char)lpWideCharStr[i];
    }
    return n;
}
DWORD GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR*) {
    if (!lpBuffer) return wcslen(lpFileName) + 10;
    wcsncpy(lpBuffer, L"C:\\", nBufferLength);
    wcsncat(lpBuffer, lpFileName, nBufferLength - 4);
    return wcslen(lpBuffer);
}

// ===== Memory functions =====
void* LocalFree(void* hMem) {
    free(hMem);
    return nullptr;
}

// ===== Entry point wrapper =====
// WinMain is the Windows entry point, but on Linux we need main()
extern int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

int main(int argc, char** argv) {
    // Set up global argc/argv for Windows compatibility
    __argc = argc;
    __argv = argv;

    // Call WinMain with dummy parameters
    return WinMain((HINSTANCE)0x400000, nullptr, (LPSTR)"", 1);
}
