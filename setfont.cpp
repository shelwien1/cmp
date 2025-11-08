// Font management implementation
#include "setfont.h"

// Initialize font structure (called before SetFont)
void myfont::InitFont( void ) {
  hFont=0;           // No font selected yet
  bzero(lf);         // Zero out LOGFONT structure
}

// Cleanup font resources
void myfont::Quit( void ) {
  delete fontbuf1;   // Free pre-rendered character buffer
  DeleteObject( hFont ); hFont=0;  // Delete GDI font object
}

// Debug: print LOGFONT structure to console
void myfont::DumpLF() {
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
void myfont::SelectFont( HWND hwndOwner ) {
  // Temporarily reset packing to Windows default for CHOOSEFONT structure
  // (common.h sets pack(1) but Windows structures need default alignment)
#pragma pack(push, 8)
  CHOOSEFONT cf;
#pragma pack(pop)

  bzero(cf);
  cf.lStructSize = sizeof(CHOOSEFONT);  // Must use sizeof(CHOOSEFONT), not sizeof(cf)
  cf.hwndOwner = hwndOwner;  // Set parent window for proper modal behavior
  cf.lpLogFont = &lf;  // Edit this LOGFONT structure
  // Flags: screen fonts only, fixed-pitch only (for hex display), no vertical fonts
  cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_FIXEDPITCHONLY | CF_NOVERTFONTS;
  printf( "SelectFont: hwndOwner=%p, sizeof(cf)=%zu, sizeof(CHOOSEFONT)=%zu\n", hwndOwner, sizeof(cf), sizeof(CHOOSEFONT) );
  BOOL result = ChooseFont(&cf);
  printf( "SelectFont: ChooseFont returned %d, CommDlgExtendedError=%X\n", result, CommDlgExtendedError() );
  if( result ) DumpLF();  // If user clicked OK, print the selection
}

// Measure character widths and heights for all 256 characters
void myfont::GetFontWidth( HDC dibDC ) {
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
void myfont::SetFont( HDC dibDC, LOGFONT& _lf ) {
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
void myfont::PrecalcFont( HDC dibDC, byte* bitmap, uint bmX ) {
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
    RECT rect = { 0,0,int(wmax),int(hmax) };  // Rectangle covering character cell

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
