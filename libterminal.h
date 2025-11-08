// Terminal Emulator Library - Header
// Encapsulated terminal component with blinking cursor and line editing
#ifndef LIBTERMINAL_H
#define LIBTERMINAL_H

#define NOMINMAX
#include <windows.h>

// CMP library includes
#include "common.h"
#include "bitmap.h"
#include "setfont.h"
#include "textblock.h"
#include "textprint.h"
#include "palette.h"

//--- Terminal state and encapsulation
struct Terminal {

  RECT    area;
  int     cols;
  int     rows;

  int     cursor_blink;
  DWORD   last_blink;
  DWORD   blink_ms;            // moved from static const to field

  // dynamic line storage: contiguous block of (lines_capacity + 1) * line_width bytes
  char*   lines_data;
  char*   current_line;        // points into lines_data (the extra row)
  char*   buf_local;
  char*   vis_buf;

  int     lines_capacity;      // number of stored lines
  int     line_width;          // chars per line (including terminating NUL)
  int     line_count;
  int     scroll_pos;
  int     cursor_pos;
  int     hscroll_pos;
  int     max_lines;

  // text colors
  uint    fg_color;
  uint    bg_color;

  mybitmap  bm;          // CMP bitmap for rendering
  textblock tb;          // CMP textblock for character grid
  HDC       dibDC;
  myfont*   font;        // CMP font

  // Public methods
  void Init( myfont& fnt, const RECT& rc );
  void Quit();

  // Helper to access stored line i
  char* LineAt(int i);

  // Content operations
  void AddLine(const char* text);
  void UpdateHScroll();
  void InsertChar(char ch);
  void Backspace();
  void Delete();
  void MoveCursorLeft();
  void MoveCursorRight();
  void MoveCursorHome();
  void MoveCursorEnd();
  void ScrollUp();
  void ScrollDown();
  void EnterLine();

  // Message handling
  uint HandleMessage(const MSG& msg, HWND hwnd);

  // Rendering
  void RenderToWindow(HDC wndDC);
  void DrawCursor( mybitmap& bm, int x, int y, int w, int h, int show );
  void RenderTerminal( mybitmap& bm, HDC dc, myfont& font, int cursor_blink );
};

#endif // LIBTERMINAL_H
