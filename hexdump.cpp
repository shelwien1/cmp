// Hex file viewer implementation
#include "hexdump.h"

// Calculate required text buffer width in characters for hex display
uint hexfile::Calc_WCX( uint mBX, uint f_addr64, uint f_vertline, uint mode ) {
  uint waddr = f_addr64 ? 8+1+8 : 8;  // Address: "XXXXXXXX" or "XXXXXXXX:XXXXXXXX"
  uint whex = 3*mBX;                   // Hex dump: "XX " per byte
  uint wtxt = 1*mBX;                   // ASCII: one char per byte
  uint WCX;

  // Calculate width based on display mode
  if( mode == MODE_HEX_ONLY ) {
    // Hex-only: "ADDRESS: " + "hex hex hex..." + "|"
    WCX = waddr+1+1+whex+(f_vertline>0);
  } else if( mode == MODE_TEXT_ONLY || mode == MODE_GRAYSCALE ) {
    // Text-only or Grayscale: "ADDRESS: " + "ASCII/spaces..." + "|"
    WCX = waddr+1+1+wtxt+(f_vertline>0);
  } else {
    // Combined (default): "ADDRESS: " + "hex hex hex..." + " " + "ASCII..." + "|"
    WCX = waddr+1+1+whex+1+wtxt+(f_vertline>0);
  }
  return WCX;
}

// Initialize textbuffer parameters (called during setup/resize)
void hexfile::SetTextbuf( textblock& tb1, uint _BX, uint _flags, uint _display_mode ) {
  flags = _flags;  // Store display flags
  BX = _BX;        // Bytes per line
  display_mode = _display_mode;  // Store display mode

  // Calculate how many bytes actually fit in the allocated text buffer
  uint aw = (flags&f_addr64)? 8+1+8 : 8;  // Address column width
  uint available = tb1.WCX - aw - 2;  // Space after address, ':', and ' '

  // Calculate bytes per line based on display mode
  if( _display_mode == MODE_HEX_ONLY ) {
    // Hex-only: each byte takes 3 chars ("XX ")
    F1cpl = available / 3;
    F1dpl = available % 3;
  } else if( _display_mode == MODE_TEXT_ONLY ) {
    // Text-only: each byte takes 1 char
    F1cpl = available / 1;
    F1dpl = available % 1;
  } else {
    // Combined: each byte takes 4 chars (3 hex + 1 text), plus 1 separator
    available -= 1;  // Account for space between hex and text sections
    F1cpl = available / 4;
    F1dpl = available % 4;
  }

  textlen = BX*tb1.WCY;  // Total bytes that can be displayed
  diffbuf = new byte[textlen];  // Allocate difference highlight buffer
  if( diffbuf!=0 ) bzero( diffbuf, textlen );  // Clear to "no differences"
}

// Cleanup resources
void hexfile::Quit( void ) {
  delete diffbuf;
}

// Get byte at offset i from current view (returns -1 if beyond EOF)
uint hexfile::viewdata( uint i ) {
  qword ofs = F1pos+i;  // Absolute file position
  uint c, f = (ofs>=viewbeg) && (ofs<viewend);  // Check if in cached range
  c = f ? databuf[F1pos-databeg+i] : -1;  // Get from cache or return -1 (EOF marker)
  return c;
}

// Compare this file with another (unused - comparison now done in main loop)
void hexfile::Compare( hexfile& F2 ) {
  uint c1,c2,i;

  // For each byte in the visible view
  for( i=0; i<textlen; i++ ) {
    c1 = (*this).viewdata(i);  // Get byte from this file
    c2 = F2.viewdata(i);       // Get byte from other file
    diffbuf[i] = (c1!=c2);     // Mark as different if bytes don't match
  }
}

// Move view position by predefined navigation type
void hexfile::MovePos( uint m_type ) {
  // Navigation deltas for: left, right, up, down, pgup, pgdn, wheel-up, wheel-dn
  const int delta[] = { -1,1, -int(BX),int(BX), -int(textlen),int(textlen), -int(BX*4), int(BX*4) };
  // m_type: 0=home, 1=end, 2-9=use delta array
  SetFilepos( (m_type==0)? 0 : (m_type==1) ? F1size-textlen : F1pos+sqword(delta[m_type-2]) );
}

// Move view by relative byte offset
void hexfile::MoveFilepos( int newpos ) {
  F1pos += sqword(newpos);  // Add delta to current position
  SetFilepos( F1pos );      // Update view (may reload cache)
}

// Set absolute view position and update cache if needed (intelligent cache management)
void hexfile::SetFilepos( qword newpos ) {
  uint len;
  F1pos=newpos;  // Update view position

  qword newend = newpos+textlen;  // Calculate end of visible region

  // Clamp to file bounds
  if( newend>F1size ) newend=F1size;  // Can't go past EOF
  if( newend<newpos ) newpos=0;       // Handle underflow

  // Check if requested region is already cached
  if( (newpos>=databeg) && (newend<=dataend) ) {
    // All necessary data is in buffer already - no I/O needed!
  } else {
    // Need to load new data from disk
    // Align to 64KB boundary for better disk I/O performance and read-ahead
    databeg = newpos - (newpos % datalign);
    F1.seek( databeg );  // Seek to aligned position
    len = F1.read( databuf, datalen );  // Read 1MB into cache
    dataend = databeg + len;  // Calculate end of cached region
    if( newend>=dataend ) newend=dataend;  // Adjust if near EOF
  }

  viewbeg = newpos;  // Update visible region boundaries
  viewend = newend;
}

