// Character rendering with caching
#ifndef TEXTPRINT_H
#define TEXTPRINT_H

#include "common.h"
#include "palette.h"
#include "setfont.h"
#include "bitmap.h"

// Character rendering cache - stores pointers to last rendered positions
// This is a key optimization: if we already rendered char X with palette Y, just copy from previous location
extern uint* cache_ptr[pal_MAX][256];  // [palette index][character] -> pointer to previously rendered pixels

// Clear the character rendering cache (called before each frame)
void SymbOut_Init( void );

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

#endif // TEXTPRINT_H
