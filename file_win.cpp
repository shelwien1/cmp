// Windows file I/O wrapper implementation
#include "file_win.h"

// Global file operation modes - these control how file_open() and file_make() behave
// Default: open files in read-only mode
uint file_open_mode = GENERIC_READ;
// Default: create files in write-only mode
uint file_make_mode = GENERIC_WRITE;
// Default: file_make() always creates new file (overwrites existing)
uint file_make_cmode = CREATE_ALWAYS;

// Low-level Win32 file opening wrapper (unused - more specific functions below are used instead)
HANDLE Win32_Open( const wchar_t* s, uint Flags, uint Attrs ) {
  return CreateFileW(
    s,                              // File path
    GENERIC_READ | GENERIC_WRITE,   // Request read and write access
    0,                              // No sharing - exclusive access
    0,                              // Default security attributes
    Flags,                          // Creation disposition (OPEN_EXISTING, CREATE_NEW, etc.)
    Attrs,                          // File attributes and flags
    0                               // No template file
  );
}

// Open a directory handle (for directory operations like setting timestamps)
HANDLE file_opendir( const wchar_t* s ) {
  HANDLE r = CreateFileW(
     s,                                 // Directory path
     GENERIC_WRITE,                     // Need write access to modify directory metadata
     FILE_SHARE_READ | FILE_SHARE_WRITE,// Allow others to access while open
     0,                                 // Default security
     OPEN_EXISTING,                     // Directory must already exist
     FILE_FLAG_BACKUP_SEMANTICS,        // Required flag to open directories (not files)
     0                                  // No template
   );
  // Convert INVALID_HANDLE_VALUE to NULL for easier checking
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create a directory
int file_mkdir( const wchar_t* name ) {
  // Returns non-zero on success, zero on failure
  return CreateDirectoryW( name, 0 );
}

// Set file_open() to read-only mode
void file_open_mode_r( void ) { file_open_mode = GENERIC_READ; }
// Set file_open() to read-write mode
void file_open_mode_rw( void ) { file_open_mode = GENERIC_READ | GENERIC_WRITE; }

// Set file_make() to write-only mode
void file_make_mode_w( void ) { file_make_mode = GENERIC_WRITE; }
// Set file_make() to read-write mode
void file_make_mode_rw( void ) { file_make_mode = GENERIC_READ | GENERIC_WRITE; }

// Set file_make() to open existing or create if doesn't exist
void file_make_open( void ) { file_make_cmode = OPEN_ALWAYS; }
// Set file_make() to always create new file (overwrites existing)
void file_make_create( void ) { file_make_cmode = CREATE_ALWAYS; }

// Open existing file (wide-char version) - used for reading files to compare
HANDLE file_open( const wchar_t* name ) {
  HANDLE r = CreateFileW(
     name,                                       // File path (Unicode)
     file_open_mode,                             // Access mode (controlled by global)
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow full sharing
     0,                                          // Default security
     OPEN_EXISTING,                              // File must exist (fail if not found)
     0,                                          // No special flags
     0                                           // No template
  );
  // Convert INVALID_HANDLE_VALUE to NULL for easier error checking
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Open existing file (ANSI version) - used in this program for command-line filenames
HANDLE file_open( const char* name ) {
  HANDLE r = CreateFileA(
     name,                                       // File path (ANSI/OEM)
     file_open_mode,                             // Access mode (controlled by global)
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow full sharing
     0,                                          // Default security
     OPEN_EXISTING,                              // File must exist
     0,                                          // No special flags
     0                                           // No template
  );
  // Convert INVALID_HANDLE_VALUE to NULL
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create or open file (wide-char version) - unused in this program
HANDLE file_make( const wchar_t* name ) {
  HANDLE r = CreateFileW(
     name,                                       // File path (Unicode)
     file_make_mode,                             // Access mode (controlled by global)
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow full sharing
     0,                                          // Default security
     file_make_cmode,                            // Creation mode (controlled by global)
     0,                                          // No special flags
     0                                           // No template
  );
  // Convert INVALID_HANDLE_VALUE to NULL
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Create or open file (ANSI version) - unused in this program
HANDLE file_make( const char* name ) {
  HANDLE r = CreateFileA(
     name,                                       // File path (ANSI/OEM)
     file_make_mode,                             // Access mode (controlled by global)
     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,  // Allow full sharing
     0,                                          // Default security
     file_make_cmode,                            // Creation mode (controlled by global)
     0,                                          // No special flags
     0                                           // No template
  );
  // Convert INVALID_HANDLE_VALUE to NULL
  return r!=INVALID_HANDLE_VALUE ? r : 0;
}

// Close file handle and release OS resources
int file_close( HANDLE file ) {
  // Returns non-zero on success
  return CloseHandle( file );
}

// Read from file (may return partial read if less data available than requested)
uint file_read( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r = 0;  // Number of bytes actually read
  // Read up to 'len' bytes into buffer, store actual count in 'r'
  ReadFile( file, buf, len, (LPDWORD)&r, 0 );
  // Return actual bytes read (may be less than len if near EOF)
  return r;
}

// Synchronized read - keeps reading until requested length or EOF
// Used when you must read exactly 'len' bytes or reach EOF
uint file_sread( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r;      // Bytes read in current iteration
  uint flag=1; // Success flag from ReadFile
  uint l = 0;  // Total bytes read so far

  // Keep reading until we get all requested bytes or hit EOF/error
  do {
    r = 0;
    // Read remaining bytes (len-l) starting at buf+l
    flag = ReadFile( file, buf+l, len-l, (LPDWORD)&r, 0 );
    l += r;  // Accumulate total bytes read
  } while( (r>0) && (l<len) && flag );  // Continue if made progress and not done

  return l;  // Return total bytes read
}

// Write to file
uint file_writ( HANDLE file, void* _buf, uint len ) {
  byte* buf = (byte*)_buf;
  uint r = 0;  // Number of bytes actually written
  // Write 'len' bytes from buffer, store actual count in 'r'
  WriteFile( file, buf, len, (LPDWORD)&r, 0 );
  return r;  // Return actual bytes written
}

// Seek to position in file (supports 64-bit positions for large files)
qword file_seek( HANDLE file, qword ofs, int typ ) {
  uint lo,hi;
  lo = uint(ofs);       // Low 32 bits of offset
  hi = uint(ofs>>32);   // High 32 bits of offset
  // SetFilePointer: pass low 32 bits directly, high 32 bits via pointer
  lo = SetFilePointer( file, lo, (PLONG)&hi, typ );
  // Reconstruct 64-bit result from returned high and low parts
  ofs = hi;
  ofs = (ofs<<32) + lo;
  return ofs;  // Return new file position
}

// Get current file position
qword file_tell( HANDLE file ) {
  // Seek by 0 bytes from current position returns current position
  return file_seek( file, 0, FILE_CURRENT );
}

// Get file size using binary search (for files where normal method fails)
// This is a fallback for special files (e.g., raw disk devices) where seeking to end doesn't work
qword getfilesize( HANDLE f ) {
  qword pos=-1LL;  // Will hold the file size
  const uint bufsize=1<<16,bufalign=1<<12;  // 64KB buffer, 4KB alignment
  byte* _buf = new byte[bufsize+bufalign];  // Allocate extra for alignment
  if( _buf!=0 ) {
    // Align buffer to page boundary (bufalign) for better I/O performance
    byte* buf = _buf + bufalign-((_buf-((byte*)0))%bufalign);
    uint bit,len;
    pos = 0;
    // Binary search from bit 62 down to 0 (searching up to ~4 exabytes)
    for( bit=62; bit!=-1; bit-- ) {
      pos |= 1ULL << bit;  // Try setting this bit (assume file is at least this large)
      file_seek(f,pos);    // Seek to that position
      len = file_read(f,buf,bufsize);  // Try to read 64KB
      // If we got data but less than buffer size, we're at EOF
      if( (len!=0) && (len<bufsize) ) { pos+=len; break; }
      // If we got full buffer, file is at least this large - keep the bit set
      if( len>=bufsize ) continue;
      // If we got no data, we seeked past EOF - clear this bit
      pos &= ~(1ULL<<bit);
    }
    delete[] _buf;
  }
  return pos;  // Return discovered file size
}

// Get file size (standard method with fallback)
qword file_size( HANDLE file ) {
  qword t = file_tell(file);  // Save current position
  qword r = file_seek( file, 0, FILE_END );  // Seek to end, returns position = size
  if( uint(r)==0xFFFFFFFF ) {
    // SetFilePointer returns 0xFFFFFFFF on error (but this is also a valid position!)
    // Check if there's actually an error
    if( GetLastError()!=0 ) r = getfilesize(file);  // Use binary search fallback
  }
  t = file_seek( file, t );  // Restore original position
  return r;  // Return file size
}

// Get last error as text string (converts Windows error code to human-readable message)
char* GetErrorText( void ) {
  wchar_t* lpMsgBuf;
  DWORD dw = GetLastError();  // Get last error code from Windows
  // Ask Windows to format the error message for us
  FormatMessageW(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, NULL
  );
  static char out[32768];  // Static buffer for return value
  int wl=wcslen(lpMsgBuf);  // Get length of wide-char message
  // Convert Unicode message to OEM code page (for console output)
  WideCharToMultiByte( CP_OEMCP, 0, lpMsgBuf,wl+1, out,sizeof(out), 0,0 );
  LocalFree(lpMsgBuf);  // Free the buffer allocated by FormatMessageW
  wl = strlen(out);
  // Remove trailing whitespace/control chars (error messages often have \r\n)
  for( --wl; wl>=0; wl-- ) if( (byte&)out[wl]<32 ) out[wl]=' ';
  return out;  // Return pointer to static buffer
}

// Expand path to full UNC path with \\?\ prefix for long path support (>260 chars)
uint ExpandPath( wchar_t* path, wchar_t* w, uint wsize ) {
  wcscpy( w, L"\\\\?\\" );  // Start with \\?\ prefix for long path support
  // Get full path (resolves relative paths, . and ..) and append to prefix
  GetFullPathNameW( path, wsize/sizeof(wchar_t)-4,&w[4], 0 );
  // If result is UNC path (\\server\share), remove the \\?\ prefix (UNC uses \\?\UNC\ instead)
  if( (w[4]=='\\') && (w[5]=='\\') ) wcscpy( &w[0], &w[4] );
  return wcslen(w);  // Return length of expanded path
}

// filehandle0 implementations
filehandle0::operator int( void ) {
  // Convert HANDLE pointer to integer (NULL becomes 0, valid handle becomes non-zero)
  return ((byte*)f)-((byte*)0);
}

int filehandle0::close( void ) {
  return file_close(f);
}

qword filehandle0::size( void ) { return file_size(f); }

void filehandle0::seek( qword ofs, uint typ ) {
  file_seek( f, ofs, typ );
}

qword filehandle0::tell( void ) {
  return file_tell(f);
}

uint filehandle0::read( void* _buf, uint len ) { return file_read( f, _buf, len ); }

uint filehandle0::sread( void* _buf, uint len ) { return file_sread( f, _buf, len ); }

int filehandle0::Getc( void ) {
  byte c;
  uint l = read(c);  // Read one byte
  return l ? -1 : c;  // If read failed (l != 0), return -1, else return character
}

uint filehandle0::writ( void* _buf, uint len ) {
  return file_writ( f, _buf, len );
}

// filehandle implementations
filehandle::filehandle() { f=0; }
