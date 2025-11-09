// Terminal Emulator Library - Implementation
// Encapsulated terminal component with blinking cursor and line editing

#include "libterminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--- simple helper to avoid STL and Windows min/max macro conflicts
template<typename T, typename U>
inline T my_min(T a, U b) {
    return (a < b) ? a : b;
}

void Terminal::Init( myfont& fnt, const RECT& rc, int line_cap, int line_w ) {

  // runtime-configurable text buffer sizes
  lines_capacity = line_cap;  // number of historical lines kept
  line_width     = line_w;    // width of each line buffer including terminator

  // cursor blink interval
  blink_ms = 500;

  // default colors
  fg_color = 0xFFFFFF;
  bg_color = 0x000000;

  // no command handler by default
  command_handler = 0;

  area = rc;
  font = &fnt;
  dibDC = CreateCompatibleDC(0);

  cols = (area.right - area.left) / font->wmax;
  rows = (area.bottom - area.top) / font->hmax;
  if( cols<1 ) cols = 1;
  if( rows<2 ) rows = 2;
  max_lines = rows-1;

  // allocate contiguous storage for lines + one working current_line row
  lines_data = (char*)malloc( (lines_capacity+3) * line_width );
  if(!lines_data) {
    lines_capacity = 0;
    line_width = 0;
    current_line = 0;
    buf_local = 0;
    vis_buf = 0;
  } else {
    memset(lines_data, 0, (lines_capacity+3) * line_width);
    current_line = lines_data + (lines_capacity+0) * line_width; // last row reserved for editing
    buf_local = lines_data + (lines_capacity+1) * line_width;
    vis_buf = lines_data + (lines_capacity+2) * line_width;
  }

  line_count = 0;
  scroll_pos = 0;
  cursor_pos = 0;
  hscroll_pos = 0;

  cursor_blink = 1;
  last_blink = GetTickCount();

  int dib_w = cols * font->wmax;
  int dib_h = rows * font->hmax;

  // Initialize CMP bitmap and textblock
  bm.AllocBitmap(dibDC, dib_w, dib_h);
  tb.Init(*font, cols, rows, 0, 0);
}

void Terminal::Resize( const RECT& rc ) {
  // Update area and recalculate rows/cols based on new dimensions
  area = rc;

  int new_cols = (area.right - area.left) / font->wmax;
  int new_rows = (area.bottom - area.top) / font->hmax;
  if( new_cols<1 ) new_cols = 1;
  if( new_rows<2 ) new_rows = 2;

  // Only reallocate if dimensions changed
  if( new_cols != cols || new_rows != rows ) {
    cols = new_cols;
    rows = new_rows;
    max_lines = rows-1;

    // Reallocate bitmap and textblock for new size
    int dib_w = cols * font->wmax;
    int dib_h = rows * font->hmax;

    // Free old bitmap
    if( bm.dib ) DeleteObject(bm.dib);

    // Allocate new bitmap
    bm.AllocBitmap(dibDC, dib_w, dib_h);

    // Reinitialize textblock
    tb.Quit();
    tb.Init(*font, cols, rows, 0, 0);

    // Adjust scroll position if needed
    if( line_count > max_lines ) {
      scroll_pos = line_count - max_lines;
    } else {
      scroll_pos = 0;
    }

    // Adjust horizontal scroll
    UpdateHScroll();
  }
}

void Terminal::Quit() {
  tb.Quit();
  if( bm.dib ) { DeleteObject(bm.dib); bm.dib=0; }
  if( dibDC ) { DeleteDC(dibDC); dibDC=0; }
  if(lines_data) { free(lines_data); lines_data=0; current_line=0; }
}

// helper to access stored line i
char* Terminal::LineAt(int i) {
  return lines_data + (size_t)i * line_width;
}

// content ops (updated to use runtime sizes)
void Terminal::AddLine(const char* text) {
  // Handle newlines in text by splitting into multiple lines
  const char* start = text;
  const char* newline;

  while( (newline = strchr(start, '\n')) != NULL ) {
    // Copy up to the newline
    int len = (int)(newline - start);
    if( len > line_width - 1 ) len = line_width - 1;

    if( line_count < lines_capacity ) {
      strncpy(LineAt(line_count), start, (size_t)len);
      LineAt(line_count)[len] = 0;
      line_count++;
      if( line_count > max_lines ) {
        scroll_pos = line_count - max_lines;
      }
    } else {
      // simple rollover: drop the oldest line, shift memory
      memmove(lines_data, lines_data + line_width, (size_t)(lines_capacity - 1) * line_width);
      strncpy(LineAt(lines_capacity - 1), start, (size_t)len);
      LineAt(lines_capacity - 1)[len] = 0;
      if (line_count < lines_capacity) line_count++; else line_count = lines_capacity;
      if( line_count > max_lines ) scroll_pos = line_count - max_lines;
    }

    start = newline + 1;
  }

  // Add the remaining text (or the entire text if no newlines)
  if( line_count < lines_capacity ) {
    strncpy(LineAt(line_count), start, (size_t)line_width - 1);
    LineAt(line_count)[line_width - 1] = 0;
    line_count++;
    if( line_count > max_lines ) {
      scroll_pos = line_count - max_lines;
    }
  } else {
    // simple rollover: drop the oldest line, shift memory
    memmove(lines_data, lines_data + line_width, (size_t)(lines_capacity - 1) * line_width);
    strncpy(LineAt(lines_capacity - 1), start, (size_t)line_width - 1);
    LineAt(lines_capacity - 1)[line_width - 1] = 0;
    if (line_count < lines_capacity) line_count++; else line_count = lines_capacity;
    if( line_count > max_lines ) scroll_pos = line_count - max_lines;
  }
}

