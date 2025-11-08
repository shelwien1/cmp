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

// Include all module headers
#include "common.h"
#include "file_win.h"
#include "thread.h"
#include "bitmap.h"
#include "setfont.h"
#include "palette.h"
#include "textblock.h"
#include "textprint.h"
#include "hexdump.h"
#include "window.h"
#include "config.h"

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
            ch1.SelectFont(win);
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
