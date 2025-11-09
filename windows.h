// Dummy windows.h for Linux compilation testing
// This file provides stub declarations for all Windows API functions and types used by cmp
// It allows syntax checking and compilation on Linux, but the resulting binary won't run

#ifndef WINDOWS_H_DUMMY
#define WINDOWS_H_DUMMY

// Basic Windows types
typedef void* HANDLE;
typedef void* HWND;
typedef void* HDC;
typedef void* HGDIOBJ;
typedef void* HBITMAP;
typedef void* HFONT;
typedef void* HICON;
typedef void* HINSTANCE;
typedef void* HKEY;
typedef void* HBRUSH;
typedef void* LPVOID;
typedef unsigned long DWORD;
typedef unsigned long ULONG;
typedef long LONG;
typedef unsigned int UINT;
typedef int INT;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef char CHAR;
typedef wchar_t WCHAR;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef long long LONG_PTR;
typedef unsigned long long ULONG_PTR;
typedef unsigned long long SIZE_T;
typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;
typedef const char* LPCSTR;
typedef const wchar_t* LPCWSTR;
typedef char* LPSTR;
typedef wchar_t* LPWSTR;
typedef DWORD* LPDWORD;
typedef LONG* PLONG;

// Calling convention macros (empty on non-Windows)
#define WINAPI
#define CALLBACK
#define __stdcall

// Boolean constants
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

// Handle definitions
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)

// Rectangle structure
typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT;

// Point structure
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT;

// Window placement structure
typedef struct tagWINDOWPLACEMENT {
    UINT length;
    UINT flags;
    UINT showCmd;
    POINT ptMinPosition;
    POINT ptMaxPosition;
    RECT rcNormalPosition;
} WINDOWPLACEMENT;

// Message structure
typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    int pt_x;
    int pt_y;
} MSG;

// Paint structure
typedef struct tagPAINTSTRUCT {
    HDC hdc;
    int fErase;
    RECT rcPaint;
    int fRestore;
    int fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT;

// Font structure
typedef struct tagLOGFONT {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    CHAR lfFaceName[32];
} LOGFONT;

// Bitmap info structures
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO;

// Brush structure
typedef struct tagLOGBRUSH {
    UINT lbStyle;
    DWORD lbColor;
    ULONG lbHatch;
} LOGBRUSH;

// Font chooser structure
typedef struct tagCHOOSEFONT {
    DWORD lStructSize;
    HWND hwndOwner;
    HDC hDC;
    LOGFONT* lpLogFont;
    INT iPointSize;
    DWORD Flags;
    DWORD rgbColors;
    LPARAM lCustData;
    void* lpfnHook;
    LPCSTR lpTemplateName;
    HINSTANCE hInstance;
    LPSTR lpszStyle;
    WORD nFontType;
    WORD ___MISSING_ALIGNMENT__;
    INT nSizeMin;
    INT nSizeMax;
} CHOOSEFONT;

// Window class structure
typedef struct tagWNDCLASS {
    UINT style;
    LRESULT (*lpfnWndProc)(HWND, UINT, WPARAM, LPARAM);
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HGDIOBJ hCursor;
    HGDIOBJ hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
} WNDCLASS;

// Size structure
typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE;

// Security attributes
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    int bInheritHandle;
} SECURITY_ATTRIBUTES;

// Overlapped structure (for async I/O)
typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        };
        LPVOID Pointer;
    };
    HANDLE hEvent;
} OVERLAPPED;

// File time structure
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

// Window messages
#define WM_NULL                 0x0000
#define WM_CREATE               0x0001
#define WM_DESTROY              0x0002
#define WM_CLOSE                0x0010
#define WM_QUIT                 0x0012
#define WM_PAINT                0x000F
#define WM_KEYDOWN              0x0100
#define WM_KEYUP                0x0101
#define WM_CHAR                 0x0102
#define WM_SYSKEYDOWN           0x0104
#define WM_SYSKEYUP             0x0105
#define WM_TIMER                0x0113
#define WM_MOUSEWHEEL           0x020A

// PeekMessage options
#define PM_NOREMOVE             0x0000
#define PM_REMOVE               0x0001

