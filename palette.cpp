// Color palette implementation
#include "palette.h"

// Helper function to initialize grayscale palette entries
static void init_grayscale_palette() {
  // Fill indices 6-255 with equally distributed grayscale colors
  for (int i = pal_GRAY_START; i < pal_MAX; i++) {
    // Calculate grayscale value: map index range [6, 255] to gray range [0, 255]
    int gray_val = ((i - pal_GRAY_START) * 255) / (pal_MAX - pal_GRAY_START - 1);
    uint gray = (gray_val << 16) | (gray_val << 8) | gray_val;  // 0x00BBGGRR format

    palette[i].fg = 0xFFFFFF;  // White foreground (for potential text display)
    palette[i].bk = gray;      // Grayscale background
  }
}

// Color palette definition (RGB values: 0x00BBGGRR)
color palette[pal_MAX] = {
  { 0xFFFFFF, 0x000000 },  // pal_Sep: white on black
  { 0xFFFF55, 0x000000 },  // pal_Hex: light yellow on black
  { 0x00AA00, 0x000000 },  // pal_Addr: green on black
  { 0xFFFFFF, 0x0000AA },  // pal_Diff: white on dark blue

  { 0x00FFFF, 0x000080 },  // pal_Help1: cyan on navy
  { 0xFFFFFF, 0x000080 },  // pal_Help2: white on navy

  // Remaining entries (6-255) are initialized by init_grayscale_palette()
};

// Static initializer to fill grayscale palette on startup
static struct PaletteInitializer {
  PaletteInitializer() { init_grayscale_palette(); }
} palette_init;
