// Terminal Emulator - Standalone test program using libterminal

#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

// Use the libterminal library
#include "libterminal.h"

#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")

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