// Virtual key codes
#define VK_BACK                 0x08
#define VK_TAB                  0x09
#define VK_RETURN               0x0D
#define VK_ESCAPE               0x1B
#define VK_SPACE                0x20
#define VK_PRIOR                0x21  // Page Up
#define VK_NEXT                 0x22  // Page Down
#define VK_END                  0x23
#define VK_HOME                 0x24
#define VK_LEFT                 0x25
#define VK_UP                   0x26
#define VK_RIGHT                0x27
#define VK_DOWN                 0x28
#define VK_INSERT               0x2D
#define VK_DELETE               0x2E
#define VK_F1                   0x70
#define VK_F5                   0x74
#define VK_F6                   0x75
#define VK_CONTROL              0x11
#define VK_SHIFT                0x10
#define VK_MENU                 0x12  // Alt key
#define VK_ADD                  0x6B
#define VK_SUBTRACT             0x6D
#define VK_OEM_PLUS             0xBB
#define VK_OEM_MINUS            0xBD

// Window styles
#define WS_OVERLAPPED           0x00000000
#define WS_POPUP                0x80000000
#define WS_CHILD                0x40000000
#define WS_MINIMIZE             0x20000000
#define WS_VISIBLE              0x10000000
#define WS_DISABLED             0x08000000
#define WS_CLIPSIBLINGS         0x04000000
#define WS_CLIPCHILDREN         0x02000000
#define WS_MAXIMIZE             0x01000000
#define WS_CAPTION              0x00C00000
#define WS_BORDER               0x00800000
#define WS_DLGFRAME             0x00400000
#define WS_VSCROLL              0x00200000
#define WS_HSCROLL              0x00100000
#define WS_SYSMENU              0x00080000
#define WS_THICKFRAME           0x00040000
#define WS_GROUP                0x00020000
#define WS_TABSTOP              0x00010000

// Show window constants
#define SW_HIDE                 0
#define SW_SHOW                 5

// File constants
#define GENERIC_READ            0x80000000
#define GENERIC_WRITE           0x40000000
#define FILE_SHARE_READ         0x00000001
#define FILE_SHARE_WRITE        0x00000002
#define FILE_SHARE_DELETE       0x00000004
#define CREATE_NEW              1
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define OPEN_ALWAYS             4
#define FILE_ATTRIBUTE_NORMAL   0x00000080
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000
#define FILE_BEGIN              0
#define FILE_CURRENT            1
#define FILE_END                2

// GDI constants
#define BI_RGB                  0
#define DIB_RGB_COLORS          0
#define SRCCOPY                 0x00CC0020
#define PS_SOLID                0
#define PS_DASH                 1
#define PS_GEOMETRIC            0x00010000
#define BS_SOLID                0
#define HS_CROSS                2
#define HS_HORIZONTAL           0

// System metrics
#define SM_CXSCREEN             0
#define SM_CYSCREEN             1
#define SM_CXFIXEDFRAME         7
#define SM_CYFIXEDFRAME         8
#define SM_CYCAPTION            4
#define SM_CXBORDER             5

// System parameters
#define SPI_GETWORKAREA         48

// Font constants
#define FW_NORMAL               400
#define FW_BOLD                 700
#define DEFAULT_CHARSET         1
#define OUT_DEFAULT_PRECIS      0
#define CLIP_DEFAULT_PRECIS     0
#define DEFAULT_QUALITY         0
#define FIXED_PITCH             1
#define FF_DONTCARE             0

// Color macros
#define RGB(r,g,b)              ((DWORD)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

// Word extraction macros
#define LOWORD(l)               ((WORD)(((DWORD)(l)) & 0xffff))
#define HIWORD(l)               ((WORD)((((DWORD)(l)) >> 16) & 0xffff))

// MSVC-specific globals (command-line arguments)
extern int __argc;
extern char** __argv;

// FormatMessage constants
#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200

// Language constants
#define LANG_NEUTRAL                   0x00
#define SUBLANG_DEFAULT                0x01
#define MAKELANGID(p, s)               ((WORD)(((WORD)(s) << 10) | (WORD)(p)))

// Code page constants
#define CP_OEMCP                       1
#define CP_UTF8                        65001

