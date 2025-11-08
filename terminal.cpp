// Terminal Emulator - Encapsulated terminal component with blinking cursor and line editing

#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")

//--- Type definitions
typedef unsigned short word;
typedef unsigned int   uint;
typedef unsigned char  byte;
typedef unsigned long long qword;

//--- Terminal state and encapsulation
struct Terminal {

  //--- DIB bitmap wrapper for double buffering
  struct DIB {
    HBITMAP hbm;
    byte*   ptr;
    int     bmX,bmY;

    void Init( int X, int Y ) {
      bmX=X; bmY=Y;
      BITMAPINFO bmi;
      memset(&bmi,0,sizeof(bmi));
      bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth = bmX;
      bmi.bmiHeader.biHeight = -bmY;
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;
      hbm = CreateDIBSection( 0, &bmi, DIB_RGB_COLORS, (void**)&ptr, 0, 0 );
    }

    void Reset( void ) {
      if(ptr) memset(ptr,0,bmX*bmY*4);
    }

    void Quit( void ) {
      if( hbm!=0 ) DeleteObject(hbm);
      hbm=0; ptr=0;
    }
  };

  //--- Font rendering wrapper
  struct Font {
    HFONT   hfont;
    int     wmax,hmax;
    LOGFONTA lf;
    HDC screenDC;

    void Init( const char* fontname, int height ) {
      memset(&lf,0,sizeof(lf));
      lf.lfHeight = height;
      lf.lfWidth = 0;
      lf.lfWeight = FW_NORMAL;
      lf.lfCharSet = DEFAULT_CHARSET;
      lf.lfQuality = NONANTIALIASED_QUALITY;
      lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
      strncpy(lf.lfFaceName,fontname,sizeof(lf.lfFaceName)-1);

      hfont = CreateFontIndirectA(&lf);

      screenDC=CreateCompatibleDC(0);
      SelectObject(screenDC,GetStockObject(BLACK_BRUSH));

      if( hfont ) GetMetrics();
    }

    void GetMetrics() {
      HDC& dc = screenDC;
      TEXTMETRICA tm;
      SIZE sz;
      SelectObject(dc,hfont);
      GetTextMetricsA(dc,&tm);
      GetTextExtentPoint32A(dc,"W",1,&sz);
      wmax = sz.cx;
      hmax = tm.tmHeight;
    }

    void Quit( void ) {
      DeleteDC(screenDC);
      if( hfont!=0 ) DeleteObject(hfont);
      hfont=0;
    }
  };

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

  DIB     dib;
  HDC     dibDC;
  Font*   font;

  void Init( Font& fnt, const RECT& rc ) {

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
    dib.Init(dib_w, dib_h);
    SelectObject(dibDC, dib.hbm);
  }

  void Quit() {
    dib.Quit();
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
    RenderTerminal(dib, dibDC, *font, cursor_blink);
    BitBlt(wndDC, area.left, area.top, dib.bmX, dib.bmY, dibDC, 0, 0, SRCCOPY);
  }

  //--- Rendering functions
  void DrawTextClamped( DIB& dib, HDC dc, Font& font, const char* text, int x, int y, uint color ) {
    int len=(int)strlen(text);
    if(len==0) return;
    SIZE sz; RECT rc;
    SetTextColor(dc,color);
    SetBkColor(dc,bg_color);
    SelectObject(dc,font.hfont);
    GetTextExtentPoint32A(dc,text,len,&sz);
    if(x<0) x=0; if(y<0) y=0;
    if(x+sz.cx>dib.bmX) sz.cx = dib.bmX-x;
    if(y+sz.cy>dib.bmY) sz.cy = dib.bmY-y;
    if(sz.cx<=0||sz.cy<=0) return;
    rc.left=x; rc.top=y; rc.right=x+sz.cx; rc.bottom=y+sz.cy;
    ExtTextOutA(dc,x,y,ETO_OPAQUE,&rc,text,len,NULL);
  }

  void DrawCursor( DIB& dib, int x, int y, int w, int h, int show ) {
    if(show==0) return;
    int cursor_h=h/4; if(cursor_h<2) cursor_h=2;
    if(x<0||y<0) return;
    if(x+w>dib.bmX) w=dib.bmX-x;
    if(y+h>dib.bmY) h=dib.bmY-y;
    if(w<=0||h<=0) return;
    for(int j=0;j<cursor_h;j++) {
      byte* p = dib.ptr + (y+h-cursor_h+j)*dib.bmX*4 + x*4;
      for(int i=0;i<w;i++) {
        p[0]=192; p[1]=192; p[2]=192; p+=4;
      }
    }
  }

  void RenderTerminal( DIB& dib, HDC dc, Font& font, int cursor_blink ) {
    dib.Reset();
    int y=0;
    int vis_start=scroll_pos;
    int vis_end=scroll_pos+max_lines;
    if(vis_end>line_count) vis_end=line_count;
    for(int i=vis_start;i<vis_end;i++) {
      DrawTextClamped(dib,dc,font,LineAt(i),0,y,fg_color);
      y+=font.hmax;
    }
    if(!current_line) return;
    // Build prompt+current_line into a temporary buffer sized to line_width
    //char buf_local[512];
    buf_local[0] = '>';
    strncpy(buf_local+1, current_line, (size_t)line_width - 2);
    buf_local[1 + (line_width - 2)] = 0;
    int buf_len=(int)strlen(buf_local);
    int start_char=hscroll_pos;
    int end_char=hscroll_pos+cols;
    if(start_char<0) start_char=0;
    if(end_char>buf_len) end_char=buf_len;
    if(start_char>buf_len) start_char=buf_len;
    // visible buffer
    //char vis_buf[512]; 
    memset(vis_buf,0,line_width);
    for(int i=start_char;i<end_char;i++) vis_buf[i-start_char]=buf_local[i];
    DrawTextClamped(dib,dc,font,vis_buf,0,y,fg_color);
    if(cursor_blink) {
      int vis_cursor_pos=(1+cursor_pos)-hscroll_pos;
      if(vis_cursor_pos>=0 && vis_cursor_pos<cols) {
        int cursor_x=font.wmax*vis_cursor_pos;
        DrawCursor(dib,cursor_x,y,font.wmax,font.hmax,1);
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

  WNDCLASSA wc;
  memset(&wc,0,sizeof(wc));
  wc.lpfnWndProc=WndProc; wc.hInstance=GetModuleHandle(0);
  wc.hCursor=LoadCursor(0,IDC_ARROW);
  wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszClassName="TermClass"; RegisterClassA(&wc);

  Terminal::Font font;
  font.Init("Courier New",-16);

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

  return 0;
}
