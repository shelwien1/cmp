// Window utilities
#ifndef WINDOW_H
#define WINDOW_H

#include "common.h"
#include <windows.h>

// Global window handles
extern HWND win;       // Main window handle
extern HDC  dibDC;     // Device context for offscreen bitmap
extern PAINTSTRUCT ps; // Paint structure for WM_PAINT

// Draw animated box (dashed line with moving offset for selection indicator)
void DrawBox( HDC dc, int x1,int y1, int x2,int y2, int dx=0 );

// Draw line between two points with specified pen
void DrawLine( HDC dc, HGDIOBJ& hPen, int x1,int y1, int x2,int y2 );

// Request full window redraw
void DisplayRedraw( void );

// Request partial window redraw (specific rectangle)
void DisplayRedraw( int x, int y, int dx, int dy );

// Move window by delta (mode: bit 0=move position, bit 1=change size)
void ShiftWindow( int x, int y, uint mode=3 );

// Set absolute window position and size
void SetWindow( HWND win, int x, int y, int w, int h );

// Set window size (keeping current position)
void SetWindowSize( HWND win, int w, int h );

#endif // WINDOW_H
