// Hex File Comparator - A visual side-by-side hex viewer and file comparison tool
// Supports comparing up to 8 files simultaneously with highlighted differences
// Features: hex/ASCII display, configurable fonts, keyboard navigation, difference scanning

// Include Windows API for window creation, graphics, and file operations
#include <windows.h>
// Link against GDI32 - needed for graphics operations (fonts, bitmaps, drawing)
#pragma comment(lib,"gdi32.lib")
// Link against USER32 - needed for window management and message handling
#pragma comment(lib,"user32.lib")
// Link against ADVAPI32 - needed for registry operations (save/load config)
#pragma comment(lib,"advapi32.lib")
// Link against COMDLG32 - needed for common dialogs (font selection, file open)
#pragma comment(lib,"comdlg32.lib")

//--- #include "common.inc"

// Standard I/O for printf debugging
#include <stdio.h>
// Standard library for memory allocation (new/delete)
#include <stdlib.h>
// Standard definitions for size_t and NULL
#include <stddef.h>
// String functions for strcpy, strlen, etc.
#include <string.h>
// Memory functions for memcpy, memset, etc.
#include <memory.h>
// Undefine EOF if previously defined - we don't use stdio file I/O
#undef EOF

// Pack structures to 1-byte alignment - ensures no padding between fields for binary I/O
#pragma pack(1)

// Type aliases for consistent width types across different platforms
typedef unsigned short word;       // 16-bit unsigned (used for char+attribute pairs)
typedef unsigned int   uint;       // 32-bit unsigned (general purpose)
typedef unsigned char  byte;       // 8-bit unsigned (file data, color components)
typedef unsigned long long qword;  // 64-bit unsigned (file positions and sizes)
typedef signed long long sqword;   // 64-bit signed (for position deltas)

// Zero-initialization templates for various data types
// Single object: zero out all bytes of the object
template <class T> void bzero( T &_p ) { int i; byte* p = (byte*)&_p; for( i=0; i<sizeof(_p); i++ ) p[i]=0; }
// Fixed-size array: call default constructor (which may zero) for each element
template <class T, int N> void bzero( T (&p)[N] ) { int i; for( i=0; i<N; i++ ) p[i]=0; }
// Pointer + count: zero out N elements
template <class T> void bzero( T* p, int N ) { int i; for( i=0; i<N; i++ ) p[i]=0; }
// 2D array: zero out all elements in the matrix
template <class T, int N, int M>  void bzero(T (&p)[N][M]) { for( int i=0; i<N; i++ ) for( int j=0; j<M; j++ ) p[i][j] = 0; }

// Utility templates
// Return minimum of two values
template <class T> T Min( T x, T y ) { return (x<y) ? x : y; }
// Return maximum of two values
template <class T> T Max( T x, T y ) { return (x>y) ? x : y; }
// Get number of elements in a static array
template <class T,int N> int DIM( T (&wr)[N] ) { return sizeof(wr)/sizeof(wr[0]); };
// Align value x up to next multiple of r (e.g., AlignUp(13, 16) = 16)
#define AlignUp(x,r) ((x)+((r)-1))/(r)*(r)

// Compile-time word constant generator for FourCC-style codes
// Packs 4 bytes into 32-bit integer in both little-endian (n) and big-endian (x) order
template<byte a,byte b,byte c,byte d> struct wc {
  static const unsigned int n=(d<<24)+(c<<16)+(b<<8)+a;  // Little-endian: d is MSB
  static const unsigned int x=(a<<24)+(b<<16)+(c<<8)+d;  // Big-endian: a is MSB
};

// Compiler-specific attributes for function inlining and alignment
#ifdef __GNUC__
 // GCC/Clang: Force function to be inlined for performance
 #define INLINE   __attribute__((always_inline))
 // GCC/Clang: Prevent function from being inlined
 #define NOINLINE __attribute__((noinline))
 // GCC/Clang: Align variable/struct to n-byte boundary (for SIMD, cache optimization)
 #define ALIGN(n) __attribute__((aligned(n)))
 // GCC/Clang: No-op - GCC doesn't need explicit alignment hints for pointers
 #define __assume_aligned(x,y)
#else
 // MSVC: Force function to be inlined
 #define INLINE   __forceinline
 // MSVC: Prevent function from being inlined
 #define NOINLINE __declspec(noinline)
 // MSVC: Align variable/struct to n-byte boundary
 #define ALIGN(n) __declspec(align(n))
#endif

// Compiler compatibility for __builtin_expect and __assume
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 // MSVC: No branch prediction hints, just return the expression
 #define __builtin_expect(x,y) (x)
 // MSVC: No pointer alignment hints needed
 #define __assume_aligned(x,y)
#endif

#if !defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 // GCC/Clang: Map __assume to expression (MSVC provides optimizer hints via __assume)
 #define __assume(x) (x)
#endif

// Get file length using stdio (unused in this program - we use Win32 API instead)
static uint flen( FILE* f ) {
  // Seek to end of file
  fseek( f, 0, SEEK_END );
  // Get current position = file length
  uint len = ftell(f);
  // Restore position to beginning
  fseek( f, 0, SEEK_SET );
  return len;
}

// Platform detection macros - detect if compiling for 64-bit or 32-bit
#if defined(__x86_64) || defined(_M_X64)
 // 64-bit build: define X64 macro
 #define X64
 #define X64flag 1
#else
 // 32-bit build: undefine X64 macro
 #undef X64
 #define X64flag 0
#endif

//--- #include "file_api_win.inc"

// Exclude rarely-used Windows APIs to reduce header size and compile time
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Additional virtual key code definitions not always in windows.h
#ifndef VK_OEM_PLUS
#define VK_OEM_PLUS   0xBB  // '+' key (used for font size increase)
#endif
#ifndef VK_OEM_MINUS
#define VK_OEM_MINUS  0xBD  // '-' key (used for font size decrease)
#endif

//--- #include "file_win.inc"

// File attribute flags for Win32 CreateFile operations (unused in this program)
// These map to FILE_FLAG_* constants from Windows API
enum {
  ffWRITE_THROUGH         =0x80000000,  // Bypass OS write cache (direct to disk)
  ffOVERLAPPED            =0x40000000,  // Enable async I/O operations
  ffNO_BUFFERING          =0x20000000,  // Bypass all OS caching (requires aligned I/O)
  ffRANDOM_ACCESS         =0x10000000,  // Hint: file accessed randomly (affects cache strategy)
  ffSEQUENTIAL_SCAN       =0x08000000,  // Hint: file accessed sequentially (prefetch more)
  ffDELETE_ON_CLOSE       =0x04000000,  // Delete file when last handle closes (temp files)
  ffBACKUP_SEMANTICS      =0x02000000,  // Allow opening directories (not just files)
  ffPOSIX_SEMANTICS       =0x01000000,  // Case-sensitive filename matching
  ffOPEN_REPARSE_POINT    =0x00200000,  // Open symlink itself, not target
  ffOPEN_NO_RECALL        =0x00100000   // Don't retrieve from offline storage (HSM systems)
};

// Low-level Win32 file opening wrapper (unused - more specific functions below are used instead)
HANDLE Win32_Open( const wchar_t* s, uint Flags, uint Attrs ) {
  return CreateFileW(
    s,                              // File path
    GENERIC_READ | GENERIC_WRITE,   // Request read and write access
    0,                              // No sharing - exclusive access
    0,                              // Default security attributes
    Flags,                          // Creation disposition (OPEN_EXISTING, CREATE_NEW, etc.)
    Attrs,                          // File attributes and flags
    0                               // No template file
  );
}