// Clipboard constants
#define CF_TEXT                        1
#define CF_UNICODETEXT                 13
#define GMEM_MOVEABLE                  0x0002

// Registry constants
#define HKEY_CURRENT_USER       ((HKEY)0x80000001)
#define KEY_READ                0x20019
#define KEY_WRITE               0x20006
#define KEY_ALL_ACCESS          0xF003F
#define REG_SZ                  1
#define REG_BINARY              3
#define REG_DWORD               4
#define REG_OPTION_NON_VOLATILE 0x00000000
#define ERROR_SUCCESS           0

// Class long constants
#define GCL_HICON               (-14)
#define GCLP_HICON              (-14)

// Window position flags
#define SWP_NOMOVE              0x0002
#define SWP_NOSIZE              0x0001
#define SWP_NOZORDER            0x0004
#define SWP_NOACTIVATE          0x0010

// Resource constants
#define MAKEINTRESOURCE(i)      ((LPSTR)((ULONG_PTR)((WORD)(i))))

// Thread constants
#define INFINITE                0xFFFFFFFF

// Font chooser flags
#define CF_SCREENFONTS          0x00000001
#define CF_INITTOLOGFONTSTRUCT  0x00000040
#define CF_FIXEDPITCHONLY       0x00004000
#define CF_NOVERTFONTS          0x01000000

// Text alignment flags
#define TA_LEFT                 0
#define TA_TOP                  0
#define TA_RIGHT                2
#define TA_BOTTOM               8

// Background mode constants
#define TRANSPARENT             1
#define OPAQUE                  2

// ===== FUNCTION DECLARATIONS =====

#ifdef __cplusplus
extern "C" {
#endif

// Module functions
HINSTANCE GetModuleHandle(LPCSTR lpModuleName);

// Window class functions
int RegisterClass(const WNDCLASS* lpWndClass);

// Window functions
HWND CreateWindow(LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
                  int x, int y, int nWidth, int nHeight,
                  HWND hWndParent, HANDLE hMenu, HINSTANCE hInstance, LPVOID lpParam);
int ShowWindow(HWND hWnd, int nCmdShow);
int SetFocus(HWND hWnd);
int GetWindowRect(HWND hWnd, RECT* lpRect);
int SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
int GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT* lpwndpl);
int SetWindowPlacement(HWND hWnd, const WINDOWPLACEMENT* lpwndpl);
LRESULT DefWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int InvalidateRect(HWND hWnd, const RECT* lpRect, int bErase);
int UpdateWindow(HWND hWnd);

