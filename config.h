// Configuration save/load
#ifndef CONFIG_H
#define CONFIG_H

#include "common.h"
#include <windows.h>

// View configuration (saved to registry for persistence across sessions)
struct viewstate {
  LOGFONT lf;      // Font configuration (face, size, weight, etc.)
  uint BX,BY;      // Bytes per line and lines per screen (grid dimensions)
  int  cur_view;   // Currently selected file index (-1 = all files move together)
  uint f_addr64;   // 64-bit address display flag (vs 32-bit)
  uint f_help;     // Help text visible flag (F1 toggles)
  uint f_terminal; // Terminal visible flag (F5 toggles)
  uint display_mode; // Display mode: 0=combined, 1=hex-only, 2=text-only (F2 toggles)
};

// Default configuration: Consolas font, 32 bytes/line, no selection, 32-bit addresses, no help
extern viewstate lf;
extern viewstate lf_old;  // Backup of old config (to detect changes)

// Save configuration to registry (HKCU\Software\SRC\cmp_01\config)
void SaveConfig( void );

// Load configuration from registry
void LoadConfig( void );

#endif // CONFIG_H
