// Color palette implementation
#include "palette.h"

// Color palette definition (RGB values: 0x00BBGGRR)
color palette[pal_MAX] = {
  { 0xFFFFFF, 0x000000 },  // pal_Sep: white on black
  { 0xFFFF55, 0x000000 },  // pal_Hex: light yellow on black
  { 0x00AA00, 0x000000 },  // pal_Addr: green on black
  { 0xFFFFFF, 0x0000AA },  // pal_Diff: white on dark blue

  { 0x00FFFF, 0x000080 },  // pal_Help1: cyan on navy
  { 0xFFFFFF, 0x000080 },  // pal_Help2: white on navy
};
