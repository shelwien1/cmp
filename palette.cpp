// Color palette implementation
#include "palette.h"

// Compile-time constexpr function to compute palette color for given index
constexpr color compute_palette_color(int i) {
  // UI color entries (0-5)
  if (i == pal_Sep)   return {0xFFFFFF, 0x000000};  // white on black
  if (i == pal_Hex)   return {0xFFFF55, 0x000000};  // light yellow on black
  if (i == pal_Addr)  return {0x00AA00, 0x000000};  // green on black
  if (i == pal_Diff)  return {0xFFFFFF, 0x0000AA};  // white on dark blue
  if (i == pal_Help1) return {0x00FFFF, 0x000080};  // cyan on navy
  if (i == pal_Help2) return {0xFFFFFF, 0x000080};  // white on navy

  // Grayscale entries (6-255): map index to grayscale with 0x0000AA foreground
  if (i >= pal_GRAY_START && i < pal_MAX) {
    int gray_val = ((i - pal_GRAY_START) * 255) / (pal_MAX - pal_GRAY_START - 1);
    uint gray = (gray_val << 16) | (gray_val << 8) | gray_val;  // 0x00BBGGRR format
    return {0x0000AA, gray};  // Dark blue foreground, grayscale background
  }

  return {0, 0};  // Fallback (should not be reached)
}

// Helper function to initialize grayscale palette entries at runtime
// (constexpr array initialization requires C++14/17 features, so we use runtime init)
static void init_palette() {
  for (int i = pal_GRAY_START; i < pal_MAX; i++) {
    palette[i] = compute_palette_color(i);
  }
}

// Color palette definition (RGB values: 0x00BBGGRR)
color palette[pal_MAX] = {
  compute_palette_color(pal_Sep),    // white on black
  compute_palette_color(pal_Hex),    // light yellow on black
  compute_palette_color(pal_Addr),   // green on black
  compute_palette_color(pal_Diff),   // white on dark blue
  compute_palette_color(pal_Help1),  // cyan on navy
  compute_palette_color(pal_Help2),  // white on navy

  // Remaining entries (6-255) are initialized by init_palette()
};

// Static initializer to fill grayscale palette on startup using constexpr function
static struct PaletteInitializer {
  PaletteInitializer() { init_palette(); }
} palette_init;
