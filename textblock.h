// Text buffer for displaying character grid
#ifndef TEXTBLOCK_H
#define TEXTBLOCK_H

#include "common.h"
#include "setfont.h"
#include "bitmap.h"
#include "palette.h"

// Forward declaration for textprint functions
void SymbOut_Init( void );

template< class myfont, class mybitmap >
void SymbOut( myfont& ch1, mybitmap& bm1,uint PX,uint PY, word cp );

// Text buffer for displaying character grid (like a terminal emulator buffer)
// Stores characters and their attributes (palette index) for each cell
struct textblock {
  uint  WCX,WCY;  // Window size in characters (columns, rows)
  uint  WPX,WPY;  // Position on screen in pixels (for rendering)
  uint  WSX,WSY;  // Size in pixels (for layout calculations)
  uint  blksize;  // Total cells = WCX * WCY
  word* text;     // Character + attribute buffer (low byte=char, high byte=palette index)

  // Clear all cells to null characters
  void Clear( void ) {
    uint i;
    if( text!=0 ) for( i=0; i<blksize; i++ ) text[i]=0;
  }

  // Initialize textblock with size and position
  template< class myfont >
  void Init( myfont& ch1, uint CX, uint CY, uint PX, uint PY ) {
    WCX=CX; WCY=CY; blksize=WCY*WCX;  // Set dimensions
    WPX=PX; WPY=PY;  // Set screen position
    // Calculate pixel size based on font dimensions
    WSX = WCX*ch1.wmax;  // Width in pixels
    WSY = WCY*ch1.hmax;  // Height in pixels
    text = new word[WCY*WCX];  // Allocate buffer
  }

  // Free resources
  void Quit( void ) {
    delete text;
  }

  // Access character cell at (x,y) - returns reference for read/write
  word& cell( uint x, uint y ) {
    return (word&)text[y*WCX+x];  // Row-major order
  }

  // Pack character and attribute into word (low byte=char, high byte=attribute)
  word ch( byte c, byte attr ) {
    return c | (attr<<8);
  }

  // Fill entire buffer with character+attribute pair
  void Fill( uint c, uint atr ) {
    uint i;
    for( i=0; i<blksize; i++ ) text[i]=ch(c,atr);
  }

  // Print text string to textblock with attribute toggling ('~' toggles between atr1 and atr2)
  // Used for help text where ~ marks text to highlight
  void Print( char* s, uint atr1, uint atr2=0, uint X=0, uint Y=0 ) {
    uint c,a=atr1,x=X,y=Y;  // c=current char, a=current attribute, x,y=cursor position
    while( c=*s++ ) {
      if( c=='\n' ) { x=X; y++; continue; }  // Newline: return to left margin, next row
      if( c=='~' ) { a^=atr1^atr2; continue; }  // Toggle attribute between atr1 and atr2
      if( (x<WCX) && (y<WCY) ) cell(x,y) = ch(c,a);  // Write char if in bounds
      x++;  // Advance cursor
    }
  }

  // Calculate text size (in characters) - used for layout calculations
  void textsize( char* s, uint* tb_SX, uint* tb_SY ) {
    uint c,x=0,y=0;
    while( c=*s++ ) {
      if( c=='\n' ) { x=0; y++; continue; }  // Newline
      if( c=='~' ) continue;  // Skip attribute toggle markers
      x++;  // Count characters
    }
    // Return width (x) and height (y+1 if partial line exists)
    if(tb_SX!=0) *tb_SX=x;
    if(tb_SY!=0) *tb_SY=y;
  }

  // Render textblock to bitmap using font (convert text buffer to pixels)
  template< class myfont, class mybitmap >
  void Print( myfont& ch1, mybitmap& bm1 ) {
    uint i,j;
    word* s = text;  // Pointer to current row in text buffer
    uint PX=WPX, PY=WPY;  // Current pixel position

    SymbOut_Init();  // Clear character rendering cache

    // For each row
    for( j=0; j<WCY; j++ ) {
      // For each column in the row
      for( i=0; i<WCX; i++ ) SymbOut( ch1, bm1,PX+i*ch1.wmax,PY, s[i] );
      PY+=ch1.hmax;  // Move to next row of pixels
      s+=WCX;        // Move to next row of characters
    }
  }

};

#endif // TEXTBLOCK_H
