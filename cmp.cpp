// Hex File Comparator - A visual side-by-side hex viewer and file comparison tool
// Supports comparing up to 8 files simultaneously with highlighted differences
// Features: hex/ASCII display, configurable fonts, keyboard navigation, difference scanning

#include <windows.h>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"comdlg32.lib")

//--- #include "common.inc"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <memory.h>
#undef EOF

#pragma pack(1)

// Type aliases for consistent width types
typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;
typedef signed long long sqword;

// Zero-initialization templates for various data types
template <class T> void bzero( T &_p ) { int i; byte* p = (byte*)&_p; for( i=0; i<sizeof(_p); i++ ) p[i]=0; }
template <class T, int N> void bzero( T (&p)[N] ) { int i; for( i=0; i<N; i++ ) p[i]=0; }
template <class T> void bzero( T* p, int N ) { int i; for( i=0; i<N; i++ ) p[i]=0; }
template <class T, int N, int M>  void bzero(T (&p)[N][M]) { for( int i=0; i<N; i++ ) for( int j=0; j<M; j++ ) p[i][j] = 0; }

// Utility templates
template <class T> T Min( T x, T y ) { return (x<y) ? x : y; }
template <class T> T Max( T x, T y ) { return (x>y) ? x : y; }
template <class T,int N> int DIM( T (&wr)[N] ) { return sizeof(wr)/sizeof(wr[0]); };
#define AlignUp(x,r) ((x)+((r)-1))/(r)*(r)

// Compile-time word constant generator for FourCC-style codes
template<byte a,byte b,byte c,byte d> struct wc { 
  static const unsigned int n=(d<<24)+(c<<16)+(b<<8)+a; 
  static const unsigned int x=(a<<24)+(b<<16)+(c<<8)+d;
};

// Compiler-specific attributes for function inlining and alignment
#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) 
 #define NOINLINE __attribute__((noinline))
 #define ALIGN(n) __attribute__((aligned(n)))
 #define __assume_aligned(x,y) 
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
 #define ALIGN(n) __declspec(align(n))
#endif

// Compiler compatibility for __builtin_expect and __assume
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 #define __builtin_expect(x,y) (x)
 #define __assume_aligned(x,y) 
#endif

#if !defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 #define __assume(x) (x)
#endif

// Get file length using stdio
static uint flen( FILE* f ) {
  fseek( f, 0, SEEK_END );
  uint len = ftell(f);
  fseek( f, 0, SEEK_SET );
  return len;
}

// Platform detection macros
#if defined(__x86_64) || defined(_M_X64)
 #define X64
 #define X64flag 1
#else
 #undef X64
 #define X64flag 0
#endif

//--- #include "file_api_win.inc"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Additional virtual key code definitions not always in windows.h
#ifndef VK_OEM_PLUS
#define VK_OEM_PLUS   0xBB  // '+' key
#endif
#ifndef VK_OEM_MINUS
#define VK_OEM_MINUS  0xBD  // '-' key
#endif

//--- #include "file_win.inc"

// File attribute flags for Win32 CreateFile operations
enum {
  ffWRITE_THROUGH         =0x80000000,
  ffOVERLAPPED            =0x40000000,
  ffNO_BUFFERING          =0x20000000,
  ffRANDOM_ACCESS         =0x10000000,
  ffSEQUENTIAL_SCAN       =0x08000000,
  ffDELETE_ON_CLOSE       =0x04000000,
  ffBACKUP_SEMANTICS      =0x02000000,
  ffPOSIX_SEMANTICS       =0x01000000,
  ffOPEN_REPARSE_POINT    =0x00200000,
  ffOPEN_NO_RECALL        =0x00100000
};

// Low-level Win32 file opening wrapper
HANDLE Win32_Open( const wchar_t* s, uint Flags, uint Attrs ) {
  return CreateFileW( 
    s,
    GENERIC_READ | GENERIC_WRITE,
    0,
    0,
    Flags,
    Attrs,
    0
  );
}