// Open a directory handle (for directory operations like setting timestamps)
HANDLE file_opendir( const wchar_t* s ) {
  HANDLE r = CreateFileW(
     s,                                 // Directory path
     GENERIC_WRITE,                     // Need write access to modify directory metadata
     FILE_SHARE_READ | FILE_SHARE_WRITE,// Allow others to access while open
     0,                                 // Default security
     OPEN_EXISTING,                     // Directory must already exist
     FILE_FLAG_BACKUP_SEMANTICS,        // Required flag to open directories (not files)
     0                                  // No template
   );
  // Convert INVALID_HANDLE_VALUE to NULL for easier checking
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create a directory
int file_mkdir( const wchar_t* name ) {
  // Returns non-zero on success, zero on failure
  return CreateDirectoryW( name, 0 );
}

// Global file operation modes - these control how file_open() and file_make() behave
// Default: open files in read-only mode
uint file_open_mode = GENERIC_READ;
// Set file_open() to read-only mode
void file_open_mode_r( void ) { file_open_mode = GENERIC_READ; }
// Set file_open() to read-write mode
void file_open_mode_rw( void ) { file_open_mode = GENERIC_READ | GENERIC_WRITE; }

// Default: create files in write-only mode
uint file_make_mode = GENERIC_WRITE;
// Set file_make() to write-only mode
void file_make_mode_w( void ) { file_make_mode = GENERIC_WRITE; }
// Set file_make() to read-write mode
void file_make_mode_rw( void ) { file_make_mode = GENERIC_READ | GENERIC_WRITE; }

// Default: file_make() always creates new file (overwrites existing)
uint file_make_cmode = CREATE_ALWAYS;
// Set file_make() to open existing or create if doesn't exist
void file_make_open( void ) { file_make_cmode = OPEN_ALWAYS; }
// Set file_make() to always create new file (overwrites existing)
void file_make_create( void ) { file_make_cmode = CREATE_ALWAYS; }

// Open existing file (wide-char version) - used for reading files to compare
HANDLE file_open( const wchar_t* name ) {
  HANDLE r = CreateFileW(
     name,                                       // File path (Unicode)
     file_open_mode,                             // Access mode (controlled by global)
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow full sharing
     0,                                          // Default security
     OPEN_EXISTING,                              // File must exist (fail if not found)
     0,                                          // No special flags
     0                                           // No template
  );
  // Convert INVALID_HANDLE_VALUE to NULL for easier error checking
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Open existing file (ANSI version) - used in this program for command-line filenames
HANDLE file_open( const char* name ) {
  HANDLE r = CreateFileA(
     name,                                       // File path (ANSI/OEM)
     file_open_mode,                             // Access mode (controlled by global)
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow full sharing
     0,                                          // Default security
     OPEN_EXISTING,                              // File must exist
     0,                                          // No special flags
     0                                           // No template
  );
  // Convert INVALID_HANDLE_VALUE to NULL
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create or open file (wide-char version) - unused in this program
HANDLE file_make( const wchar_t* name ) {
  HANDLE r = CreateFileW(
     name,                                       // File path (Unicode)
     file_make_mode,                             // Access mode (controlled by global)
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow full sharing
     0,                                          // Default security
     file_make_cmode,                            // Creation mode (controlled by global)
     0,                                          // No special flags
     0                                           // No template
  );
  // Convert INVALID_HANDLE_VALUE to NULL
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create or open file (ANSI version) - unused in this program
HANDLE file_make( const char* name ) {
  HANDLE r = CreateFileA(
     name,                                       // File path (ANSI/OEM)
     file_make_mode,                             // Access mode (controlled by global)
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow full sharing
     0,                                          // Default security
     file_make_cmode,                            // Creation mode (controlled by global)
     0,                                          // No special flags
     0                                           // No template
  );
  // Convert INVALID_HANDLE_VALUE to NULL
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Close file handle and release OS resources
int file_close( HANDLE file ) {
  // Returns non-zero on success
  return CloseHandle( file );
}

// Read from file (may return partial read if less data available than requested)
uint file_read( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r = 0;  // Number of bytes actually read
  // Read up to 'len' bytes into buffer, store actual count in 'r'
  ReadFile( file, buf, len, (LPDWORD)&r, 0 );
  // Return actual bytes read (may be less than len if near EOF)
  return r;
}

// Synchronized read - keeps reading until requested length or EOF
// Used when you must read exactly 'len' bytes or reach EOF
uint file_sread( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r;      // Bytes read in current iteration
  uint flag=1; // Success flag from ReadFile
  uint l = 0;  // Total bytes read so far

  // Keep reading until we get all requested bytes or hit EOF/error
  do {
    r = 0;
    // Read remaining bytes (len-l) starting at buf+l
    flag = ReadFile( file, buf+l, len-l, (LPDWORD)&r, 0 );
    l += r;  // Accumulate total bytes read
  } while( (r>0) && (l<len) && flag );  // Continue if made progress and not done

  return l;  // Return total bytes read
}

// Write to file
uint file_writ( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r = 0;  // Number of bytes actually written
  // Write 'len' bytes from buffer, store actual count in 'r'
  WriteFile( file, buf, len, (LPDWORD)&r, 0 );
  return r;  // Return actual bytes written
}

// Seek to position in file (supports 64-bit positions for large files)
qword file_seek( HANDLE file, qword ofs, int typ = FILE_BEGIN ) {
  uint lo,hi;
  lo = uint(ofs);       // Low 32 bits of offset
  hi = uint(ofs>>32);   // High 32 bits of offset
  // SetFilePointer: pass low 32 bits directly, high 32 bits via pointer
  lo = SetFilePointer( file, lo, (PLONG)&hi, typ );
  // Reconstruct 64-bit result from returned high and low parts
  ofs = hi;
  ofs = (ofs<<32) + lo;
  return ofs;  // Return new file position
}

// Get current file position
qword file_tell( HANDLE file ) {
  // Seek by 0 bytes from current position returns current position
  return file_seek( file, 0, FILE_CURRENT );
}

// Get file size using binary search (for files where normal method fails)
// This is a fallback for special files (e.g., raw disk devices) where seeking to end doesn't work
qword getfilesize( HANDLE f ) {
  qword pos=-1LL;  // Will hold the file size
  const uint bufsize=1<<16,bufalign=1<<12;  // 64KB buffer, 4KB alignment
  byte* _buf = new byte[bufsize+bufalign];  // Allocate extra for alignment
  if( _buf!=0 ) {
    // Align buffer to page boundary (bufalign) for better I/O performance
    byte* buf = _buf + bufalign-((_buf-((byte*)0))%bufalign);
    uint bit,len;
    pos = 0;
    // Binary search from bit 62 down to 0 (searching up to ~4 exabytes)
    for( bit=62; bit!=-1; bit-- ) {
      pos |= 1ULL << bit;  // Try setting this bit (assume file is at least this large)
      file_seek(f,pos);    // Seek to that position
      len = file_read(f,buf,bufsize);  // Try to read 64KB
      // If we got data but less than buffer size, we're at EOF
      if( (len!=0) && (len<bufsize) ) { pos+=len; break; }
      // If we got full buffer, file is at least this large - keep the bit set
      if( len>=bufsize ) continue;
      // If we got no data, we seeked past EOF - clear this bit
      pos &= ~(1ULL<<bit);
    }
    delete[] _buf;
  }
  return pos;  // Return discovered file size
}

// Get file size (standard method with fallback)
qword file_size( HANDLE file ) {
  qword t = file_tell(file);  // Save current position
  qword r = file_seek( file, 0, FILE_END );  // Seek to end, returns position = size
  if( uint(r)==0xFFFFFFFF ) {
    // SetFilePointer returns 0xFFFFFFFF on error (but this is also a valid position!)
    // Check if there's actually an error
    if( GetLastError()!=0 ) r = getfilesize(file);  // Use binary search fallback
  }
  t = file_seek( file, t );  // Restore original position
  return r;  // Return file size
}

// Get last error as text string (converts Windows error code to human-readable message)
char* GetErrorText( void ) {
  wchar_t* lpMsgBuf;
  DWORD dw = GetLastError();  // Get last error code from Windows
  // Ask Windows to format the error message for us
  FormatMessageW(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL
  );
  static char out[32768];  // Static buffer for return value
  int wl=wcslen(lpMsgBuf);  // Get length of wide-char message
  // Convert Unicode message to OEM code page (for console output)
  WideCharToMultiByte( CP_OEMCP, 0, lpMsgBuf,wl+1, out,sizeof(out), 0,0 );
  LocalFree(lpMsgBuf);  // Free the buffer allocated by FormatMessageW
  wl = strlen(out);
  // Remove trailing whitespace/control chars (error messages often have \r\n)
  for( --wl; wl>=0; wl-- ) if( (byte&)out[wl]<32 ) out[wl]=' ';
  return out;  // Return pointer to static buffer
}

// Expand path to full UNC path with \\?\ prefix for long path support (>260 chars)
uint ExpandPath( wchar_t* path, wchar_t* w, uint wsize ) {
  wcscpy( w, L"\\\\?\\" );  // Start with \\?\ prefix for long path support
  // Get full path (resolves relative paths, . and ..) and append to prefix
  GetFullPathNameW( path, wsize/sizeof(wchar_t)-4,&w[4], 0 );
  // If result is UNC path (\\server\share), remove the \\?\ prefix (UNC uses \\?\UNC\ instead)
  if( (w[4]=='\\') && (w[5]=='\\') ) wcscpy( &w[0], &w[4] );
  return wcslen(w);  // Return length of expanded path
}

// File handle wrapper providing convenient C++ interface
// Base class without constructor (allows derived class to initialize)
struct filehandle0 {

  HANDLE f;  // Windows file handle

  // Allow implicit conversion to int for boolean checks (non-zero = valid handle)
  operator int( void ) {
    // Convert HANDLE pointer to integer (NULL becomes 0, valid handle becomes non-zero)
    return ((byte*)f)-((byte*)0);
  }

  // Open existing file (template works with char* or wchar_t*)
  template< typename CHAR >
  uint open( const CHAR* name ) {
    f = file_open( name );  // Call appropriate overload based on CHAR type
    return ((byte*)f)-((byte*)0);  // Return non-zero if successful
  }

  // Create or open file (template works with char* or wchar_t*)
  template< typename CHAR >
  uint make( const CHAR* name ) {
    f = file_make( name );  // Call appropriate overload based on CHAR type
    return ((byte*)f)-((byte*)0);  // Return non-zero if successful
  }

  // Close the file
  int close( void ) {
    return file_close(f);
  }

  // Get file size
  qword size( void ) { return file_size(f); }

  // Seek to position (default: absolute position from beginning)
  void seek( qword ofs, uint typ=FILE_BEGIN ) {
    file_seek( f, ofs, typ );
  }

  // Get current position
  qword tell( void ) {
    return file_tell(f);
  }

  // Read into structure/variable (template deduces size automatically)
  template< typename BUF >
  uint read( BUF& buf ) { return read( &buf, sizeof(buf) )!=sizeof(buf); }

  // Read raw bytes
  uint read( void* _buf, uint len ) { return file_read( f, _buf, len ); }

  // Synchronized read into structure (keeps reading until buffer full or EOF)
  template< typename BUF >
  uint sread( BUF& buf ) { return sread( &buf, sizeof(buf) )!=sizeof(buf); }

  // Synchronized read raw bytes
  uint sread( void* _buf, uint len ) { return file_sread( f, _buf, len ); }

  // Read single character (returns -1 on EOF, character value otherwise)
  int Getc( void ) {
    byte c;
    uint l = read(c);  // Read one byte
    return l ? -1 : c;  // If read failed (l != 0), return -1, else return character
  }

  // Write raw bytes
  uint writ( void* _buf, uint len ) {
    return file_writ( f, _buf, len );
  }

  // Write structure/variable (template deduces size automatically)
  template< typename BUF >
  uint writ( BUF& buf ) {
    return writ( &buf, sizeof(buf) )!=sizeof(buf);
  }

};

// File handle with automatic initialization (adds constructor to base class)
struct filehandle : filehandle0 {
  // Default constructor: initialize handle to NULL
  filehandle() { f=0; }

  // Constructor that opens or creates file based on f_wr flag
  template< typename CHAR >
  filehandle( const CHAR* name, uint f_wr=0 ) {
    // If f_wr is non-zero, create/write file; otherwise open for reading
    f_wr ? make(name) : open(name);
  }
};

//--- #include "thread.inc"

// Template wrapper for Win32 thread creation using CRTP (Curiously Recurring Template Pattern)
// Child class must implement: void thread() - the thread's main function
template <class child>
struct thread {

  HANDLE th;  // Windows thread handle

  // Start the thread (calls child's thread() method in new thread)
  uint start( void ) {
    // Create thread with default settings, passing 'this' as parameter
    th = CreateThread( 0, 0, &thread_w, this, 0, 0 );
    return th!=0;  // Return non-zero if successful
  }

  // Wait for thread to finish and cleanup resources
  void quit( void ) {
    WaitForSingleObject( th, INFINITE );  // Block until thread terminates
    CloseHandle( th );  // Release thread handle
  }

  // Static wrapper to redirect to member function (Win32 threads can't call member functions directly)
  static DWORD WINAPI thread_w( LPVOID lpParameter ) {
    // Cast parameter back to child class and call its thread() method
    ((child*)lpParameter)->thread();
    return 0;  // Thread exit code
  }
};

// Sleep for a short duration in thread (10ms - used for polling loops)
void thread_wait( void ) {
  Sleep(10);
} 

//--- #include "bitmap.inc"

// Allocate a DIB (Device Independent Bitmap) section for drawing
// DIB allows direct pixel access (unlike DDB which is device-dependent)
HBITMAP AllocBitmap( HDC dibDC, byte*& bitmap, uint bmX, uint bmY ) {
  // Setup bitmap info: width=bmX, height=-bmY (negative = top-down), 32bpp RGBA
  BITMAPINFO bm = { sizeof(BITMAPINFOHEADER), int(bmX), -int(bmY), 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
  // Create DIB section and get pointer to pixel data via 'bitmap' parameter
  HBITMAP dib = CreateDIBSection( dibDC, &bm, DIB_RGB_COLORS, (void**)&bitmap, 0,0 );
  // Select the bitmap into the DC so drawing operations target it
  SelectObject( dibDC, dib );
  return dib;  // Return bitmap handle
}

// Bitmap wrapper with pixel access
struct mybitmap {

  uint bmX, bmY;     // Bitmap dimensions in pixels
  byte* bitmap;      // Pointer to raw pixel data (4 bytes per pixel: BGRA)
  HBITMAP dib;       // Windows bitmap handle

  // Allocate bitmap of specified size
  void AllocBitmap( HDC dibDC, uint _bmX, uint _bmY ) {
    bitmap = 0;
    bmX = _bmX; bmY = _bmY;
    // Call global AllocBitmap function, which sets 'bitmap' pointer
    dib = ::AllocBitmap( dibDC, bitmap, bmX, bmY );
  }

  // Clear bitmap to black (all pixels = 0x00000000)
  void Reset() {
    memset( bitmap, 0, 4*bmX*bmY );  // 4 bytes per pixel
  }

  // Access pixel at (x,y) as 32-bit RGBA value (returns reference for read/write)
  uint& pixel( uint x, uint y ) {
    // Pixel at row y, column x. Each pixel is 4 bytes (BGRA order in memory)
    return (uint&)bitmap[(y*bmX+x)*4];
  }

};

//--- #include "setfont.inc"

// Font management and pre-rendering
// Pre-renders all 256 characters to bitmaps for fast blitting (avoids slow TextOut in render loop)
struct myfont {
  HFONT hFont;         // Windows font handle
  LOGFONT lf;          // Font specification (size, face name, weight, etc.)
  uint* fontbuf1;      // Pre-rendered character bitmaps (256 chars * wmax * hmax pixels)
  int wmin,wmax,hmin,hmax;  // Min/max character cell dimensions
  int wbuf[256];       // Width of each character (0-255)
  int hbuf[256];       // Height of each character (0-255)

  // Initialize font structure (called before SetFont)
  void InitFont( void ) {
    hFont=0;           // No font selected yet
    bzero(lf);         // Zero out LOGFONT structure
  }

  // Cleanup font resources
  void Quit( void ) {
    delete fontbuf1;   // Free pre-rendered character buffer
    DeleteObject( hFont ); hFont=0;  // Delete GDI font object
  }

  // Debug: print LOGFONT structure to console
  void DumpLF() {
    uint* x= (uint*)&lf;           // First 5 fields are ints
    byte* y= (byte*)&lf.lfItalic;  // Next 8 fields are bytes
    uint i;
    printf( "{ " );
    // Print height, width, escapement, orientation, weight
    for( i=0; i<5; i++ ) printf( "%i,", x[i] ); printf( "  " );
    // Print italic, underline, strikeout, charset, etc.
    for( i=0; i<8; i++ ) printf( "%i,", y[i] ); printf( "  " );
    // Print font face name
    printf( "\"%s\" }\n", lf.lfFaceName );
  }

  // Show font selection dialog to user
  void SelectFont( void ) {
    CHOOSEFONT cf;
    bzero(cf);
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = 0;  // No parent window
    cf.lpLogFont = &lf;  // Edit this LOGFONT structure
    // Flags: screen fonts only, fixed-pitch only (for hex display), no vertical fonts
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FIXEDPITCHONLY | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;
    if( ChooseFont(&cf) ) DumpLF();  // If user clicked OK, print the selection
  }

  // Measure character widths and heights for all 256 characters
  void GetFontWidth( HDC dibDC ) {
    char s[2] = {0,0}; SIZE cs;  // Single-char string and size result
    int i;
    for( i=0; i<256; i++ ) wbuf[i]=0;  // Initialize width buffer

    // Measure each character
    for( i=0; i<256; i++ ) {
      s[0]=i; cs.cx=cs.cy=0;  // Set character to measure
      // Get dimensions of this character in current font
      GetTextExtentPoint32( dibDC, s,1, &cs );
      wbuf[i] = cs.cx;  // Store width
      hbuf[i] = cs.cy;  // Store height
    }

    // Find min/max character dimensions across all 256 chars
    wmin=wbuf[0]; hmin=hbuf[0];
    wmax=0;       hmax=0;
    for( i=0; i<256; i++ ) {
      wmin=Min(wmin,wbuf[i]), wmax=Max(wmax,wbuf[i]);
      hmin=Min(hmin,hbuf[i]), hmax=Max(hmax,hbuf[i]);
    }

    // Debug output
    printf( "wmin=%i wmax=%i\n", wmin,wmax );
    printf( "hmin=%i hmax=%i\n", hmin,hmax );
  }

  // Set font and pre-render all characters (called when font changes)
  void SetFont( HDC dibDC, LOGFONT& _lf ) {
    HFONT currfont = hFont;  // Save old font to delete later
    lf = _lf;  // Store new font specification
    hFont = CreateFontIndirect( &lf );  // Create Windows font object
    if( currfont!=0 ) DeleteObject( currfont );  // Delete old font if exists
    SelectObject( dibDC, hFont );  // Select new font into DC

    byte* bitmap = 0;

    GetFontWidth(dibDC);  // Measure all character dimensions

    // Create temporary DC and bitmap for pre-rendering characters
    HDC DC0 = GetDC(0);  // Get screen DC
    HDC DC = CreateCompatibleDC( DC0 );  // Create memory DC

    // Allocate bitmap large enough for largest character
    HBITMAP dib = AllocBitmap( DC, bitmap, wmax, hmax );
    if( bitmap!=0 ) {
      SelectObject( DC, hFont );  // Select font into temp DC
      PrecalcFont( DC, bitmap, wmax );  // Pre-render all 256 characters
      DeleteObject(dib);  // Delete temporary bitmap
    }

    ReleaseDC(0,DC0);  // Release screen DC
    DeleteDC(DC);      // Delete memory DC
  }

  // Pre-render all 256 characters to bitmap buffer for fast blitting
  // This is a critical optimization: render each character once, then copy pixels during display
  void PrecalcFont( HDC dibDC, byte* bitmap, uint bmX ) {
    uint c,x,y,i,j; char s[2];
    // Configure DC for character rendering
    SetBkMode(dibDC, TRANSPARENT);  // Don't fill background
    SetTextAlign( dibDC, TA_TOP|TA_LEFT );  // Align text to top-left corner
    SetBkColor(dibDC,   0x000000 );  // Black background (RGB)
    SetTextColor(dibDC, 0xFFFFFF );  // White text (RGB) - will be colorized later

    // Allocate buffer to store all 256 pre-rendered characters
    uint* fontbuf = new uint[ 256*wmax*hmax ];
    if( fontbuf!=0 ) {
      HBRUSH hbr = CreateSolidBrush(0x000000);  // Black brush for clearing
      RECT rect = { 0,0,wmax,hmax };  // Rectangle covering character cell

      // Render each character (0-255)
      for( c=0; c<256; c++ ) {

        FillRect(dibDC, &rect, hbr );  // Clear to black

        s[0]=c; s[1]=0;  // Single character string
        TextOut( dibDC, 0, 0, s, 1 );  // Render character at (0,0)

        // Copy rendered character from temporary bitmap to font buffer
        for( y=0; y<hmax; y++ ) {
          for( x=0; x<wmax; x++ ) {
            i = (y*bmX + x)*4;  // Index in source bitmap (4 bytes per pixel)
            // Store pixel in font buffer at: character c, row y, column x
            fontbuf[(c*hmax+y)*wmax+x] = (uint&)bitmap[i];
          }
        }
      }

      DeleteObject( hbr );  // Delete brush
    }

    fontbuf1 = fontbuf;  // Store pointer to pre-rendered buffer
  }

};

//--- #include "palette.inc"

// Color pair (foreground and background) stored as 24-bit RGB (0x00BBGGRR)
struct color {
  uint fg,bk;  // Foreground and background colors
};

// Palette indices for different UI elements
// Each UI element is assigned a palette index which selects its color pair
enum pal_ {
  pal_Sep=0,    // Separator lines between file panels
  pal_Hex,      // Normal hex/ASCII display (yellow on black)
  pal_Addr,     // Address column (green on black)
  pal_Diff,     // Highlighted differences (white on blue)

  pal_Help1,    // Help text normal (cyan on dark blue)
  pal_Help2,    // Help text highlighted sections (white on dark blue)

  pal_MAX       // Number of palette entries
};

// Color palette definition (RGB values: 0x00BBGGRR)
color palette[pal_MAX] = {
  { 0xFFFFFF, 0x000000 },  // pal_Sep: white on black
  { 0xFFFF55, 0x000000 },  // pal_Hex: light yellow on black
  { 0x00AA00, 0x000000 },  // pal_Addr: green on black
  { 0xFFFFFF, 0x0000AA },  // pal_Diff: white on dark blue

  { 0x00FFFF, 0x000080 },  // pal_Help1: cyan on navy
  { 0xFFFFFF, 0x000080 },  // pal_Help2: white on navy
};

//--- #include "textblock.inc"

// Text buffer for displaying character grid (like a terminal emulator buffer)
// Stores characters and their attributes (palette index) for each cell
struct textblock {
  uint  WCX,WCY;  // Window size in characters (columns, rows)
  uint  WPX,WPY;  // Position on screen in pixels (for rendering)
  uint  WSX,WSY;  // Size in pixels (for layout calculations)
  uint  blksize;  // Total cells = WCX * WCY
  word* text;     // Character + attribute buffer (low byte=char, high byte=palette index)

  // Clear all cells to null characters
  void Clear( void ) {
    uint i;
    if( text!=0 ) for( i=0; i<blksize; i++ ) text[i]=0;
  }

  // Initialize textblock with size and position
  template< class myfont >
  void Init( myfont& ch1, uint CX, uint CY, uint PX, uint PY ) {
    WCX=CX; WCY=CY; blksize=WCY*WCX;  // Set dimensions
    WPX=PX; WPY=PY;  // Set screen position
    // Calculate pixel size based on font dimensions
    WSX = WCX*ch1.wmax;  // Width in pixels
    WSY = WCY*ch1.hmax;  // Height in pixels
    text = new word[WCY*WCX];  // Allocate buffer
  }

  // Free resources
  void Quit( void ) {
    delete text;
  }

  // Access character cell at (x,y) - returns reference for read/write
  word& cell( uint x, uint y ) {
    return (word&)text[y*WCX+x];  // Row-major order
  }

  // Pack character and attribute into word (low byte=char, high byte=attribute)
  word ch( byte c, byte attr ) {
    return c | (attr<<8);
  }

  // Fill entire buffer with character+attribute pair
  void Fill( uint c, uint atr ) {
    uint i;
    for( i=0; i<blksize; i++ ) text[i]=ch(c,atr);
  }

  // Print text string to textblock with attribute toggling ('~' toggles between atr1 and atr2)
  // Used for help text where ~ marks text to highlight
  void Print( char* s, uint atr1, uint atr2=0, uint X=0, uint Y=0 ) {
    uint c,a=atr1,x=X,y=Y;  // c=current char, a=current attribute, x,y=cursor position
    while( c=*s++ ) {
      if( c=='\n' ) { x=X; y++; continue; }  // Newline: return to left margin, next row
      if( c=='~' ) { a^=atr1^atr2; continue; }  // Toggle attribute between atr1 and atr2
      if( (x<WCX) && (y<WCY) ) cell(x,y) = ch(c,a);  // Write char if in bounds
      x++;  // Advance cursor
    }
  }

  // Calculate text size (in characters) - used for layout calculations
  void textsize( char* s, uint* tb_SX, uint* tb_SY ) {
    uint c,x=0,y=0;
    while( c=*s++ ) {
      if( c=='\n' ) { x=0; y++; continue; }  // Newline
      if( c=='~' ) continue;  // Skip attribute toggle markers
      x++;  // Count characters
    }
    // Return width (x) and height (y+1 if partial line exists)
    if(tb_SX!=0) *tb_SX=x;
    if(tb_SY!=0) *tb_SY=y;
  }

  // Render textblock to bitmap using font (convert text buffer to pixels)
  template< class myfont, class mybitmap >
  void Print( myfont& ch1, mybitmap& bm1 ) {
    uint i,j;
    word* s = text;  // Pointer to current row in text buffer
    uint PX=WPX, PY=WPY;  // Current pixel position

    SymbOut_Init();  // Clear character rendering cache

    // For each row
    for( j=0; j<WCY; j++ ) {
      // For each column in the row
      for( i=0; i<WCX; i++ ) SymbOut( ch1, bm1,PX+i*ch1.wmax,PY, s[i] );
      PY+=ch1.hmax;  // Move to next row of pixels
      s+=WCX;        // Move to next row of characters
    }
  }

//--- #include "textprint.inc"

// Character rendering cache - stores pointers to last rendered positions
// This is a key optimization: if we already rendered char X with palette Y, just copy from previous location
uint* cache_ptr[pal_MAX][256];  // [palette index][character] -> pointer to previously rendered pixels

// Clear the character rendering cache (called before each frame)
void SymbOut_Init( void ) {
  bzero( cache_ptr );  // Set all cache pointers to NULL
}

// Optimized character rendering with caching
// Instead of re-rendering, copies from previous location if same char+palette was already drawn
// This is a major performance optimization for displaying thousands of characters
template< class myfont, class mybitmap >
void SymbOut( myfont& ch1, mybitmap& bm1,uint PX,uint PY, word cp ) {
  uint i,j,k;

  uint chridx=byte(cp), palidx=cp>>8;  // Extract character code and palette index

  uint tx = bm1.bmX;  // Bitmap width (for scanline stride)
  uint* outp = &bm1.pixel(PX,PY);  // Destination pixel pointer

  uint* fontbuf = ch1.fontbuf1;  // Pre-rendered character bitmaps
  uint sx=ch1.wmax;  // Character width in pixels
  uint sy=ch1.hmax;  // Character height in pixels

  uint*& pcache = cache_ptr[palidx][chridx];  // Reference to cache entry

  // If this char+palette combination was already rendered, copy pixels from previous location
  if( pcache!=0 ) {

    uint* __restrict p = pcache;  // Source: previously rendered location
    uint* __restrict o = outp;    // Destination: current location
    __assume_aligned(p,4);  // Hint to compiler for optimization
    __assume_aligned(o,4);
    pcache = outp;  // Update cache to point to new location (for next time)
    // Copy character pixels row by row
    for( k=0; k<sy; k++ ) {
      for( i=0; i<sx; i++ ) o[i]=p[i];
      p += tx;  // Next row in source
      o += tx;  // Next row in destination
    }

  } else {
    // First time rendering this char+palette combination: colorize pre-rendered character
    byte* __restrict p = (byte*)&fontbuf[chridx*sy*sx];  // Pre-rendered grayscale character
    byte* __restrict o = (byte*)outp;  // Output pixels
    pcache = outp;  // Cache this location for future use
    ALIGN(16) byte q[4];  // Foreground color components
    ALIGN(16) byte r[4];  // Background color components
    __assume_aligned(p,4);
    __assume_aligned(o,4);

    // Load foreground and background colors from palette
    for( i=0; i<sizeof(q); i+=4 ) (uint&)q[i]=palette[palidx].fg, (uint&)r[i]=palette[palidx].bk;

    // Alpha-blend pre-rendered grayscale character with palette colors
    // Font bitmap values are used as alpha (0=background, 255=foreground)
    for( k=0; k<sy; k++ ) {  // For each row
      for( i=0; i<sx*4; i+=sizeof(q) ) {  // For each pixel (4 bytes/pixel)
        for( j=0; j<3; j++ ) {  // For each color component (B,G,R - skip alpha)
          // Blend: output = background + (foreground-background) * alpha/255
          word x = r[j]*255 + (int(q[j])-int(r[j]))*p[i+j];
          o[i+j] = (x+255)>>8;  // Divide by 256 with rounding
        }
      }
      p += sx*4;  // Next row in font bitmap
      o += tx*4;  // Next row in output bitmap
    }

  }

}

};

//--- #include "hexdump.inc"

// Hex file viewer with caching and difference highlighting
// Displays hex dump and ASCII view of a file, with intelligent caching for large files
struct hexfile {
  filehandle0 F1;  // File handle
  qword F1size;    // Total file size in bytes
  qword F1pos;     // Current view position in file (top-left byte being displayed)

  // Display flags
  enum {
    f_vertline=1,  // Draw vertical separator line (between file panels)
    f_addr64       // Use 64-bit address display (vs 32-bit)
  };
  uint  flags;     // Combination of above flags

  uint  BX;       // Bytes per line (configured - actual data width)
  uint  F1cpl;    // Chars per line (visible - may be < BX if window too narrow)
  uint  F1dpl;    // Unused chars in line (horizontal slack space)
  uint  textlen;  // Total bytes visible in current view (BX * number_of_rows)
  byte* diffbuf;  // Difference highlight flags per byte (1=different, 0=same)

  // File data caching - keeps a 1MB sliding window of file data
  // This allows viewing multi-GB files without loading everything into RAM
  qword viewbeg;  // First visible byte position in file
  qword viewend;  // Last visible byte position+1 in file
  enum{ datalen=1<<20, datalign=1<<16 };  // 1MB cache buffer, 64KB alignment for I/O
  qword databeg;  // Start of cached region in file
  qword dataend;  // End of cached region in file
  byte  databuf[datalen];  // Cached file data (1MB sliding window)

  // Calculate required text buffer width in characters for hex display
  uint Calc_WCX( uint mBX, uint f_addr64, uint f_vertline ) {
    uint waddr = f_addr64 ? 8+1+8 : 8;  // Address: "XXXXXXXX" or "XXXXXXXX:XXXXXXXX"
    uint whex = 3*mBX;                   // Hex dump: "XX " per byte
    uint wtxt = 1*mBX;                   // ASCII: one char per byte
    // Total: "ADDRESS: " + "hex hex hex..." + " " + "ASCII..." + "|"
    uint WCX = waddr+1+1+whex+1+wtxt+(f_vertline>0);
    return WCX;
  }

  // Initialize textbuffer parameters (called during setup/resize)
  void SetTextbuf( textblock& tb1, uint _BX, uint _flags=0 ) {
    flags = _flags;  // Store display flags
    BX = _BX;        // Bytes per line

    // Calculate how many bytes actually fit in the allocated text buffer
    uint aw = (flags&f_addr64)? 8+1+8 : 8;  // Address column width
    F1cpl = (tb1.WCX-aw-2-1)/(3+1);  // Each byte takes 3 chars in hex + 1 char in ASCII
    F1dpl = (tb1.WCX-aw-2-1)%(3+1);  // Leftover horizontal space

    textlen = BX*tb1.WCY;  // Total bytes that can be displayed
    diffbuf = new byte[textlen];  // Allocate difference highlight buffer
    if( diffbuf!=0 ) bzero( diffbuf, textlen );  // Clear to "no differences"
  }

  // Cleanup resources
  void Quit( void ) {
    delete diffbuf;
  }

  // Get byte at offset i from current view (returns -1 if beyond EOF)
  uint viewdata( uint i ) {
    qword ofs = F1pos+i;  // Absolute file position
    uint c, f = (ofs>=viewbeg) && (ofs<viewend);  // Check if in cached range
    c = f ? databuf[F1pos-databeg+i] : -1;  // Get from cache or return -1 (EOF marker)
    return c;
  }

  // Compare this file with another (unused - comparison now done in main loop)
  void Compare( hexfile& F2 ) {
    uint c1,c2,i;

    // For each byte in the visible view
    for( i=0; i<textlen; i++ ) {
      c1 = (*this).viewdata(i);  // Get byte from this file
      c2 = F2.viewdata(i);       // Get byte from other file
      diffbuf[i] = (c1!=c2);     // Mark as different if bytes don't match
    }
  }

  // Move view position by predefined navigation type
  void MovePos( uint m_type=0 ) {
    // Navigation deltas for: left, right, up, down, pgup, pgdn, wheel-up, wheel-dn
    const int delta[] = { -1,1, -int(BX),int(BX), -int(textlen),int(textlen), -int(BX*4), int(BX*4) };
    // m_type: 0=home, 1=end, 2-9=use delta array
    SetFilepos( (m_type==0)? 0 : (m_type==1) ? F1size-textlen : F1pos+sqword(delta[m_type-2]) );
  }

  // Move view by relative byte offset
  void MoveFilepos( int newpos=0 ) {
    F1pos += sqword(newpos);  // Add delta to current position
    SetFilepos( F1pos );      // Update view (may reload cache)
  }

  // Set absolute view position and update cache if needed (intelligent cache management)
  void SetFilepos( qword newpos ) {
    uint len;
    F1pos=newpos;  // Update view position

    qword newend = newpos+textlen;  // Calculate end of visible region

    // Clamp to file bounds
    if( newend>F1size ) newend=F1size;  // Can't go past EOF
    if( newend<newpos ) newpos=0;       // Handle underflow

    // Check if requested region is already cached
    if( (newpos>=databeg) && (newend<=dataend) ) {
      // All necessary data is in buffer already - no I/O needed!
    } else {
      // Need to load new data from disk
      // Align to 64KB boundary for better disk I/O performance and read-ahead
      databeg = newpos - (newpos % datalign);
      F1.seek( databeg );  // Seek to aligned position
      len = F1.read( databuf, datalen );  // Read 1MB into cache
      dataend = databeg + len;  // Calculate end of cached region
      if( newend>=dataend ) newend=dataend;  // Adjust if near EOF
    }

    viewbeg = newpos;  // Update visible region boundaries
    viewend = newend;
  }

  // Open file and get size
  size_t Open( char* fnam ) {
    F1pos=0;           // Start at beginning of file
    databeg = dataend=0;  // Cache is empty
    if( F1.open(fnam) ) {  // Open file for reading
      F1size = F1.size();  // Get total file size
    }
    // Return non-zero if successful (handle converted to size_t)
    return ((byte*)F1.f)-((byte*)0);
  }

  // Print hex number with specified width to textblock buffer
  word* HexPrint( word* s, qword x, uint w, uint attr ) {
    uint i,c;
    attr<<=8;  // Shift attribute to high byte
    // Print hex digits from left to right (most significant first)
    for( i=w-1; i!=-1; i-- ) {
      c = '0'+((x>>(i*4))&15); if( c>'9' ) c+='A'-('9'+1);  // Convert 0-15 to '0'-'9','A'-'F'
      *s++ = c | attr;  // Pack character and attribute
    }
    return s;  // Return pointer to next position
  }

  // Render hex dump to textblock (generates the hex view display)
  void hexdump( textblock& tb1 ) {

    uint c,i,j;
    uint cpl = F1cpl;  // Characters (bytes) per line that fit
    uint dpl = F1dpl;  // Dead space padding
    uint len = textlen;
    byte* p  = &databuf[F1pos-databeg];  // Pointer to data in cache

    qword ofs;  // Current file offset

    word* s = &tb1.cell(0,0);  // Pointer to text buffer
    word* _s = s;

    // For each line (row) in the display
    for( j=0; j<tb1.WCY; j++ ) {
      ofs = F1pos+j*BX;  // File offset for this row

      // Print address column (64-bit or 32-bit)
      if( flags&f_addr64 ) {
        s = HexPrint( s, ofs>>32, 8, pal_Addr );  // High 32 bits
        *s++ = tb1.ch(':',pal_Addr);  // Separator
      }
      s = HexPrint( s, ofs, 8, pal_Addr );  // Low 32 bits (or full 32-bit address)

      *s++ = tb1.ch(':',pal_Addr);  // Address-hex separator
      *s++ = tb1.ch(' ',pal_Hex);   // Space

      // Print hex bytes section
      for( i=0; i<cpl; i++ ) {
        s[2]=s[1]=s[0] = tb1.ch(' ',pal_Hex);  // Default to spaces
        // If byte is within viewable range (not past EOF)
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          // Print 2-digit hex value, use pal_Diff if byte differs from other files
          HexPrint( s, p[j*BX+i], 2, diffbuf[j*BX+i]?pal_Diff:pal_Hex );
        }
        s+=3;  // Move to next byte position (2 hex + 1 space)
      }
      if( F1cpl<BX ) s[-1]=tb1.ch('>',pal_Addr);  // '>' indicator if line truncated

      *s++ = tb1.ch(' ',pal_Hex);  // Space before ASCII section

      // Print ASCII representation section
      for( i=0; i<cpl; i++ ) {
        c = tb1.ch(' ',pal_Hex);  // Default to space
        // If byte is within viewable range
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          // Print byte as character (control chars will display as-is)
          c = tb1.ch( p[j*BX+i], diffbuf[j*BX+i]?pal_Diff:pal_Hex );
        }
        *s++ = tb1.ch( byte(c),pal_Hex);  // Write character
      }

      // Fill any dead space at end of line
      for( i=0; i<dpl; i++ ) *s++=tb1.ch(' ',pal_Hex);

      // Draw vertical separator if enabled (rightmost column)
      if( flags & f_vertline ) s[-1]=tb1.ch('|',pal_Sep);

    }

  }

};

//--- #include "window.inc"

// Global window handles
HWND win;       // Main window handle
HDC  dibDC;     // Device context for offscreen bitmap
PAINTSTRUCT ps; // Paint structure for WM_PAINT

// Draw animated box (dashed line with moving offset for selection indicator)
void DrawBox( HDC dc, int x1,int y1, int x2,int y2, int dx=0 ) {
  dx %= (x2-x1);  // Animate by shifting start point
  // Create a polygon path that draws the box with animated offset
  POINT a[] = { { x1+dx,y1 }, { x2,y1 }, { x2,y2 }, { x1,y2 }, { x1,y1 }, { x1+dx,y1 } };
  Polyline( dc, a, 6 );
}

// Draw line between two points with specified pen
void DrawLine( HDC dc, HGDIOBJ& hPen, int x1,int y1, int x2,int y2 ) {
  POINT a[] = { { x1,y1 }, { x2,y2 } };
  HGDIOBJ hPenOld = SelectObject( dibDC, hPen );  // Select pen
  Polyline( dc, a, 2 );  // Draw line
  SelectObject( dibDC, hPenOld );  // Restore old pen
}

// Request full window redraw
void DisplayRedraw( void ) {
  InvalidateRect( win, 0, 0 );  // Invalidate entire window
}

// Request partial window redraw (specific rectangle)
void DisplayRedraw( int x, int y, int dx, int dy ) {
  RECT v;
  v.left   = x;
  v.top    = y;
  v.right  = x+dx;
  v.bottom = y+dy;
  InvalidateRect( win, &v, 0 );  // Invalidate only specified region
}

// Move window by delta (mode: bit 0=move position, bit 1=change size)
void ShiftWindow( int x, int y, uint mode=3 ) {
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  GetWindowPlacement( win, &winpl );
  winpl.rcNormalPosition.left  += (mode&1) ? x : 0;  // Move left edge if mode&1
  winpl.rcNormalPosition.top   += (mode&1) ? y : 0;  // Move top edge if mode&1
  winpl.rcNormalPosition.right += (mode&2) ? x : 0;  // Resize width if mode&2
  winpl.rcNormalPosition.bottom+= (mode&2) ? y : 0;  // Resize height if mode&2
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}

// Set absolute window position and size
void SetWindow( HWND win, int x, int y, int w, int h ) {
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  winpl.rcNormalPosition.left  = x;
  winpl.rcNormalPosition.top   = y;
  winpl.rcNormalPosition.right = x+w;
  winpl.rcNormalPosition.bottom= y+h;  // Should be y+h (BUG: uses x+h)
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}

// Set window size (keeping current position)
void SetWindowSize( HWND win, int w, int h ) {
  int x,y;
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  GetWindowPlacement( win, &winpl );
  x = winpl.rcNormalPosition.left;   // Keep current position
  y = winpl.rcNormalPosition.top;
  winpl.rcNormalPosition.right = x+w;   // Set new width
  winpl.rcNormalPosition.bottom= y+h;   // Set new height (BUG: should be y+h, not x+h)
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}

//--- #include "config.inc"

// View configuration (saved to registry for persistence across sessions)
struct viewstate {
  LOGFONT lf;      // Font configuration (face, size, weight, etc.)
  uint BX,BY;      // Bytes per line and lines per screen (grid dimensions)
  int  cur_view;   // Currently selected file index (-1 = all files move together)
  uint f_addr64;   // 64-bit address display flag (vs 32-bit)
  uint f_help;     // Help text visible flag (F1 toggles)
};

// Default configuration: Consolas font, 32 bytes/line, no selection, 32-bit addresses, no help
viewstate lf = { {-19,-10, 0, 0, 400, 0, 0, 0, 204, 3, 2, 1, 49, "Consolas"}, 32,255, -1, 0, 0 };
viewstate lf_old;  // Backup of old config (to detect changes)

// Save configuration to registry (HKCU\Software\SRC\cmp_01\config)
void SaveConfig( ) {
  uint r,disp;
  HKEY hk;
  // Create or open registry key
  r = RegCreateKeyEx(
    HKEY_CURRENT_USER,           // Root key: current user's settings
    "Software\\SRC\\cmp_01", 0, 0,  // Subkey path
    REG_OPTION_NON_VOLATILE,     // Persist across reboots
    KEY_ALL_ACCESS,              // Full access rights
    0,                           // Default security
    &hk, (LPDWORD)&disp          // Returns handle and disposition
  );
  if( r==ERROR_SUCCESS ) {
    // Write entire viewstate structure as binary blob
    r = RegSetValueEx( hk, "config", 0, REG_BINARY, (byte*)&lf, sizeof(lf) );
    RegCloseKey( hk );
  } else {
    SetLastError(r);
    printf( "r=%X disp=%X\nError=<%s>\n", r, disp, GetErrorText() );
  }
}

// Load configuration from registry
void LoadConfig( ) {
  uint r,l=sizeof(lf);  // Expected data size
  HKEY hk;
  // Open registry key for reading
  r = RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\SRC\\cmp_01", 0, KEY_READ, &hk );
  if( r==ERROR_SUCCESS ) {
    // Read binary blob into viewstate structure
    r = RegQueryValueEx( hk, "config", 0, 0, (byte*)&lf, (LPDWORD)&l );
    RegCloseKey( hk );
  } else {
    SetLastError(r);
    printf( "r=%X\nConfig Error=<%s>\n", r, GetErrorText() );
  }
}

//--- #include "help.inc"

// Help text with ~ markers for highlighting (~ toggles between normal and highlighted colors)
char helptext[] =
"~Up~,~Down~,~Left~,~Right~,~PgUp~,~PgDn~,~Home~,~End~ = Navigation\n"
"~'X'~ = Toggle 32-bit/64-bit address; ~'R'~ = Reload file data\n"
"~Tab~ = Select a file; Navigation keys only apply to selected file\n"
"~Ctrl~-~left~/~right~ = Change row width; ~Ctrl~-~up~/~down~ = Change row number\n"
"~Space~,~F6~ = Skip to next difference; any key = stop\n"
"~'+'~/~'-'~ = Change font size; ~Ctrl~-~'+'~/~'-'~ = Change font height\n"
"~Alt~-~'+'~/~'-'~ = Change font width; ~'C'~ = Change font\n"
"~Escape~ = Quit; ~'S'~= Save GUI config; ~'L'~ = Load config\n"
;

enum{ N_VIEWS=8 };  // Maximum number of files to compare simultaneously

// Global GUI state
MSG  msg;                     // Windows message structure
uint lastkey = 0;             // Last key pressed (for key repeat detection)
uint curtim, lasttim = 0;     // Current and last key press time (for repeat detection)

mybitmap  bm1;                // Offscreen display bitmap
myfont    ch1;                // Font renderer with pre-rendered characters
hexfile   F[N_VIEWS];         // File viewers (one per file)
textblock tb[N_VIEWS];        // Text buffers for each file's display
uint F_num;                   // Actual number of files opened

textblock tb_help;            // Help text buffer (bottom of screen)
uint help_SY;                 // Help text height in lines

uint f_started = 0;           // First-time initialization flag (prevents re-init)
volatile uint f_busy = 0;     // Background difference scan in progress flag

// Background thread for scanning to next difference (Space/F6 key)
// Scans forward through files looking for next byte that differs
struct DiffScan : thread<DiffScan> {

  typedef thread<DiffScan> base;

  // Start thread and set busy flag
  uint start( void ) {
    f_busy=1;  // Signal that scan is in progress
    return base::start();  // Start background thread
  }

  // Thread function - scans forward until difference or EOF
  void thread( void ) {
    uint c,i,j,d,x[N_VIEWS],ff_num,delta,flag;
    qword pos_delta=0;  // Total distance scanned

    // Start scanning from next screen (skip current view)
    for(i=0;i<F_num;i++) F[i].MoveFilepos(F[0].textlen);

    // Continue scanning while not cancelled by user
    while( f_busy ) {
      delta=0;  // Count of matching bytes in current view
      // Check each byte in current view
      for( j=0; j<F[0].textlen; j++ ) {
        c=0; d=-1; ff_num=0;
        // Compare this byte across all files
        for(i=0;i<F_num;i++) {
          x[i] = F[i].viewdata(j);      // Get byte from file i
          ff_num += (x[i]==-1);         // Count how many files are at EOF
          if( x[i]!=-1 ) c|=x[i],d&=x[i];  // Accumulate OR and AND of all bytes
        }
        // Check if all files have same value (c==x[i] && d==x[i] means all bits match)
        for(flag=1,i=0;i<F_num;i++) flag &= ((c==x[i])&&(d==x[i]));
        delta += flag;  // Count matching bytes
      }

      // Stop if all files at EOF
      if( ff_num>=F_num ) break;
      pos_delta += delta;
      // Stop if found difference (delta < textlen means some bytes didn't match)
      if( delta<F[0].textlen ) break;
      // Continue to next screen (all bytes matched)
      for(i=0;i<F_num;i++) F[i].MoveFilepos(delta);
    }

    f_busy=0;           // Clear busy flag
    DisplayRedraw();    // Trigger redraw to show result
    return;
  }

};

DiffScan diffscan;  // Global difference scanner thread

HINSTANCE g_hInstance = GetModuleHandle(0);  // Application instance handle

// Minimal window procedure (we handle everything in message loop, not here)
LRESULT CALLBACK wndproc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) {
  // Forward all messages to default handler - we process them in GetMessage loop
  return DefWindowProc(hWnd, message, wParam, lParam);
}

// Window class definition
WNDCLASS wndclass = {
  0,             // style
  wndproc,       // window procedure
  0,0,           // class and window extra bytes
  g_hInstance,   // instance handle
  0,0,           // icon, cursor
  0,             // background brush
  0,             // menu name
  "wndclass"     // class name
};

// Main entry point - WinMain is entry point for GUI Windows applications
int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {
  int argc=__argc; char **argv=__argv;  // Get command-line arguments

  uint c,i,j,l;

  char* fil1 = argv[0];  // First filename (defaults to exe name)

  // Open files from command line (at least 1, up to N_VIEWS files)
  for( i=1; i<Min(DIM(F)+1,Max(2,argc)); i++ ) {
    if( i<argc ) fil1=argv[i];  // Use command-line arg if available
    if( F[i-1].Open(fil1)==0 ) return 1;  // Open file, exit on failure
    // Enable 64-bit addresses if any file is >4GB
    if( F[i-1].F1size>0xFFFFFFFFU ) lf.f_addr64=hexfile::f_addr64;
  }
  F_num = i-1;  // Store actual number of files opened

  LoadConfig();  // Load saved configuration from registry
  tb_help.textsize( helptext, 0, &help_SY );  // Calculate help text height

  // Get window frame metrics (borders, titlebar) to calculate client area
  const int wfr_x = GetSystemMetrics(SM_CXFIXEDFRAME) + GetSystemMetrics(SM_CXBORDER);
  const int wfr_y = GetSystemMetrics(SM_CYFIXEDFRAME);
  const int wfr_c = GetSystemMetrics(SM_CYCAPTION);  // Titlebar height
  RECT scr; SystemParametersInfo( SPI_GETWORKAREA,0,&scr,0 );  // Get work area (screen minus taskbar)
  const int scr_w = scr.right-scr.left;   // Available screen width
  const int scr_h = scr.bottom-scr.top;   // Available screen height

  printf( "wfr_x=%i\n", wfr_x );  // Debug output

  // Create offscreen DC and bitmap (for double-buffering)
  dibDC = CreateCompatibleDC( GetDC(0) );
  bm1.AllocBitmap( dibDC, scr_w, scr_h );  // Allocate bitmap as large as screen

  RegisterClass( &wndclass );  // Register window class

  // Create main window (initially fills entire work area)
  win = CreateWindow(
   "wndclass",                      // Window class name
    "File Comparator  (F1=help)",   // Window title
    WS_SYSMENU,                      // Style: system menu only (no resize, minimize, maximize)
    scr.left,scr.top,                // Position: top-left of work area
    scr_w,scr_h,                     // Size: fill work area
    0,                               // No parent window
    0,                               // No menu
    GetModuleHandle(0),              // Application instance
    0                                // No creation parameters
  );

  // Set window icon from application resources (ID=500)
  HICON ico = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(500));
#ifndef _WIN64
  SetClassLong( win, GCL_HICON, LONG(ico) );  // 32-bit version
#else
  SetClassLongPtr(win, GCLP_HICON, (LONG_PTR)ico);  // 64-bit version
#endif
  DeleteObject(ico);  // Delete icon handle

  // Create pens for UI drawing (selection box and help separator)
  LOGBRUSH lb;
  HGDIOBJ hPen, hPenOld, hPen_help;
  lb.lbStyle = BS_SOLID;
  lb.lbColor = RGB(0,0xFF,0);  // Green
  lb.lbHatch = HS_CROSS;
  hPen = ExtCreatePen( PS_GEOMETRIC|PS_DASH, 5, &lb, 0, NULL );  // Dashed green pen for selection box
  uint pen_shift = 0;  // Animated dash offset

  lb.lbStyle = BS_SOLID;
  lb.lbColor = RGB(0xFF,0xFF,0xFF);  // White
  lb.lbHatch = HS_HORIZONTAL;
  hPen_help = ExtCreatePen( PS_GEOMETRIC, 2, &lb, 0, NULL );  // Solid white pen for help separator

  int delta;      // Movement delta for navigation
  int rp1,rp,alt,ctr;  // Key repeat flags, alt key, control key
  uint mBX,mBY;   // Maximum bytes per line and lines that fit on screen
  int  WX,WY;     // Window size in pixels

  // Restart point for GUI reconfiguration (font change, display size change, etc.)
  // This label allows reinitialization without restarting the entire program
  if(0) {
    Restart:  // Jump here when configuration changes
    // Cleanup old resources
    for(i=0;i<F_num;i++ ) {
      tb[i].Quit();   // Free text buffers
      F[i].Quit();    // Free file resources
    }
    ch1.Quit();  // Free font resources
  }

  // Initialize font from saved configuration
  memcpy( &ch1.lf, &lf.lf, sizeof(ch1.lf) );
  ch1.DumpLF();  // Debug: print font info

  ch1.InitFont();         // Initialize font structure
  ch1.SetFont(dibDC,lf.lf);  // Create font and pre-render all characters

  // Calculate maximum window size that fits on screen (binary search from bit 15 down to 0)
  // This finds the largest grid dimensions (mBX bytes wide, mBY lines tall) that fit
  mBX=0; mBY=0;
  for( j=15; j!=-1; j-- ) {
    mBX |= (1<<j);  // Try setting this bit (assume we can fit this many bytes/line)
    mBY |= (1<<j);  // Try setting this bit (assume we can fit this many lines)

    // Calculate required width for all file views side-by-side
    WX = 2*wfr_x;  // Start with frame borders
    for(i=0;i<F_num;i++) WX += F[i].Calc_WCX( mBX, lf.f_addr64, (i!=F_num-1) ) * ch1.wmax;

    // Calculate required height (hex grid + optional help text + frame)
    WY = mBY*ch1.hmax + lf.f_help* help_SY*ch1.hmax + 2*wfr_y+wfr_c;

    // If doesn't fit, clear this bit
    if( WX>bm1.bmX ) mBX&=~(1<<j);
    if( WY>bm1.bmY ) mBY&=~(1<<j);
  }
  mBX = Max(1U,mBX);  // At least 1 byte wide
  mBY = Max(1U,mBY);  // At least 1 line tall
  mBX = Min(mBX,lf.BX);  // Don't exceed configured width
  if( f_started==0 ) lf.BX=mBX;  // On first run, set configured width to maximum
  if( lf.BY>mBY ) lf.BY=mBY;  // Clamp configured height to maximum
  // Ensure buffer doesn't exceed file cache size (1MB - 64KB alignment = max buffer)
  if( lf.BX*lf.BY>hexfile::datalen-hexfile::datalign ) lf.BX=(hexfile::datalen-hexfile::datalign)/lf.BY;

  printf( "mBX=%i mBY=%i BX=%i BY=%i\n", mBX, mBY, lf.BX, lf.BY );  // Debug output

  // Initialize text buffers for each file view (layout side-by-side)
  WX=0*wfr_x;
  for(i=0;i<F_num;i++ ) {
    uint WCX = F[i].Calc_WCX( mBX, lf.f_addr64, (i!=F_num-1) );  // Chars needed per line
    tb[i].Init( ch1, WCX,lf.BY, WX,0 );  // Create text buffer at horizontal position WX
    WX += WCX*ch1.wmax;  // Advance horizontal position for next file
    F[i].SetTextbuf( tb[i], lf.BX, ((i!=F_num-1)?hexfile::f_vertline:0) | lf.f_addr64);
    F[i].SetFilepos( F[i].F1pos );  // Initialize file position (loads cache)
  }
  WX+=2*wfr_x;  // Add frame borders to total width

  // Initialize help text buffer if enabled
  WY = tb[0].WCY*ch1.hmax;  // Height of hex display in pixels
  if( lf.f_help ) {
    tb_help.Init( ch1, WX/ch1.wmax,help_SY, 0,WY );  // Position help below hex display
    tb_help.Fill( ' ', pal_Help1 );  // Fill with spaces
    tb_help.Print( helptext, pal_Help1,pal_Help2, 1,0 );  // Print help text
    WY += help_SY*ch1.hmax;  // Add help height to total
  }
  WY += 2*wfr_y+wfr_c;  // Add frame borders to total height

  printf( "WX=%i WY=%i bmX=%i bmY=%i\n", WX,WY,bm1.bmX,bm1.bmY );  // Debug output

  SetWindowSize( win, WX,WY );  // Resize window to fit content exactly

  SetFocus( win );  // Give window keyboard focus
  ShowWindow( win, SW_SHOW );  // Make window visible
  SetTimer( win, 0, 100, 0 );  // Timer for animated selection box (100ms interval)
  DisplayRedraw();  // Request initial paint
  f_started=1;  // Mark as initialized
  lf_old = lf;  // Save config for change detection

  // Main message loop - processes Windows messages (keyboard, mouse, paint, timer)
  while( GetMessage(&msg,win,0,0) ) {

    if(0) { m_break: break; }  // Exit label (jumped to from WM_CLOSE)

    switch( msg.message ) {

    case WM_MOUSEWHEEL:
      // Mouse wheel: convert delta to navigation type (8=wheel up, 9=wheel down)
      delta = (short)HIWORD(msg.wParam);  // Get scroll delta (positive=up, negative=down)
      delta = (delta<0) ? 9 : 8; goto MovePos;

    case WM_CLOSE: case WM_NULL:
      // Window close: exit message loop
      goto m_break;

    case WM_TIMER:
      // Timer tick (100ms): redraw for animated selection box
      DisplayRedraw();
      break;

    case WM_PAINT:
      // Paint: render hex display to screen

      if( f_busy==0 ) {

        bm1.Reset();

        // Compare all files and mark differences
        {
          uint d,x[N_VIEWS];
          for( j=0; j<F[0].textlen; j++ ) {
            c=0; d=-1;
            for(i=0;i<F_num;i++) {
              x[i] = F[i].viewdata(j);
              if( x[i]!=-1 )
              c|=x[i],d&=x[i];
            }
            // Mark position as different if not all files have same byte
            for(i=0;i<F_num;i++) F[i].diffbuf[j]=((c!=x[i])||(d!=x[i]));
          }
        }

        // Render all file views
        for(i=0;i<F_num;i++) F[i].hexdump(tb[i]);
        for(i=0;i<F_num;i++) tb[i].Print(ch1,bm1);

        // Render help text
        if( lf.f_help ) {
          DrawLine(dibDC,hPen_help, tb_help.WPX,tb_help.WPY, tb_help.WPX+tb_help.WSX,tb_help.WPY );
          tb_help.Print(ch1,bm1);
        }

        // Draw selection box around active file view
        if( (lf.cur_view>=0) && (lf.cur_view<F_num) ) {
          i = lf.cur_view;
          hPenOld = SelectObject( dibDC, hPen );
          printf( "!i=%i WSX=%i WCX=%i!\n", i, tb[i].WSX, tb[i].WCX );
          DrawBox( dibDC, tb[i].WPX-2+5*(i==0),tb[i].WPY, tb[i].WPX-2+tb[i].WSX-5*(i!=F_num-1),tb[i].WPY+tb[i].WSY-5, pen_shift );
          SelectObject( dibDC, hPenOld );
          pen_shift++;
        }

      }

      // Blit offscreen buffer to window
      {
        HDC DC = BeginPaint( win, &ps );
        BitBlt( DC, 0,0, WX,WY,  dibDC, 0,0, SRCCOPY );
        EndPaint( win, &ps );
      }
      break;

    case WM_KEYDOWN: case WM_SYSKEYDOWN:
      curtim = GetTickCount();
      {
        // Detect key repeat
        rp1 = (msg.wParam==lastkey) && (curtim>lasttim+1000);
        rp  = ((msg.lParam>>30)&1) ? 4*(rp1+1) : 1;
        alt = ((msg.lParam>>29)&1);
        ctr = (GetKeyState(VK_CONTROL)>>31)&1;
        c = msg.wParam;
        if( (c>='0') && (c<='9') ) c='0';  // Treat all digits as same key

        printf( "WM_KEYDOWN: w=%04X l=%04X c=%X alt=%i ctr=%i rp1=%i f_busy=%X\n", msg.wParam, msg.lParam, c,alt,ctr,rp1, f_busy );
        
        // Stop background scan on any key
        if( f_busy ) { f_busy=0; diffscan.quit(); }

        // Handle keyboard commands
        switch( c ) {
          case VK_ESCAPE:
            exit(0);

          case VK_F1:
            lf.f_help ^= 1;
            goto Restart;

          case VK_TAB:
            lf.cur_view = ((lf.cur_view+1+1) % (F_num+1)) -1;
            goto Redraw;

          case VK_SPACE: case VK_F6:
            diffscan.start();
            break;

          case 'R': // Reload file data
            for(j=0;j<F_num;j++) {
              i = (lf.cur_view==-1) ? j : lf.cur_view;
              F[i].databeg=F[i].dataend=0;
            }
            goto MovePos0;

          case 'C': // Change font
            ch1.SelectFont();
            lf.lf=ch1.lf;
            goto Restart;

          case 'X': // Toggle 32/64-bit address display
            lf.f_addr64 ^= hexfile::f_addr64;
            goto Restart;

          case 'S': // Save config
            SaveConfig();
            goto Restart;

          case 'L': // Load config
            LoadConfig();
            goto Restart;

          case VK_OEM_MINUS: case VK_SUBTRACT:
            if( ( ctr) || (!alt) ) if( lf.lf.lfHeight<-1 ) lf.lf.lfHeight++;
            if( (!ctr) || ( alt) ) if( lf.lf.lfWidth<-1 ) lf.lf.lfWidth++; 
            goto Restart;

          case VK_OEM_PLUS: case VK_ADD:
            if( ( ctr) || (!alt) ) lf.lf.lfHeight--; 
            if( (!ctr) || ( alt) ) lf.lf.lfWidth--; 
            goto Restart;

          case VK_LEFT:
            if( ctr ) { if( lf.BX>1 ) lf.BX--; goto Restart; }
            delta=2; goto MovePos;

          case VK_RIGHT:
            if( ctr ) { lf.BX++; goto Restart; }
            delta=3; goto MovePos;

          case VK_UP:
            if( ctr ) { if( lf.BY>1 ) lf.BY--; goto Restart; }
            delta=4; goto MovePos;

          case VK_DOWN:
            if( ctr ) { lf.BY++; goto Restart; }
            delta=5; goto MovePos;

          case VK_PRIOR:
            delta=6; goto MovePos;

          case VK_NEXT:
            delta=7; goto MovePos;

          case VK_END:
            delta=1; goto MovePos;

          case VK_HOME:
MovePos0:
            delta=0;
MovePos:
            // Apply movement to selected file or all files
            if( lf.cur_view==-1 ) {
              for(i=0;i<F_num;i++) F[i].MovePos(delta);
            } else {
              F[lf.cur_view].MovePos(delta);
            }
Redraw:
            DisplayRedraw(); break;

          default:;
        }
      }

      if( lastkey!=msg.wParam ) lasttim = curtim;
      lastkey = msg.wParam;
      break;

    default:
      DispatchMessage(&msg);
    }
  }

  return 0;
}
