// Bitmap allocation and management
#ifndef BITMAP_H
#define BITMAP_H

#include "common.h"
#include <windows.h>

// Allocate a DIB (Device Independent Bitmap) section for drawing
// DIB allows direct pixel access (unlike DDB which is device-dependent)
inline HBITMAP AllocBitmap( HDC dibDC, byte*& bitmap, uint bmX, uint bmY ) {
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

#endif // BITMAP_H