void Terminal::UpdateLastLine(const char* text) {
  if( !text || line_count == 0 ) return;

  // Get pointer to last line and update it
  int last_idx = line_count - 1;
  strncpy(LineAt(last_idx), text, (size_t)line_width - 1);
  LineAt(last_idx)[line_width - 1] = 0;
}

void Terminal::UpdateHScroll() {
  int vis_cursor = 1 + cursor_pos;
  if( vis_cursor < hscroll_pos ) {
    hscroll_pos = vis_cursor - 1;
    if( hscroll_pos < 0 ) hscroll_pos = 0;
  }
  if( vis_cursor >= hscroll_pos + cols ) {
    hscroll_pos = vis_cursor - cols + 1;
  }
  if( hscroll_pos < 0 ) hscroll_pos = 0;
}

void Terminal::InsertChar(char ch) {
  if(!current_line) return;
  int len = (int)strlen(current_line);
  if( (len < line_width - 2) && (cursor_pos <= len) ) { // line_width-2 == previous 254
    for(int i=len; i>=cursor_pos; i--) current_line[i+1] = current_line[i];
    current_line[cursor_pos] = ch;
    cursor_pos++;
    UpdateHScroll();
  }
}

void Terminal::Backspace() {
  if( cursor_pos > 0 ) {
    int len = (int)strlen(current_line);
    for(int i=cursor_pos-1; i<len; i++) current_line[i] = current_line[i+1];
    cursor_pos--;
    UpdateHScroll();
  }
}

void Terminal::Delete() {
  int len = (int)strlen(current_line);
  if( cursor_pos < len ) {
    for(int i=cursor_pos; i<len; i++) current_line[i] = current_line[i+1];
  }
}

void Terminal::MoveCursorLeft() {
  if(cursor_pos>0){ cursor_pos--; UpdateHScroll(); }
}

void Terminal::MoveCursorRight() {
  int len=(int)strlen(current_line);
  if(cursor_pos<len){ cursor_pos++; UpdateHScroll(); }
}

void Terminal::MoveCursorHome() {
  cursor_pos=0; UpdateHScroll();
}

void Terminal::MoveCursorEnd() {
  cursor_pos=(int)strlen(current_line); UpdateHScroll();
}

void Terminal::ScrollUp() {
  if(scroll_pos>0) scroll_pos--;
}

void Terminal::ScrollDown() {
  int maxScroll=line_count-max_lines;
  if(maxScroll<0) maxScroll=0;
  if(scroll_pos<maxScroll) scroll_pos++;
}

void Terminal::EnterLine() {
  if(!current_line) return;

  // prepare with prompt
  int tocopy = (int)strlen(current_line);
  if(tocopy > line_width - 2) tocopy = line_width - 2;
  strncpy(buf_local+1, current_line, (size_t)tocopy);
  buf_local[0] = '>';
  buf_local[1+tocopy] = 0;
  AddLine(buf_local);

  // Handle built-in commands
  bool handled = false;
  if( current_line[0] != 0 ) {
    // Trim leading spaces
    const char* cmd = current_line;
    while( *cmd == ' ' ) cmd++;

    // Check for help commands
    if( strcmp(cmd, "h") == 0 || strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0 ) {
      AddLine("Available commands:\n"
              "  g <address>        - Go to file position (hex: 0x123, decimal: 123, or EOF)\n"
              "  g <N>,<address>    - Go to address in specific file N\n"
              "  l, list            - List all open files with full paths\n"
              "  h <NN>             - Set terminal height to NN rows (2-100)\n"
              "  q, quit, exit      - Quit the application\n"
              "  help, ?            - Show this help");
      handled = true;
    }

    // Try external command handler
    if( !handled && command_handler ) {
      handled = command_handler(this, cmd);
    }

    // If still not handled, show unknown command message
    if( !handled && *cmd != 0 ) {
      AddLine("Unknown command. Type 'h' for help.");
    }
  }

  printf("%s\n", current_line); fflush(stdout);

  // clear current_line
  memset(current_line,0,(size_t)line_width);
  cursor_pos=0; hscroll_pos=0;
}