// Open a directory handle (for directory operations)
HANDLE file_opendir( const wchar_t* s ) {
  HANDLE r = CreateFileW( 
     s,
     GENERIC_WRITE,
     FILE_SHARE_READ | FILE_SHARE_WRITE,
     0,
     OPEN_EXISTING,
     FILE_FLAG_BACKUP_SEMANTICS,
     0
   );
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create a directory
int file_mkdir( const wchar_t* name ) {
  return CreateDirectoryW( name, 0 );
}

// Global file operation modes
uint file_open_mode = GENERIC_READ;
void file_open_mode_r( void ) { file_open_mode = GENERIC_READ; }
void file_open_mode_rw( void ) { file_open_mode = GENERIC_READ | GENERIC_WRITE; }

uint file_make_mode = GENERIC_WRITE;
void file_make_mode_w( void ) { file_make_mode = GENERIC_WRITE; }
void file_make_mode_rw( void ) { file_make_mode = GENERIC_READ | GENERIC_WRITE; }

uint file_make_cmode = CREATE_ALWAYS;
void file_make_open( void ) { file_make_cmode = OPEN_ALWAYS; }
void file_make_create( void ) { file_make_cmode = CREATE_ALWAYS; }

// Open existing file (wide-char version)
HANDLE file_open( const wchar_t* name ) { 
  HANDLE r = CreateFileW( 
     name,
     file_open_mode,
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
     0,
     OPEN_EXISTING,
     0,
     0
  );
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Open existing file (ANSI version)
HANDLE file_open( const char* name ) { 
  HANDLE r = CreateFileA(
     name,
     file_open_mode,
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
     0,
     OPEN_EXISTING,
     0,
     0
  );
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create or open file (wide-char version)
HANDLE file_make( const wchar_t* name ) { 
  HANDLE r = CreateFileW( 
     name,
     file_make_mode,
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
     0,
     file_make_cmode,
     0,
     0
  );
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create or open file (ANSI version)
HANDLE file_make( const char* name ) { 
  HANDLE r = CreateFileA( 
     name,
     file_make_mode,
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
     0,
     file_make_cmode,
     0,
     0
  );
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Close file handle
int file_close( HANDLE file ) {
  return CloseHandle( file );
}

// Read from file (may return partial read)
uint file_read( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r = 0;
  ReadFile( file, buf, len, (LPDWORD)&r, 0 );
  return r;
}

// Synchronized read - keeps reading until requested length or EOF
uint file_sread( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r;
  uint flag=1;
  uint l = 0;

  do {
    r = 0;
    flag = ReadFile( file, buf+l, len-l, (LPDWORD)&r, 0 );
    l += r;
  } while( (r>0) && (l<len) && flag );

  return l;
}

// Write to file
uint file_writ( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r = 0;
  WriteFile( file, buf, len, (LPDWORD)&r, 0 );
  return r;
}

// Seek to position in file
qword file_seek( HANDLE file, qword ofs, int typ = FILE_BEGIN ) {
  uint lo,hi;
  lo = uint(ofs);
  hi = uint(ofs>>32);
  lo = SetFilePointer( file, lo, (PLONG)&hi, typ );
  ofs = hi;
  ofs = (ofs<<32) + lo;
  return ofs;
}

// Get current file position
qword file_tell( HANDLE file ) {
  return file_seek( file, 0, FILE_CURRENT );
}

// Get file size using binary search (for files where normal method fails)
qword getfilesize( HANDLE f ) {
  qword pos=-1LL;
  const uint bufsize=1<<16,bufalign=1<<12;
  byte* _buf = new byte[bufsize+bufalign];
  if( _buf!=0 ) {
    // Align buffer to page boundary
    byte* buf = _buf + bufalign-((_buf-((byte*)0))%bufalign);
    uint bit,len;
    pos = 0;
    // Binary search for file end
    for( bit=62; bit!=-1; bit-- ) {
      pos |= 1ULL << bit;
      file_seek(f,pos);
      len = file_read(f,buf,bufsize);
      if( (len!=0) && (len<bufsize) ) { pos+=len; break; }
      if( len>=bufsize ) continue;
      pos &= ~(1ULL<<bit);
    }
    delete[] _buf;
  }
  return pos;
}

// Get file size
qword file_size( HANDLE file ) {
  qword t = file_tell(file);
  qword r = file_seek( file, 0, FILE_END );
  if( uint(r)==0xFFFFFFFF ) {
    // SetFilePointer returns 0xFFFFFFFF on error
    if( GetLastError()!=0 ) r = getfilesize(file);
  }
  t = file_seek( file, t );
  return r;
}

// Get last error as text string
char* GetErrorText( void ) { 
  wchar_t* lpMsgBuf;
  DWORD dw = GetLastError(); 
  FormatMessageW(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL 
  );
  static char out[32768];
  int wl=wcslen(lpMsgBuf);
  WideCharToMultiByte( CP_OEMCP, 0, lpMsgBuf,wl+1, out,sizeof(out), 0,0 );
  LocalFree(lpMsgBuf);
  wl = strlen(out);
  // Remove trailing whitespace/control chars
  for( --wl; wl>=0; wl-- ) if( (byte&)out[wl]<32 ) out[wl]=' ';
  return out;
}

// Expand path to full UNC path with \\?\ prefix for long path support
uint ExpandPath( wchar_t* path, wchar_t* w, uint wsize ) {
  wcscpy( w, L"\\\\?\\" );
  GetFullPathNameW( path, wsize/sizeof(wchar_t)-4,&w[4], 0 );
  if( (w[4]=='\\') && (w[5]=='\\') ) wcscpy( &w[0], &w[4] );
  return wcslen(w);
}

// File handle wrapper providing convenient C++ interface
struct filehandle0 {

  HANDLE f;

  // Allow implicit conversion to int for boolean checks
  operator int( void ) {
    return ((byte*)f)-((byte*)0);
  }

  template< typename CHAR >
  uint open( const CHAR* name ) {
    f = file_open( name );
    return ((byte*)f)-((byte*)0);
  }

  template< typename CHAR >
  uint make( const CHAR* name ) {
    f = file_make( name );
    return ((byte*)f)-((byte*)0);
  }

  int close( void ) {
    return file_close(f);
  }

  qword size( void ) { return file_size(f); }

  void seek( qword ofs, uint typ=FILE_BEGIN ) {
    file_seek( f, ofs, typ );
  }

  qword tell( void ) {
    return file_tell(f);
  }

  template< typename BUF >
  uint read( BUF& buf ) { return read( &buf, sizeof(buf) )!=sizeof(buf); }

  uint read( void* _buf, uint len ) { return file_read( f, _buf, len ); }

  template< typename BUF >
  uint sread( BUF& buf ) { return sread( &buf, sizeof(buf) )!=sizeof(buf); }

  uint sread( void* _buf, uint len ) { return file_sread( f, _buf, len ); }

  // Read single character
  int Getc( void ) {
    byte c;
    uint l = read(c);
    return l ? -1 : c;
  }

  uint writ( void* _buf, uint len ) {
    return file_writ( f, _buf, len );
  }

  template< typename BUF >
  uint writ( BUF& buf ) {
    return writ( &buf, sizeof(buf) )!=sizeof(buf);
  }

};

// File handle with automatic initialization
struct filehandle : filehandle0 {
  filehandle() { f=0; }

  template< typename CHAR >
  filehandle( const CHAR* name, uint f_wr=0 ) {
    f_wr ? make(name) : open(name);
  }
};

//--- #include "thread.inc"

// Template wrapper for Win32 thread creation
template <class child> 
struct thread {

  HANDLE th;

  // Start the thread
  uint start( void ) {
    th = CreateThread( 0, 0, &thread_w, this, 0, 0 );
    return th!=0;
  }

  // Wait for thread to finish and cleanup
  void quit( void ) {
    WaitForSingleObject( th, INFINITE );
    CloseHandle( th );
  }

  // Static wrapper to redirect to member function
  static DWORD WINAPI thread_w( LPVOID lpParameter ) { ((child*)lpParameter)->thread(); return 0; }
};

// Sleep for a short duration in thread
void thread_wait( void ) { 
  Sleep(10); 
} 

//--- #include "bitmap.inc"

// Allocate a DIB (Device Independent Bitmap) section for drawing
HBITMAP AllocBitmap( HDC dibDC, byte*& bitmap, uint bmX, uint bmY ) {
  BITMAPINFO bm = { sizeof(BITMAPINFOHEADER), int(bmX), -int(bmY), 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
  HBITMAP dib = CreateDIBSection( dibDC, &bm, DIB_RGB_COLORS, (void**)&bitmap, 0,0 );
  SelectObject( dibDC, dib );
  return dib;
}

// Bitmap wrapper with pixel access
struct mybitmap {

  uint bmX, bmY;
  byte* bitmap;
  HBITMAP dib;

  void AllocBitmap( HDC dibDC, uint _bmX, uint _bmY ) {
    bitmap = 0;
    bmX = _bmX; bmY = _bmY;
    dib = ::AllocBitmap( dibDC, bitmap, bmX, bmY );
  }

  // Clear bitmap to black
  void Reset() {
    memset( bitmap, 0, 4*bmX*bmY );
  }

  // Access pixel at (x,y) as 32-bit RGBA value
  uint& pixel( uint x, uint y ) { 
    return (uint&)bitmap[(y*bmX+x)*4]; 
  }

};

//--- #include "setfont.inc"

// Font management and pre-rendering
struct myfont {
  HFONT hFont;
  LOGFONT lf;
  uint* fontbuf1;  // Pre-rendered character bitmaps
  int wmin,wmax,hmin,hmax;  // Character cell dimensions
  int wbuf[256];  // Width of each character
  int hbuf[256];  // Height of each character

  void InitFont( void ) {
    hFont=0;
    bzero(lf);
  }

  void Quit( void ) {
    delete fontbuf1;
    DeleteObject( hFont ); hFont=0;
  }

  // Debug: print LOGFONT structure
  void DumpLF() {
    uint* x= (uint*)&lf;
    byte* y= (byte*)&lf.lfItalic; 
    uint i;
    printf( "{ " );
    for( i=0; i<5; i++ ) printf( "%i,", x[i] ); printf( "  " );
    for( i=0; i<8; i++ ) printf( "%i,", y[i] ); printf( "  " );
    printf( "\"%s\" }\n", lf.lfFaceName );
  }

  // Show font selection dialog
  void SelectFont( void ) {
    CHOOSEFONT cf;
    bzero(cf);
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = 0;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FIXEDPITCHONLY | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS;
    if( ChooseFont(&cf) ) DumpLF();
  }

  // Measure character widths and heights
  void GetFontWidth( HDC dibDC ) {
    char s[2] = {0,0}; SIZE cs;
    int i;
    for( i=0; i<256; i++ ) wbuf[i]=0;

    for( i=0; i<256; i++ ) {
      s[0]=i; cs.cx=cs.cy=0;
      GetTextExtentPoint32( dibDC, s,1, &cs );
      wbuf[i] = cs.cx;
      hbuf[i] = cs.cy;
    }

    // Find min/max character dimensions
    wmin=wbuf[0]; hmin=hbuf[0];
    wmax=0;       hmax=0;
    for( i=0; i<256; i++ ) {
      wmin=Min(wmin,wbuf[i]), wmax=Max(wmax,wbuf[i]);
      hmin=Min(hmin,hbuf[i]), hmax=Max(hmax,hbuf[i]);
    }

    printf( "wmin=%i wmax=%i\n", wmin,wmax );
    printf( "hmin=%i hmax=%i\n", hmin,hmax );
  }

  // Set font and pre-render all characters
  void SetFont( HDC dibDC, LOGFONT& _lf ) {
    HFONT currfont = hFont;
    lf = _lf;
    hFont = CreateFontIndirect( &lf );
    if( currfont!=0 ) DeleteObject( currfont );
    SelectObject( dibDC, hFont );

    byte* bitmap = 0;

    GetFontWidth(dibDC);

    // Create temporary DC and bitmap for pre-rendering
    HDC DC0 = GetDC(0);
    HDC DC = CreateCompatibleDC( DC0 );

    HBITMAP dib = AllocBitmap( DC, bitmap, wmax, hmax );
    if( bitmap!=0 ) {
      SelectObject( DC, hFont );
      PrecalcFont( DC, bitmap, wmax );
      DeleteObject(dib);
    }

    ReleaseDC(0,DC0);
    DeleteDC(DC);
  }

  // Pre-render all 256 characters to bitmap buffer for fast blitting
  void PrecalcFont( HDC dibDC, byte* bitmap, uint bmX ) {
    uint c,x,y,i,j; char s[2];
    SetBkMode(dibDC, TRANSPARENT);
    SetTextAlign( dibDC, TA_TOP|TA_LEFT );
    SetBkColor(dibDC,   0x000000 );
    SetTextColor(dibDC, 0xFFFFFF );

    uint* fontbuf = new uint[ 256*wmax*hmax ];
    if( fontbuf!=0 ) {
      HBRUSH hbr = CreateSolidBrush(0x000000);
      RECT rect = { 0,0,wmax,hmax };

      // Render each character
      for( c=0; c<256; c++ ) {

        FillRect(dibDC, &rect, hbr ); 

        s[0]=c; s[1]=0;
        TextOut( dibDC, 0, 0, s, 1 ); 

        // Copy rendered character to buffer
        for( y=0; y<hmax; y++ ) {
          for( x=0; x<wmax; x++ ) {
            i = (y*bmX + x)*4;
            fontbuf[(c*hmax+y)*wmax+x] = (uint&)bitmap[i];
          }
        }
      }

      DeleteObject( hbr );
    }

    fontbuf1 = fontbuf;
  }

};

//--- #include "palette.inc"

// Color pair (foreground and background)
struct color {
  uint fg,bk;
};

// Palette indices for different UI elements
enum pal_ {
  pal_Sep=0,    // Separator lines
  pal_Hex,      // Normal hex display
  pal_Addr,     // Address column
  pal_Diff,     // Highlighted differences

  pal_Help1,    // Help text normal
  pal_Help2,    // Help text highlighted

  pal_MAX
};

// Color palette definition
color palette[pal_MAX] = {
  { 0xFFFFFF, 0x000000 },
  { 0xFFFF55, 0x000000 },
  { 0x00AA00, 0x000000 },
  { 0xFFFFFF, 0x0000AA },

  { 0x00FFFF, 0x000080 },
  { 0xFFFFFF, 0x000080 },
};

//--- #include "textblock.inc"

// Text buffer for displaying character grid
struct textblock {
  uint  WCX,WCY;  // Window size in characters
  uint  WPX,WPY;  // Position on screen in pixels
  uint  WSX,WSY;  // Size in pixels
  uint  blksize;
  word* text;     // Character + attribute buffer

  void Clear( void ) {
    uint i;
    if( text!=0 ) for( i=0; i<blksize; i++ ) text[i]=0;
  }

  template< class myfont >
  void Init( myfont& ch1, uint CX, uint CY, uint PX, uint PY ) {
    WCX=CX; WCY=CY; blksize=WCY*WCX;
    WPX=PX; WPY=PY;
    WSX = WCX*ch1.wmax;
    WSY = WCY*ch1.hmax;
    text = new word[WCY*WCX];
  }

  void Quit( void ) {
    delete text;
  }

  // Access character cell at (x,y)
  word& cell( uint x, uint y ) { 
    return (word&)text[y*WCX+x]; 
  }

  // Pack character and attribute into word
  word ch( byte c, byte attr ) {
    return c | (attr<<8);
  }

  // Fill entire buffer with character+attribute
  void Fill( uint c, uint atr ) {
    uint i;
    for( i=0; i<blksize; i++ ) text[i]=ch(c,atr);
  }

  // Print text string to textblock with attribute toggling ('~' toggles between atr1 and atr2)
  void Print( char* s, uint atr1, uint atr2=0, uint X=0, uint Y=0 ) {
    uint c,a=atr1,x=X,y=Y;
    while( c=*s++ ) {
      if( c=='\n' ) { x=X; y++; continue; }
      if( c=='~' ) { a^=atr1^atr2; continue; }
      if( (x<WCX) && (y<WCY) ) cell(x,y) = ch(c,a);
      x++;
    }
  }

  // Calculate text size (in characters)
  void textsize( char* s, uint* tb_SX, uint* tb_SY ) {
    uint c,x=0,y=0;
    while( c=*s++ ) {
      if( c=='\n' ) { x=0; y++; continue; }
      if( c=='~' ) continue;
      x++;
    }
    if(tb_SX!=0) *tb_SX=x;
    if(tb_SY!=0) *tb_SY=y;
  }

  // Render textblock to bitmap using font
  template< class myfont, class mybitmap >
  void Print( myfont& ch1, mybitmap& bm1 ) {
    uint i,j;
    word* s = text;
    uint PX=WPX, PY=WPY;

    SymbOut_Init();

    for( j=0; j<WCY; j++ ) {
      for( i=0; i<WCX; i++ ) SymbOut( ch1, bm1,PX+i*ch1.wmax,PY, s[i] );
      PY+=ch1.hmax;
      s+=WCX;
    }
  }

//--- #include "textprint.inc"

// Character rendering cache - stores pointers to last rendered positions
uint* cache_ptr[pal_MAX][256];

void SymbOut_Init( void ) {
  bzero( cache_ptr );
}

// Optimized character rendering with caching
// Instead of re-rendering, copies from previous location if same char+palette was already drawn
template< class myfont, class mybitmap >
void SymbOut( myfont& ch1, mybitmap& bm1,uint PX,uint PY, word cp ) {
  uint i,j,k;

  uint chridx=byte(cp), palidx=cp>>8;

  uint tx = bm1.bmX;
  uint* outp = &bm1.pixel(PX,PY);

  uint* fontbuf = ch1.fontbuf1;
  uint sx=ch1.wmax;
  uint sy=ch1.hmax;

  uint*& pcache = cache_ptr[palidx][chridx];

  // If this char+palette was already rendered, copy from cache
  if( pcache!=0 ) {

    uint* __restrict p = pcache;
    uint* __restrict o = outp;
    __assume_aligned(p,4);
    __assume_aligned(o,4);
    pcache = outp;
    for( k=0; k<sy; k++ ) {
      for( i=0; i<sx; i++ ) o[i]=p[i];
      p += tx;
      o += tx;
    }

  } else {
    // First time rendering this char+palette, colorize and store location
    byte* __restrict p = (byte*)&fontbuf[chridx*sy*sx];;
    byte* __restrict o = (byte*)outp;
    pcache = outp;
    ALIGN(16) byte q[4]; 
    ALIGN(16) byte r[4];
    __assume_aligned(p,4);
    __assume_aligned(o,4);

    // Get foreground and background colors
    for( i=0; i<sizeof(q); i+=4 ) (uint&)q[i]=palette[palidx].fg, (uint&)r[i]=palette[palidx].bk;

    // Alpha-blend font bitmap with colors
    for( k=0; k<sy; k++ ) {
      for( i=0; i<sx*4; i+=sizeof(q) ) {
        for( j=0; j<3; j++ ) {
          word x = r[j]*255 + (int(q[j])-int(r[j]))*p[i+j];
          o[i+j] = (x+255)>>8;
        }
      }
      p += sx*4;
      o += tx*4;
    }

  }

}

};

//--- #include "hexdump.inc"

// Hex file viewer with caching and difference highlighting
struct hexfile {
  filehandle0 F1;
  qword F1size;  // Total file size
  qword F1pos;   // Current view position in file

  enum {
    f_vertline=1,  // Draw vertical separator line
    f_addr64       // Use 64-bit addresses
  };
  uint  flags;

  uint  BX;       // Bytes per line (actual)
  uint  F1cpl;    // Chars per line (visible, may be < BX if window too narrow)
  uint  F1dpl;    // Unused chars in line
  uint  textlen;  // Total bytes visible in view
  byte* diffbuf;  // Difference highlight flags per byte

  // File data caching - keeps a 1MB window of file data
  qword viewbeg;  // First visible byte in databuf
  qword viewend;  // Last visible byte+1 in databuf
  enum{ datalen=1<<20, datalign=1<<16 };  // 1MB buffer, 64KB aligned
  qword databeg;  // Start of cached region
  qword dataend;  // End of cached region
  byte  databuf[datalen];  // Cached file data

  // Calculate required text buffer width in characters
  uint Calc_WCX( uint mBX, uint f_addr64, uint f_vertline ) {
    uint waddr = f_addr64 ? 8+1+8 : 8;  // Address column width
    uint whex = 3*mBX;                   // Hex dump width (2 hex + 1 space per byte)
    uint wtxt = 1*mBX;                   // ASCII dump width
    uint WCX = waddr+1+1+whex+1+wtxt+(f_vertline>0);
    return WCX;
  }

  // Initialize textbuffer parameters
  void SetTextbuf( textblock& tb1, uint _BX, uint _flags=0 ) {
    flags = _flags;
    BX = _BX;

    uint aw = (flags&f_addr64)? 8+1+8 : 8;
    F1cpl = (tb1.WCX-aw-2-1)/(3+1);  // Calculate how many bytes fit per line
    F1dpl = (tb1.WCX-aw-2-1)%(3+1);  // Leftover space

    textlen = BX*tb1.WCY;  // Total bytes in view
    diffbuf = new byte[textlen];
    if( diffbuf!=0 ) bzero( diffbuf, textlen );
  }

  void Quit( void ) {
    delete diffbuf;
  }

  // Get byte at offset i from view (returns -1 if beyond EOF)
  uint viewdata( uint i ) {
    qword ofs = F1pos+i; 
    uint c, f = (ofs>=viewbeg) && (ofs<viewend);
    c = f ? databuf[F1pos-databeg+i] : -1;
    return c;
  }

  // Compare this file with another (unused, comparison now done in main loop)
  void Compare( hexfile& F2 ) {
    uint c1,c2,i;

    for( i=0; i<textlen; i++ ) {
      c1 = (*this).viewdata(i);
      c2 = F2.viewdata(i);
      diffbuf[i] = (c1!=c2);
    }
  }

  // Move view position by predefined amount
  void MovePos( uint m_type=0 ) {
    // Navigation deltas: [home, end, left, right, up, down, pgup, pgdn, wheel-up, wheel-dn]
    const int delta[] = { -1,1, -int(BX),int(BX), -int(textlen),int(textlen), -int(BX*4), int(BX*4) };
    SetFilepos( (m_type==0)? 0 : (m_type==1) ? F1size-textlen : F1pos+sqword(delta[m_type-2]) );
  }

  // Move view by relative amount
  void MoveFilepos( int newpos=0 ) {
    F1pos += sqword(newpos);
    SetFilepos( F1pos );
  }

  // Set absolute view position and update cache if needed
  void SetFilepos( qword newpos ) {
    uint len;
    F1pos=newpos;
  
    qword newend = newpos+textlen;

    // Clamp to file bounds
    if( newend>F1size ) newend=F1size;
    if( newend<newpos ) newpos=0;
 
    // Check if requested region is already cached
    if( (newpos>=databeg) && (newend<=dataend) ) {
      // All necessary data is in buffer already
    } else {
      // Need to load new data - align to 64KB boundary for better disk I/O
      databeg = newpos - (newpos % datalign);
      F1.seek( databeg );
      len = F1.read( databuf, datalen );
      dataend = databeg + len;
      if( newend>=dataend ) newend=dataend;
    }

    viewbeg = newpos;
    viewend = newend;
  }

  // Open file and get size
  size_t Open( char* fnam ) {
    F1pos=0;
    databeg = dataend=0;
    if( F1.open(fnam) ) {
      F1size = F1.size();
    }
    return ((byte*)F1.f)-((byte*)0);
  }

  // Print hex number with specified width
  word* HexPrint( word* s, qword x, uint w, uint attr ) {
    uint i,c;
    attr<<=8;
    for( i=w-1; i!=-1; i-- ) {
      c = '0'+((x>>(i*4))&15); if( c>'9' ) c+='A'-('9'+1);
      *s++ = c | attr;
    }
    return s;
  }

  // Render hex dump to textblock
  void hexdump( textblock& tb1 ) {

    uint c,i,j;
    uint cpl = F1cpl;
    uint dpl = F1dpl;
    uint len = textlen;
    byte* p  = &databuf[F1pos-databeg];

    qword ofs;

    word* s = &tb1.cell(0,0);
    word* _s = s;

    // For each line in display
    for( j=0; j<tb1.WCY; j++ ) {
      ofs = F1pos+j*BX;

      // Print address (64-bit or 32-bit)
      if( flags&f_addr64 ) {
        s = HexPrint( s, ofs>>32, 8, pal_Addr );
        *s++ = tb1.ch(':',pal_Addr);
      }
      s = HexPrint( s, ofs, 8, pal_Addr );

      *s++ = tb1.ch(':',pal_Addr);
      *s++ = tb1.ch(' ',pal_Hex);

      // Print hex bytes
      for( i=0; i<cpl; i++ ) {
        s[2]=s[1]=s[0] = tb1.ch(' ',pal_Hex);
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          HexPrint( s, p[j*BX+i], 2, diffbuf[j*BX+i]?pal_Diff:pal_Hex );
        }
        s+=3;
      }
      if( F1cpl<BX ) s[-1]=tb1.ch('>',pal_Addr);  // Indicator if line is truncated

      *s++ = tb1.ch(' ',pal_Hex);

      // Print ASCII representation
      for( i=0; i<cpl; i++ ) {
        c = tb1.ch(' ',pal_Hex);
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          c = tb1.ch( p[j*BX+i], diffbuf[j*BX+i]?pal_Diff:pal_Hex );
        }
        *s++ = tb1.ch( byte(c),pal_Hex);
      }

      for( i=0; i<dpl; i++ ) *s++=tb1.ch(' ',pal_Hex);

      // Draw vertical separator if enabled
      if( flags & f_vertline ) s[-1]=tb1.ch('|',pal_Sep);

    }

  }

};

//--- #include "window.inc"

HWND win;
HDC  dibDC;
PAINTSTRUCT ps;

// Draw animated box (dashed line with moving offset)
void DrawBox( HDC dc, int x1,int y1, int x2,int y2, int dx=0 ) {
  dx %= (x2-x1);
  POINT a[] = { { x1+dx,y1 }, { x2,y1 }, { x2,y2 }, { x1,y2 }, { x1,y1 }, { x1+dx,y1 } };
  Polyline( dc, a, 6 );
}

// Draw line between two points
void DrawLine( HDC dc, HGDIOBJ& hPen, int x1,int y1, int x2,int y2 ) {
  POINT a[] = { { x1,y1 }, { x2,y2 } };
  HGDIOBJ hPenOld = SelectObject( dibDC, hPen );
  Polyline( dc, a, 2 );
  SelectObject( dibDC, hPenOld );
}

// Request window redraw
void DisplayRedraw( void ) {
  InvalidateRect( win, 0, 0 );
}

void DisplayRedraw( int x, int y, int dx, int dy ) {
  RECT v;
  v.left   = x;
  v.top    = y;
  v.right  = x+dx;
  v.bottom = y+dy;
  InvalidateRect( win, &v, 0 );
}

// Move window by delta
void ShiftWindow( int x, int y, uint mode=3 ) {
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  GetWindowPlacement( win, &winpl );
  winpl.rcNormalPosition.left  += (mode&1) ? x : 0;
  winpl.rcNormalPosition.top   += (mode&1) ? y : 0;
  winpl.rcNormalPosition.right += (mode&2) ? x : 0;
  winpl.rcNormalPosition.bottom+= (mode&2) ? y : 0;
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}

// Set absolute window position and size
void SetWindow( HWND win, int x, int y, int w, int h ) {
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  winpl.rcNormalPosition.left  = x;
  winpl.rcNormalPosition.top   = y;
  winpl.rcNormalPosition.right = x+w;
  winpl.rcNormalPosition.bottom= x+h;
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}

// Set window size (keeping position)
void SetWindowSize( HWND win, int w, int h ) {
  int x,y;
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  GetWindowPlacement( win, &winpl );
  x = winpl.rcNormalPosition.left;
  y = winpl.rcNormalPosition.top;
  winpl.rcNormalPosition.right = x+w;
  winpl.rcNormalPosition.bottom= x+h;
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}

//--- #include "config.inc"

// View configuration (saved to registry)
struct viewstate {
  LOGFONT lf;      // Font configuration
  uint BX,BY;      // Bytes per line and lines per screen
  int  cur_view;   // Currently selected file (-1 = all files)
  uint f_addr64;   // 64-bit address display flag
  uint f_help;     // Help text visible flag
};

// Default configuration (Consolas font)
viewstate lf = { {-19,-10, 0, 0, 400, 0, 0, 0, 204, 3, 2, 1, 49, "Consolas"}, 32,255, -1, 0, 0 };
viewstate lf_old;

// Save configuration to registry
void SaveConfig( ) {
  uint r,disp;
  HKEY hk;
  r = RegCreateKeyEx( 
    HKEY_CURRENT_USER, 
    "Software\\SRC\\cmp_01", 0, 0,
    REG_OPTION_NON_VOLATILE,
    KEY_ALL_ACCESS,
    0,
    &hk, (LPDWORD)&disp
  );
  if( r==ERROR_SUCCESS ) {
    r = RegSetValueEx( hk, "config", 0, REG_BINARY, (byte*)&lf, sizeof(lf) );
    RegCloseKey( hk );
  } else {
    SetLastError(r);
    printf( "r=%X disp=%X\nError=<%s>\n", r, disp, GetErrorText() );
  }
}

// Load configuration from registry
void LoadConfig( ) {
  uint r,l=sizeof(lf);
  HKEY hk;
  r = RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\SRC\\cmp_01", 0, KEY_READ, &hk );
  if( r==ERROR_SUCCESS ) {
    r = RegQueryValueEx( hk, "config", 0, 0, (byte*)&lf, (LPDWORD)&l );
    RegCloseKey( hk );
  } else {
    SetLastError(r);
    printf( "r=%X\nConfig Error=<%s>\n", r, GetErrorText() );
  }
}

//--- #include "help.inc"

// Help text with ~ markers for highlighting
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

enum{ N_VIEWS=8 };  // Maximum number of files to compare

// Global GUI state
MSG  msg;
uint lastkey = 0;
uint curtim, lasttim = 0;

mybitmap  bm1;                // Display bitmap
myfont    ch1;                // Font renderer
hexfile   F[N_VIEWS];         // File viewers
textblock tb[N_VIEWS];        // Text buffers for each file
uint F_num;                   // Actual number of files opened

textblock tb_help;            // Help text buffer
uint help_SY;                 // Help text height in lines

uint f_started = 0;           // First-time initialization flag
volatile uint f_busy = 0;     // Background difference scan in progress flag

// Background thread for scanning to next difference
struct DiffScan : thread<DiffScan> {

  typedef thread<DiffScan> base;

  uint start( void ) {
    f_busy=1;
    return base::start();
  }

  // Thread function - scans forward until difference or EOF
  void thread( void ) {
    uint c,i,j,d,x[N_VIEWS],ff_num,delta,flag;
    qword pos_delta=0;

    // Start scanning from next screen
    for(i=0;i<F_num;i++) F[i].MoveFilepos(F[0].textlen);

    while( f_busy ) {
      delta=0;
      // Check each byte in current view
      for( j=0; j<F[0].textlen; j++ ) {
        c=0; d=-1; ff_num=0; 
        // Compare across all files
        for(i=0;i<F_num;i++) {
          x[i] = F[i].viewdata(j);
          ff_num += (x[i]==-1);  // Count EOF markers
          if( x[i]!=-1 ) c|=x[i],d&=x[i];
        }
        // Check if all files have same value
        for(flag=1,i=0;i<F_num;i++) flag &= ((c==x[i])&&(d==x[i]));
        delta += flag;
      }

      // Stop if all files at EOF
      if( ff_num>=F_num ) break;
      pos_delta += delta;
      // Stop if found difference
      if( delta<F[0].textlen ) break;
      // Continue to next screen
      for(i=0;i<F_num;i++) F[i].MoveFilepos(delta);
    }

    f_busy=0;
    DisplayRedraw();
    return;
  }

};

DiffScan diffscan;

HINSTANCE g_hInstance = GetModuleHandle(0);

// Minimal window procedure (we handle everything in message loop)
LRESULT CALLBACK wndproc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) {
  return DefWindowProc(hWnd, message, wParam, lParam);
}

WNDCLASS wndclass = {
  0,
  wndproc,
  0,0,
  g_hInstance,
  0,0,
  0,
  0,
  "wndclass"
};

// Main entry point
int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {
  int argc=__argc; char **argv=__argv;

  uint c,i,j,l;

  char* fil1 = argv[0];

  // Open files from command line
  for( i=1; i<Min(DIM(F)+1,Max(2,argc)); i++ ) {
    if( i<argc ) fil1=argv[i];
    if( F[i-1].Open(fil1)==0 ) return 1;
    // Enable 64-bit addresses if any file is >4GB
    if( F[i-1].F1size>0xFFFFFFFFU ) lf.f_addr64=hexfile::f_addr64;
  } 
  F_num = i-1;

  LoadConfig();
  tb_help.textsize( helptext, 0, &help_SY );

  // Get window frame metrics
  const int wfr_x = GetSystemMetrics(SM_CXFIXEDFRAME) + GetSystemMetrics(SM_CXBORDER);
  const int wfr_y = GetSystemMetrics(SM_CYFIXEDFRAME);
  const int wfr_c = GetSystemMetrics(SM_CYCAPTION);
  RECT scr; SystemParametersInfo( SPI_GETWORKAREA,0,&scr,0 );
  const int scr_w = scr.right-scr.left;
  const int scr_h = scr.bottom-scr.top;

  printf( "wfr_x=%i\n", wfr_x );

  // Create offscreen DC and bitmap
  dibDC = CreateCompatibleDC( GetDC(0) );
  bm1.AllocBitmap( dibDC, scr_w, scr_h );

  RegisterClass( &wndclass );

  // Create main window
  win = CreateWindow(
   "wndclass",
    "File Comparator  (F1=help)",
    WS_SYSMENU,
    scr.left,scr.top,
    scr_w,scr_h,
    0,
    0,
    GetModuleHandle(0),
    0
  );

  // Set window icon
  HICON ico = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(500));
#ifndef _WIN64
  SetClassLong( win, GCL_HICON, LONG(ico) );
#else
  SetClassLongPtr(win, GCLP_HICON, (LONG_PTR)ico);
#endif
  DeleteObject(ico);

  // Create pens for UI drawing
  LOGBRUSH lb; 
  HGDIOBJ hPen, hPenOld, hPen_help;
  lb.lbStyle = BS_SOLID; 
  lb.lbColor = RGB(0,0xFF,0);
  lb.lbHatch = HS_CROSS; 
  hPen = ExtCreatePen( PS_GEOMETRIC|PS_DASH, 5, &lb, 0, NULL ); 
  uint pen_shift = 0;

  lb.lbStyle = BS_SOLID; 
  lb.lbColor = RGB(0xFF,0xFF,0xFF);
  lb.lbHatch = HS_HORIZONTAL; 
  hPen_help = ExtCreatePen( PS_GEOMETRIC, 2, &lb, 0, NULL ); 

  int delta;
  int rp1,rp,alt,ctr;
  uint mBX,mBY;
  int  WX,WY;

  // Restart point for GUI reconfiguration
  if(0) {
    Restart:
    for(i=0;i<F_num;i++ ) {
      tb[i].Quit();
      F[i].Quit();
    }
    ch1.Quit();
  }

  // Initialize font
  memcpy( &ch1.lf, &lf.lf, sizeof(ch1.lf) );
  ch1.DumpLF();
  
  ch1.InitFont();
  ch1.SetFont(dibDC,lf.lf);

  // Calculate maximum window size that fits on screen
  mBX=0; mBY=0;
  for( j=15; j!=-1; j-- ) {
    mBX |= (1<<j);
    mBY |= (1<<j);

    // Calculate required width for all file views
    WX = 2*wfr_x;
    for(i=0;i<F_num;i++) WX += F[i].Calc_WCX( mBX, lf.f_addr64, (i!=F_num-1) ) * ch1.wmax;

    WY = mBY*ch1.hmax + lf.f_help* help_SY*ch1.hmax + 2*wfr_y+wfr_c;

    if( WX>bm1.bmX ) mBX&=~(1<<j);
    if( WY>bm1.bmY ) mBY&=~(1<<j);
  }
  mBX = Max(1U,mBX);
  mBY = Max(1U,mBY);
  mBX = Min(mBX,lf.BX);
  if( f_started==0 ) lf.BX=mBX;
  if( lf.BY>mBY ) lf.BY=mBY;
  // Ensure buffer doesn't exceed cache size
  if( lf.BX*lf.BY>hexfile::datalen-hexfile::datalign ) lf.BX=(hexfile::datalen-hexfile::datalign)/lf.BY;

  printf( "mBX=%i mBY=%i BX=%i BY=%i\n", mBX, mBY, lf.BX, lf.BY );

  // Initialize text buffers for each file view
  WX=0*wfr_x;
  for(i=0;i<F_num;i++ ) {
    uint WCX = F[i].Calc_WCX( mBX, lf.f_addr64, (i!=F_num-1) );
    tb[i].Init( ch1, WCX,lf.BY, WX,0 );
    WX += WCX*ch1.wmax;
    F[i].SetTextbuf( tb[i], lf.BX, ((i!=F_num-1)?hexfile::f_vertline:0) | lf.f_addr64);
    F[i].SetFilepos( F[i].F1pos );
  }
  WX+=2*wfr_x;

  // Initialize help text buffer if enabled
  WY = tb[0].WCY*ch1.hmax;
  if( lf.f_help ) {
    tb_help.Init( ch1, WX/ch1.wmax,help_SY, 0,WY );
    tb_help.Fill( ' ', pal_Help1 );
    tb_help.Print( helptext, pal_Help1,pal_Help2, 1,0 );
    WY += help_SY*ch1.hmax; 
  }
  WY += 2*wfr_y+wfr_c;

  printf( "WX=%i WY=%i bmX=%i bmY=%i\n", WX,WY,bm1.bmX,bm1.bmY );

  SetWindowSize( win, WX,WY );

  SetFocus( win );
  ShowWindow( win, SW_SHOW );
  SetTimer( win, 0, 100, 0 );  // Timer for animation
  DisplayRedraw();
  f_started=1;
  lf_old = lf;

  // Main message loop
  while( GetMessage(&msg,win,0,0) ) {

    if(0) { m_break: break; }

    switch( msg.message ) {

    case WM_MOUSEWHEEL:
      delta = (short)HIWORD(msg.wParam);
      delta = (delta<0) ? 9 : 8; goto MovePos;

    case WM_CLOSE: case WM_NULL:
      goto m_break;

    case WM_TIMER: 
      DisplayRedraw();
      break;

    case WM_PAINT:

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
