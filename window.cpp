// Window utilities implementation
#include "window.h"

// Global window handles
HWND win = 0;       // Main window handle
HDC  dibDC = 0;     // Device context for offscreen bitmap
PAINTSTRUCT ps;     // Paint structure for WM_PAINT

// Draw animated box (dashed line with moving offset for selection indicator)
void DrawBox( HDC dc, int x1,int y1, int x2,int y2, int dx ) {
  dx %= (x2-x1);  // Animate by shifting start point
  // Create a polygon path that draws the box with animated offset
  POINT a[] = { { x1+dx,y1 }, { x2,y1 }, { x2,y2 }, { x1,y2 }, { x1,y1 }, { x1+dx,y1 } };
  Polyline( dc, a, 6 );
}

// Draw line between two points with specified pen
void DrawLine( HDC dc, HGDIOBJ& hPen, int x1,int y1, int x2,int y2 ) {
  POINT a[] = { { x1,y1 }, { x2,y2 } };
  HGDIOBJ hPenOld = SelectObject( dibDC, hPen );  // Select pen
  Polyline( dc, a, 2 );  // Draw line
  SelectObject( dibDC, hPenOld );  // Restore old pen
}

// Request full window redraw
void DisplayRedraw( void ) {
  InvalidateRect( win, 0, 0 );  // Invalidate entire window
}

// Request partial window redraw (specific rectangle)
void DisplayRedraw( int x, int y, int dx, int dy ) {
  RECT v;
  v.left   = x;
  v.top    = y;
  v.right  = x+dx;
  v.bottom = y+dy;
  InvalidateRect( win, &v, 0 );  // Invalidate only specified region
}

// Move window by delta (mode: bit 0=move position, bit 1=change size)
void ShiftWindow( int x, int y, uint mode ) {
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  GetWindowPlacement( win, &winpl );
  winpl.rcNormalPosition.left  += (mode&1) ? x : 0;  // Move left edge if mode&1
  winpl.rcNormalPosition.top   += (mode&1) ? y : 0;  // Move top edge if mode&1
  winpl.rcNormalPosition.right += (mode&2) ? x : 0;  // Resize width if mode&2
  winpl.rcNormalPosition.bottom+= (mode&2) ? y : 0;  // Resize height if mode&2
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}

// Set absolute window position and size
void SetWindow( HWND win, int x, int y, int w, int h ) {
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  winpl.rcNormalPosition.left  = x;
  winpl.rcNormalPosition.top   = y;
  winpl.rcNormalPosition.right = x+w;
  winpl.rcNormalPosition.bottom= y+h;
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}

// Set window size (keeping current position)
void SetWindowSize( HWND win, int w, int h ) {
  int x,y;
  WINDOWPLACEMENT winpl = { sizeof(WINDOWPLACEMENT) };
  GetWindowPlacement( win, &winpl );
  x = winpl.rcNormalPosition.left;   // Keep current position
  y = winpl.rcNormalPosition.top;
  winpl.rcNormalPosition.right = x+w;   // Set new width
  winpl.rcNormalPosition.bottom= y+h;   // Set new height
  winpl.showCmd = SW_SHOW;
  SetWindowPlacement( win, &winpl );
}