uint Terminal::HandleMessage(const MSG& msg, HWND hwnd) {
  uint handled=0;
  switch(msg.message) {
    case WM_KEYDOWN:
      handled=1;
      switch(msg.wParam) {
        case VK_RETURN: EnterLine(); break;
        case VK_BACK:   Backspace(); break;
        case VK_DELETE: Delete(); break;
        case VK_LEFT:   MoveCursorLeft(); break;
        case VK_RIGHT:  MoveCursorRight(); break;
        case VK_HOME:   MoveCursorHome(); break;
        case VK_END:    MoveCursorEnd(); break;
        case VK_UP:     ScrollUp(); break;
        case VK_DOWN:   ScrollDown(); break;
        case VK_INSERT:
          // Shift-Insert: paste from clipboard
          if( GetKeyState(VK_SHIFT) & 0x8000 ) {
            if( OpenClipboard(hwnd) ) {
              HANDLE hData = GetClipboardData(CF_TEXT);
              if( hData ) {
                char* pszText = (char*)GlobalLock(hData);
                if( pszText ) {
                  // Insert text at cursor position
                  int len = (int)strlen(pszText);
                  for(int i = 0; i < len; i++) {
                    char ch = pszText[i];
                    // Skip newlines and other control characters
                    if( ch >= 32 && ch < 127 ) {
                      InsertChar(ch);
                    }
                  }
                  GlobalUnlock(hData);
                }
              }
              CloseClipboard();
            }
          } else {
            handled = 0;
          }
          break;
        default: handled=0; break;
      }
      if(handled) InvalidateRect(hwnd,&area,FALSE);
      break;
    case WM_CHAR:
      if((msg.wParam>=32)&&(msg.wParam<256)) {
        InsertChar((char)msg.wParam);
        InvalidateRect(hwnd,&area,FALSE);
        handled=1;
      }
      break;
  }
  // steady blink timing (uses instance blink_ms)
  DWORD now=GetTickCount();
  if((now-last_blink) >= blink_ms) {
    last_blink = now;
    cursor_blink ^= 1;
    InvalidateRect(hwnd,&area,FALSE);
  }

  return handled;
}

void Terminal::RenderToWindow(HDC wndDC) {
  RenderTerminal(bm, dibDC, *font, cursor_blink);
  BitBlt(wndDC, area.left, area.top, bm.bmX, bm.bmY, dibDC, 0, 0, SRCCOPY);
}

//--- Rendering functions (using CMP textblock/SymbOut)

void Terminal::DrawCursor( mybitmap& bm, int x, int y, int w, int h, int show ) {
  if(show==0) return;
  int cursor_h=h/4; if(cursor_h<2) cursor_h=2;
  if(x<0||y<0) return;
  if(x+w>bm.bmX) w=bm.bmX-x;
  if(y+h>bm.bmY) h=bm.bmY-y;
  if(w<=0||h<=0) return;
  for(int j=0;j<cursor_h;j++) {
    byte* p = bm.bitmap + (y+h-cursor_h+j)*bm.bmX*4 + x*4;
    for(int i=0;i<w;i++) {
      p[0]=192; p[1]=192; p[2]=192; p+=4;
    }
  }
}

void Terminal::RenderTerminal( mybitmap& bm, HDC dc, myfont& font, int cursor_blink ) {
  // Clear bitmap and textblock
  bm.Reset();
  tb.Clear();

  // Render historical lines to textblock
  int vis_start=scroll_pos;
  int vis_end=scroll_pos+max_lines;
  if(vis_end>line_count) vis_end=line_count;

  for(int i=vis_start;i<vis_end;i++) {
    int row = i - vis_start;
    const char* line = LineAt(i);
    int len = (int)strlen(line);
    // Copy line to textblock with attribute 1 (normal text color)
    for(int j=0; j<len && j<cols; j++) {
      tb.cell(j, row) = tb.ch(line[j], 1);
    }
  }

  if(!current_line) return;

  // Build prompt+current_line into a temporary buffer
  buf_local[0] = '>';
  strncpy(buf_local+1, current_line, (size_t)line_width - 2);
  buf_local[1 + (line_width - 2)] = 0;
  int buf_len=(int)strlen(buf_local);

  int start_char=hscroll_pos;
  int end_char=hscroll_pos+cols;
  if(start_char<0) start_char=0;
  if(end_char>buf_len) end_char=buf_len;
  if(start_char>buf_len) start_char=buf_len;

  // Render current input line to textblock
  int input_row = vis_end - vis_start;
  for(int i=start_char; i<end_char; i++) {
    int col = i - start_char;
    if(col < cols && input_row < rows) {
      tb.cell(col, input_row) = tb.ch(buf_local[i], 1);
    }
  }

  // Render textblock to bitmap using CMP
  tb.Print(font, bm);

  // Draw cursor
  if(cursor_blink) {
    int vis_cursor_pos=(1+cursor_pos)-hscroll_pos;
    if(vis_cursor_pos>=0 && vis_cursor_pos<cols) {
      int cursor_x=font.wmax*vis_cursor_pos;
      int cursor_y=input_row*font.hmax;
      DrawCursor(bm,cursor_x,cursor_y,font.wmax,font.hmax,1);
    }
  }
}
