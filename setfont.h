// Font management and pre-rendering
#ifndef SETFONT_H
#define SETFONT_H

#include "common.h"
#include "bitmap.h"
#include <windows.h>

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
  void InitFont( void );

  // Cleanup font resources
  void Quit( void );

  // Debug: print LOGFONT structure to console
  void DumpLF();

  // Show font selection dialog to user
  void SelectFont( HWND hwndOwner = 0 );

  // Measure character widths and heights for all 256 characters
  void GetFontWidth( HDC dibDC );

  // Set font and pre-render all characters (called when font changes)
  void SetFont( HDC dibDC, LOGFONT& _lf );

  // Pre-render all 256 characters to bitmap buffer for fast blitting
  // This is a critical optimization: render each character once, then copy pixels during display
  void PrecalcFont( HDC dibDC, byte* bitmap, uint bmX );

};

#endif // SETFONT_H