// Open file and get size
size_t hexfile::Open( char* fnam ) {
  F1pos=0;           // Start at beginning of file
  databeg = dataend=0;  // Cache is empty
  if( F1.open(fnam) ) {  // Open file for reading
    F1size = F1.size();  // Get total file size
  }
  // Return non-zero if successful (handle converted to size_t)
  return ((byte*)F1.f)-((byte*)0);
}

// Print hex number with specified width to textblock buffer
word* hexfile::HexPrint( word* s, qword x, uint w, uint attr ) {
  uint i,c;
  attr<<=8;  // Shift attribute to high byte
  // Print hex digits from left to right (most significant first)
  for( i=w-1; i!=-1; i-- ) {
    c = '0'+((x>>(i*4))&15); if( c>'9' ) c+='A'-('9'+1);  // Convert 0-15 to '0'-'9','A'-'F'
    *s++ = c | attr;  // Pack character and attribute
  }
  return s;  // Return pointer to next position
}

// Render hex dump to textblock (generates the hex view display)
void hexfile::hexdump( textblock& tb1 ) {

  uint c,i,j;
  uint cpl = F1cpl;  // Characters (bytes) per line that fit
  uint dpl = F1dpl;  // Dead space padding
  uint len = textlen;
  byte* p  = &databuf[F1pos-databeg];  // Pointer to data in cache

  qword ofs;  // Current file offset

  word* s = &tb1.cell(0,0);  // Pointer to text buffer
  word* _s = s;

  // For each line (row) in the display
  for( j=0; j<tb1.WCY; j++ ) {
    ofs = F1pos+j*BX;  // File offset for this row

    // Print address column (64-bit or 32-bit)
    if( flags&f_addr64 ) {
      s = HexPrint( s, ofs>>32, 8, pal_Addr );  // High 32 bits
      *s++ = tb1.ch(':',pal_Addr);  // Separator
    }
    s = HexPrint( s, ofs, 8, pal_Addr );  // Low 32 bits (or full 32-bit address)

    *s++ = tb1.ch(':',pal_Addr);  // Address-hex separator
    *s++ = tb1.ch(' ',pal_Hex);   // Space

    // Render based on display mode
    if( display_mode == MODE_TEXT_ONLY ) {
      // Text-only mode: skip hex section, render only ASCII
      for( i=0; i<cpl; i++ ) {
        c = tb1.ch(' ',pal_Hex);  // Default to space
        // If byte is within viewable range
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          // Print byte as character (control chars will display as-is)
          c = tb1.ch( p[j*BX+i], diffbuf[j*BX+i]?pal_Diff:pal_Hex );
        }
        *s++ = tb1.ch( byte(c),pal_Hex);  // Write character
      }
    } else if( display_mode == MODE_GRAYSCALE ) {
      // Grayscale mode: display spaces with grayscale background based on byte value
      for( i=0; i<cpl; i++ ) {
        c = tb1.ch(' ',pal_Hex);  // Default to space with normal palette
        // If byte is within viewable range
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          // Map byte value (0-255) to grayscale palette index (6-255)
          byte byte_val = p[j*BX+i];
          uint gray_idx = pal_GRAY_START + ((byte_val * (pal_MAX - pal_GRAY_START - 1)) / 255);
          c = tb1.ch( ' ', gray_idx );  // Display space with grayscale background
        }
        *s++ = c;  // Write character with grayscale attribute
      }
    } else if( display_mode == MODE_HEX_ONLY ) {
      // Hex-only mode: render only hex bytes
      for( i=0; i<cpl; i++ ) {
        s[2]=s[1]=s[0] = tb1.ch(' ',pal_Hex);  // Default to spaces
        // If byte is within viewable range (not past EOF)
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          // Print 2-digit hex value, use pal_Diff if byte differs from other files
          HexPrint( s, p[j*BX+i], 2, diffbuf[j*BX+i]?pal_Diff:pal_Hex );
        }
        s+=3;  // Move to next byte position (2 hex + 1 space)
      }
      if( F1cpl<BX ) s[-1]=tb1.ch('>',pal_Addr);  // '>' indicator if line truncated
    } else {
      // Combined mode (default): render both hex and ASCII
      // Print hex bytes section
      for( i=0; i<cpl; i++ ) {
        s[2]=s[1]=s[0] = tb1.ch(' ',pal_Hex);  // Default to spaces
        // If byte is within viewable range (not past EOF)
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          // Print 2-digit hex value, use pal_Diff if byte differs from other files
          HexPrint( s, p[j*BX+i], 2, diffbuf[j*BX+i]?pal_Diff:pal_Hex );
        }
        s+=3;  // Move to next byte position (2 hex + 1 space)
      }
      if( F1cpl<BX ) s[-1]=tb1.ch('>',pal_Addr);  // '>' indicator if line truncated

      *s++ = tb1.ch(' ',pal_Hex);  // Space before ASCII section

      // Print ASCII representation section
      for( i=0; i<cpl; i++ ) {
        c = tb1.ch(' ',pal_Hex);  // Default to space
        // If byte is within viewable range
        if( ((ofs+i)>=viewbeg) && ((ofs+i)<viewend) ) {
          // Print byte as character (control chars will display as-is)
          c = tb1.ch( p[j*BX+i], diffbuf[j*BX+i]?pal_Diff:pal_Hex );
        }
        *s++ = tb1.ch( byte(c),pal_Hex);  // Write character
      }
    }

    // Fill any dead space at end of line
    for( i=0; i<dpl; i++ ) *s++=tb1.ch(' ',pal_Hex);

    // Draw vertical separator if enabled (rightmost column)
    if( flags & f_vertline ) s[-1]=tb1.ch('|',pal_Sep);

  }

}
