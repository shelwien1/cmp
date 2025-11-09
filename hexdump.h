// Hex file viewer with caching and difference highlighting
#ifndef HEXDUMP_H
#define HEXDUMP_H

#include "common.h"
#include "file_win.h"
#include "textblock.h"

// Hex file viewer with caching and difference highlighting
// Displays hex dump and ASCII view of a file, with intelligent caching for large files
struct hexfile {
  filehandle0 F1;  // File handle
  qword F1size;    // Total file size in bytes
  qword F1pos;     // Current view position in file (top-left byte being displayed)

  // Display flags
  enum {
    f_vertline=1,  // Draw vertical separator line (between file panels)
    f_addr64       // Use 64-bit address display (vs 32-bit)
  };
  uint  flags;     // Combination of above flags

  // Display modes
  enum DisplayMode {
    MODE_COMBINED = 0,  // Hex + text (default)
    MODE_HEX_ONLY = 1,  // Hex only
    MODE_TEXT_ONLY = 2  // Text only
  };
  uint  display_mode;  // Current display mode

  uint  BX;       // Bytes per line (configured - actual data width)
  uint  F1cpl;    // Chars per line (visible - may be < BX if window too narrow)
  uint  F1dpl;    // Unused chars in line (horizontal slack space)
  uint  textlen;  // Total bytes visible in current view (BX * number_of_rows)
  byte* diffbuf;  // Difference highlight flags per byte (1=different, 0=same)

  // File data caching - keeps a 1MB sliding window of file data
  // This allows viewing multi-GB files without loading everything into RAM
  qword viewbeg;  // First visible byte position in file
  qword viewend;  // Last visible byte position+1 in file
  enum{ datalen=1<<20, datalign=1<<16 };  // 1MB cache buffer, 64KB alignment for I/O
  qword databeg;  // Start of cached region in file
  qword dataend;  // End of cached region in file
  byte  databuf[datalen];  // Cached file data (1MB sliding window)

  // Calculate required text buffer width in characters for hex display
  uint Calc_WCX( uint mBX, uint f_addr64, uint f_vertline, uint mode );

  // Initialize textbuffer parameters (called during setup/resize)
  void SetTextbuf( textblock& tb1, uint _BX, uint _flags=0, uint _display_mode=MODE_COMBINED );

  // Cleanup resources
  void Quit( void );

  // Get byte at offset i from current view (returns -1 if beyond EOF)
  uint viewdata( uint i );

  // Compare this file with another (unused - comparison now done in main loop)
  void Compare( hexfile& F2 );

  // Move view position by predefined navigation type
  void MovePos( uint m_type=0 );

  // Move view by relative byte offset
  void MoveFilepos( int newpos=0 );

  // Set absolute view position and update cache if needed (intelligent cache management)
  void SetFilepos( qword newpos );

  // Open file and get size
  size_t Open( char* fnam );

  // Print hex number with specified width to textblock buffer
  word* HexPrint( word* s, qword x, uint w, uint attr );

  // Render hex dump to textblock (generates the hex view display)
  void hexdump( textblock& tb1 );

};

#endif // HEXDUMP_H
