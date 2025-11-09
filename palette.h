// Color palette definitions
#ifndef PALETTE_H
#define PALETTE_H

#include "common.h"

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

  pal_GRAY_START = 6,  // Start of grayscale region (indices 6-255)
  pal_MAX = 256        // Total palette entries (6 UI colors + 250 grayscale)
};

// Color palette definition (RGB values: 0x00BBGGRR)
extern color palette[pal_MAX];

#endif // PALETTE_H
