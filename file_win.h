// Windows file I/O wrapper functions
#ifndef FILE_WIN_H
#define FILE_WIN_H

#include "common.h"

// Exclude rarely-used Windows APIs to reduce header size and compile time
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Additional virtual key code definitions not always in windows.h
#ifndef VK_OEM_PLUS
#define VK_OEM_PLUS   0xBB  // '+' key (used for font size increase)
#endif
#ifndef VK_OEM_MINUS
#define VK_OEM_MINUS  0xBD  // '-' key (used for font size decrease)
#endif

// File attribute flags for Win32 CreateFile operations (unused in this program)
// These map to FILE_FLAG_* constants from Windows API
enum {
  ffWRITE_THROUGH         =0x80000000,  // Bypass OS write cache (direct to disk)
  ffOVERLAPPED            =0x40000000,  // Enable async I/O operations
  ffNO_BUFFERING          =0x20000000,  // Bypass all OS caching (requires aligned I/O)
  ffRANDOM_ACCESS         =0x10000000,  // Hint: file accessed randomly (affects cache strategy)
  ffSEQUENTIAL_SCAN       =0x08000000,  // Hint: file accessed sequentially (prefetch more)
  ffDELETE_ON_CLOSE       =0x04000000,  // Delete file when last handle closes (temp files)
  ffBACKUP_SEMANTICS      =0x02000000,  // Allow opening directories (not just files)
  ffPOSIX_SEMANTICS       =0x01000000,  // Case-sensitive filename matching
  ffOPEN_REPARSE_POINT    =0x00200000,  // Open symlink itself, not target
  ffOPEN_NO_RECALL        =0x00100000   // Don't retrieve from offline storage (HSM systems)
};

// Low-level Win32 file opening wrapper (unused - more specific functions below are used instead)
HANDLE Win32_Open( const wchar_t* s, uint Flags, uint Attrs );

// Open a directory handle (for directory operations like setting timestamps)
HANDLE file_opendir( const wchar_t* s );

// Create a directory
int file_mkdir( const wchar_t* name );

// Global file operation modes - these control how file_open() and file_make() behave
extern uint file_open_mode;
extern uint file_make_mode;
extern uint file_make_cmode;

// Set file_open() to read-only mode
void file_open_mode_r( void );
// Set file_open() to read-write mode
void file_open_mode_rw( void );

// Set file_make() to write-only mode
void file_make_mode_w( void );
// Set file_make() to read-write mode
void file_make_mode_rw( void );

// Set file_make() to open existing or create if doesn't exist
void file_make_open( void );
// Set file_make() to always create new file (overwrites existing)
void file_make_create( void );

// Open existing file (wide-char version) - used for reading files to compare
HANDLE file_open( const wchar_t* name );

// Open existing file (ANSI version) - used in this program for command-line filenames
HANDLE file_open( const char* name );

// Create or open file (wide-char version) - unused in this program
HANDLE file_make( const wchar_t* name );

// Create or open file (ANSI version) - unused in this program
HANDLE file_make( const char* name );

// Close file handle and release OS resources
int file_close( HANDLE file );

// Read from file (may return partial read if less data available than requested)
uint file_read( HANDLE file, void* _buf, uint len );

// Synchronized read - keeps reading until requested length or EOF
uint file_sread( HANDLE file, void* _buf, uint len );

// Write to file
uint file_writ( HANDLE file, void* _buf, uint len );

// Seek to position in file (supports 64-bit positions for large files)
qword file_seek( HANDLE file, qword ofs, int typ = FILE_BEGIN );

// Get current file position
qword file_tell( HANDLE file );

// Get file size using binary search (for files where normal method fails)
qword getfilesize( HANDLE f );

// Get file size (standard method with fallback)
qword file_size( HANDLE file );

// Get last error as text string (converts Windows error code to human-readable message)
char* GetErrorText( void );

// Expand path to full UNC path with \\?\ prefix for long path support (>260 chars)
uint ExpandPath( wchar_t* path, wchar_t* w, uint wsize );

// File handle wrapper providing convenient C++ interface
// Base class without constructor (allows derived class to initialize)
struct filehandle0 {

  HANDLE f;  // Windows file handle

  // Allow implicit conversion to int for boolean checks (non-zero = valid handle)
  operator int( void );

  // Open existing file (template works with char* or wchar_t*)
  template< typename CHAR >
  uint open( const CHAR* name ) {
    f = file_open( name );  // Call appropriate overload based on CHAR type
    return ((byte*)f)-((byte*)0);  // Return non-zero if successful
  }

  // Create or open file (template works with char* or wchar_t*)
  template< typename CHAR >
  uint make( const CHAR* name ) {
    f = file_make( name );  // Call appropriate overload based on CHAR type
    return ((byte*)f)-((byte*)0);  // Return non-zero if successful
  }

  // Close the file
  int close( void );

  // Get file size
  qword size( void );

  // Seek to position (default: absolute position from beginning)
  void seek( qword ofs, uint typ=FILE_BEGIN );

  // Get current position
  qword tell( void );

  // Read into structure/variable (template deduces size automatically)
  template< typename BUF >
  uint read( BUF& buf ) { return read( &buf, sizeof(buf) )!=sizeof(buf); }

  // Read raw bytes
  uint read( void* _buf, uint len );

  // Synchronized read into structure (keeps reading until buffer full or EOF)
  template< typename BUF >
  uint sread( BUF& buf ) { return sread( &buf, sizeof(buf) )!=sizeof(buf); }

  // Synchronized read raw bytes
  uint sread( void* _buf, uint len );

  // Read single character (returns -1 on EOF, character value otherwise)
  int Getc( void );

  // Write raw bytes
  uint writ( void* _buf, uint len );

  // Write structure/variable (template deduces size automatically)
  template< typename BUF >
  uint writ( BUF& buf ) {
    return writ( &buf, sizeof(buf) )!=sizeof(buf);
  }

};

// File handle with automatic initialization (adds constructor to base class)
struct filehandle : filehandle0 {
  // Default constructor: initialize handle to NULL
  filehandle();

  // Constructor that opens or creates file based on f_wr flag
  template< typename CHAR >
  filehandle( const CHAR* name, uint f_wr=0 ) {
    // If f_wr is non-zero, create/write file; otherwise open for reading
    f_wr ? make(name) : open(name);
  }
};

#endif // FILE_WIN_H
