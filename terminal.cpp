// Terminal Emulator - Encapsulated terminal component with blinking cursor and line editing

#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CMP library includes
#include "common.h"
#include "bitmap.h"
#include "setfont.h"
#include "textblock.h"
#include "textprint.h"
#include "palette.h"

#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")

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

  void Init( myfont& fnt, const RECT& rc ) {

    // runtime-configurable text buffer sizes
    lines_capacity = 1000;      // number of historical lines kept
    line_width     = 256;       // width of each line buffer including terminator

    // cursor blink interval
    blink_ms = 500; 

    // default colors
    fg_color = 0xFFFFFF;
    bg_color = 0x000000;

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

  void Quit() {
    tb.Quit();
    if( bm.dib ) { DeleteObject(bm.dib); bm.dib=0; }
    if( dibDC ) { DeleteDC(dibDC); dibDC=0; }
    if(lines_data) { free(lines_data); lines_data=0; current_line=0; }
  }

  // helper to access stored line i
  char* LineAt(int i) { return lines_data + (size_t)i * line_width; }

  // content ops (updated to use runtime sizes)
  void AddLine(const char* text) {
    if( line_count < lines_capacity ) {
      strncpy(LineAt(line_count), text, (size_t)line_width - 1);
      LineAt(line_count)[line_width - 1] = 0;
      line_count++;
      if( line_count > max_lines ) {
        scroll_pos = line_count - max_lines;
      }
    } else {
      // simple rollover: drop the oldest line, shift memory
      memmove(lines_data, lines_data + line_width, (size_t)(lines_capacity - 1) * line_width);
      strncpy(LineAt(lines_capacity - 1), text, (size_t)line_width - 1);
      LineAt(lines_capacity - 1)[line_width - 1] = 0;
      if (line_count < lines_capacity) line_count++; else line_count = lines_capacity;
      if( line_count > max_lines ) scroll_pos = line_count - max_lines;
    }
  }

  void UpdateHScroll() {
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

  void InsertChar(char ch) {
    if(!current_line) return;
    int len = (int)strlen(current_line);
    if( (len < line_width - 2) && (cursor_pos <= len) ) { // line_width-2 == previous 254
      for(int i=len; i>=cursor_pos; i--) current_line[i+1] = current_line[i];
      current_line[cursor_pos] = ch;
      cursor_pos++;
      UpdateHScroll();
    }
  }

  void Backspace() {
    if( cursor_pos > 0 ) {
      int len = (int)strlen(current_line);
      for(int i=cursor_pos-1; i<len; i++) current_line[i] = current_line[i+1];
      cursor_pos--;
      UpdateHScroll();
    }
  }

  void Delete() {
    int len = (int)strlen(current_line);
    if( cursor_pos < len ) {
      for(int i=cursor_pos; i<len; i++) current_line[i] = current_line[i+1];
    }
  }

  void MoveCursorLeft() { if(cursor_pos>0){ cursor_pos--; UpdateHScroll(); } }
  void MoveCursorRight() { int len=(int)strlen(current_line); if(cursor_pos<len){ cursor_pos++; UpdateHScroll(); } }
  void MoveCursorHome() { cursor_pos=0; UpdateHScroll(); }
  void MoveCursorEnd() { cursor_pos=(int)strlen(current_line); UpdateHScroll(); }
  void ScrollUp() { if(scroll_pos>0) scroll_pos--; }
  void ScrollDown() { int maxScroll=line_count-max_lines; if(maxScroll<0) maxScroll=0; if(scroll_pos<maxScroll) scroll_pos++; }

  void EnterLine() {
    if(!current_line) return;
    //char buf_local[512];
    // prepare with prompt
    int tocopy = (int)strlen(current_line);
    if(tocopy > line_width - 2) tocopy = line_width - 2;
    strncpy(buf_local+1, current_line, (size_t)tocopy);
    buf_local[0] = '>';
    buf_local[1+tocopy] = 0;
    AddLine(buf_local);
    printf("%s\n", current_line); fflush(stdout);
    // clear current_line
    memset(current_line,0,(size_t)line_width);
    cursor_pos=0; hscroll_pos=0;
  }

  uint HandleMessage(const MSG& msg, HWND hwnd) {
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

  void RenderToWindow(HDC wndDC) {
    RenderTerminal(bm, dibDC, *font, cursor_blink);
    BitBlt(wndDC, area.left, area.top, bm.bmX, bm.bmY, dibDC, 0, 0, SRCCOPY);
  }

  //--- Rendering functions (using CMP textblock/SymbOut)

  void DrawCursor( mybitmap& bm, int x, int y, int w, int h, int show ) {
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

  void RenderTerminal( mybitmap& bm, HDC dc, myfont& font, int cursor_blink ) {
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

};

//--- simple helper to avoid STL and Windows min/max macro conflicts
template<typename T, typename U>
inline T my_min(T a, U b) {
    return (a < b) ? a : b;
}

//--- Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  return DefWindowProcA(hwnd,msg,wp,lp);
}

//--- Main
int main(int argc,char**argv) {
  AllocConsole(); freopen("CONOUT$","w",stdout);
  SetConsoleOutputCP(1251);

  // Use palette[1] (pal_Hex) for terminal text: light yellow on black
  // Palette is already defined in palette.cpp

  WNDCLASSA wc;
  memset(&wc,0,sizeof(wc));
  wc.lpfnWndProc=WndProc; wc.hInstance=GetModuleHandle(0);
  wc.hCursor=LoadCursor(0,IDC_ARROW);
  wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszClassName="TermClass"; RegisterClassA(&wc);

  // Initialize CMP font
  myfont font;
  font.InitFont();

  HDC screenDC = CreateCompatibleDC(0);
  LOGFONT lf;
  memset(&lf,0,sizeof(lf));
  lf.lfHeight = -16;
  lf.lfWidth = 0;
  lf.lfWeight = FW_NORMAL;
  lf.lfCharSet = DEFAULT_CHARSET;
  lf.lfQuality = NONANTIALIASED_QUALITY;
  lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
  strncpy(lf.lfFaceName, "Courier New", sizeof(lf.lfFaceName)-1);

  font.SetFont(screenDC, lf);

  int cols=80, rows=25;
  int area_w=font.wmax*cols, area_h=font.hmax*rows;

  HWND win;
  win=CreateWindowA("TermClass","Terminal",WS_OVERLAPPEDWINDOW,100,100,area_w+16,area_h+39,0,0,GetModuleHandle(0),0);
  ShowWindow(win,SW_SHOW); SetFocus(win); InvalidateRect(win,0,0);
  RECT client; GetClientRect(win,&client);

  RECT termArea; 
  termArea.left=0; termArea.top=0;
  termArea.right=my_min(client.right,area_w);
  termArea.bottom=my_min(client.bottom,area_h);

  Terminal term;
  term.Init(font,termArea);

  MSG msg; PAINTSTRUCT ps;
  BOOL running=TRUE;
  while(running) {

    while( PeekMessageA(&msg,0,0,0,PM_REMOVE) ) {

      if( term.HandleMessage(msg,win)==0 )
      switch(msg.message) {
        case WM_DESTROY: PostQuitMessage(0); break;
        case WM_CLOSE: DestroyWindow(win); break;
        case WM_QUIT:  running=FALSE; break;
        case WM_KEYDOWN:
          if(msg.wParam==VK_ESCAPE) PostQuitMessage(0);
          else { TranslateMessage(&msg); DispatchMessageA(&msg); }
          break;
        case WM_PAINT:
          BeginPaint(win,&ps);
          term.RenderToWindow(ps.hdc);
          EndPaint(win,&ps);
          break;
        default:
          TranslateMessage(&msg); DispatchMessageA(&msg); break;
      }

    } // while

    term.HandleMessage((msg.message=0,msg),win); // timer check

    Sleep(1);
  }

  term.Quit();
  font.Quit();
  DeleteDC(screenDC);

  return 0;
}