// Message loop functions
int GetMessage(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
int PeekMessage(MSG* lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
int DispatchMessage(const MSG* lpMsg);
int TranslateMessage(const MSG* lpMsg);
void PostQuitMessage(int nExitCode);

// GDI functions
HDC GetDC(HWND hWnd);
HDC CreateCompatibleDC(HDC hdc);
HBITMAP CreateDIBSection(HDC hdc, const BITMAPINFO* lpbmi, UINT usage,
                         void** ppvBits, HANDLE hSection, DWORD offset);
HGDIOBJ SelectObject(HDC hdc, HGDIOBJ h);
int BitBlt(HDC hdcDest, int x, int y, int cx, int cy,
           HDC hdcSrc, int x1, int y1, DWORD rop);
int DeleteObject(HGDIOBJ ho);
HDC BeginPaint(HWND hWnd, PAINTSTRUCT* lpPaint);
int EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint);
HGDIOBJ ExtCreatePen(DWORD dwPenStyle, DWORD dwWidth, const LOGBRUSH* plbrush,
                     DWORD dwStyleCount, const DWORD* lpStyle);
int MoveToEx(HDC hdc, int x, int y, void* lppt);
int LineTo(HDC hdc, int x, int y);
int Polyline(HDC hdc, const POINT* apt, int cpt);
int Rectangle(HDC hdc, int left, int top, int right, int bottom);
int TextOut(HDC hdc, int x, int y, LPCSTR lpString, int c);
int GetTextExtentPoint32(HDC hdc, LPCSTR lpString, int c, SIZE* psizl);
int ReleaseDC(HWND hWnd, HDC hDC);
int DeleteDC(HDC hdc);
int SetBkMode(HDC hdc, int mode);
UINT SetTextAlign(HDC hdc, UINT align);
DWORD SetBkColor(HDC hdc, DWORD color);
DWORD SetTextColor(HDC hdc, DWORD color);
int FillRect(HDC hDC, const RECT* lprc, HBRUSH hbr);
HBRUSH CreateSolidBrush(DWORD color);

// Font functions
HFONT CreateFontIndirect(const LOGFONT* lplf);
int ChooseFont(CHOOSEFONT* lpcf);

// Icon functions
HICON LoadIcon(HINSTANCE hInstance, LPCSTR lpIconName);
LONG SetClassLong(HWND hWnd, int nIndex, LONG dwNewLong);
LONG_PTR SetClassLongPtr(HWND hWnd, int nIndex, LONG_PTR dwNewLong);

// System functions
int GetSystemMetrics(int nIndex);
int SystemParametersInfo(UINT uiAction, UINT uiParam, LPVOID pvParam, UINT fWinIni);

// Keyboard/mouse functions
short GetKeyState(int nVirtKey);

// Timer functions
UINT SetTimer(HWND hWnd, UINT nIDEvent, UINT uElapse, void* lpTimerFunc);
DWORD GetTickCount(void);

// Thread functions
HANDLE CreateThread(SECURITY_ATTRIBUTES* lpThreadAttributes, SIZE_T dwStackSize,
                    DWORD (*lpStartAddress)(LPVOID), LPVOID lpParameter,
                    DWORD dwCreationFlags, DWORD* lpThreadId);
DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
void Sleep(DWORD dwMilliseconds);
int CloseHandle(HANDLE hObject);

// File I/O functions
HANDLE CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   SECURITY_ATTRIBUTES* lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HANDLE CreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   SECURITY_ATTRIBUTES* lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
int ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
             LPDWORD lpNumberOfBytesRead, OVERLAPPED* lpOverlapped);
int WriteFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToWrite,
              LPDWORD lpNumberOfBytesWritten, OVERLAPPED* lpOverlapped);
DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG* lpDistanceToMoveHigh,
                     DWORD dwMoveMethod);
DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
int CreateDirectoryW(LPCWSTR lpPathName, SECURITY_ATTRIBUTES* lpSecurityAttributes);

// Registry functions
LONG RegOpenKeyEx(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, DWORD samDesired, HKEY* phkResult);
LONG RegQueryValueEx(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType,
                     BYTE* lpData, LPDWORD lpcbData);
LONG RegSetValueEx(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType,
                   const BYTE* lpData, DWORD cbData);
LONG RegCloseKey(HKEY hKey);
LONG RegCreateKeyEx(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions,
                    DWORD samDesired, SECURITY_ATTRIBUTES* lpSecurityAttributes,
                    HKEY* phkResult, LPDWORD lpdwDisposition);

// Error functions
DWORD GetLastError(void);
void SetLastError(DWORD dwErrCode);
DWORD FormatMessage(DWORD dwFlags, LPVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId,
                    LPSTR lpBuffer, DWORD nSize, void* Arguments);
DWORD FormatMessageW(DWORD dwFlags, LPVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId,
                     LPWSTR lpBuffer, DWORD nSize, void* Arguments);

// String functions
int MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte,
                        LPWSTR lpWideCharStr, int cchWideChar);
int WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar,
                        LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, int* lpUsedDefaultChar);
DWORD GetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart);
DWORD GetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR* lpFilePart);

// Memory functions
void* LocalFree(void* hMem);
void* GlobalLock(HANDLE hMem);
int GlobalUnlock(HANDLE hMem);
HANDLE GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
HANDLE GlobalFree(HANDLE hMem);

// Clipboard functions
int OpenClipboard(HWND hWndNewOwner);
int CloseClipboard(void);
HANDLE GetClipboardData(UINT uFormat);
HANDLE SetClipboardData(UINT uFormat, HANDLE hMem);
int EmptyClipboard(void);

#ifdef __cplusplus
}

// C++ standard library functions needed (not Windows-specific but used)
#include <wchar.h>  // for wcslen, wcscpy

#endif

#endif // WINDOWS_H_DUMMY
