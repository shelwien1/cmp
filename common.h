// Common types, templates and utility functions
#ifndef COMMON_H
#define COMMON_H

// Standard I/O for printf debugging
#include <stdio.h>
// Standard library for memory allocation (new/delete)
#include <stdlib.h>
// Standard definitions for size_t and NULL
#include <stddef.h>
// String functions for strcpy, strlen, etc.
#include <string.h>
// Memory functions for memcpy, memset, etc.
#include <memory.h>
// Undefine EOF if previously defined - we don't use stdio file I/O
#undef EOF

// Pack structures to 1-byte alignment - ensures no padding between fields for binary I/O
// Use push/pop to scope packing to this header only, preventing it from affecting Windows structures
#pragma pack(push, 1)

// Type aliases for consistent width types across different platforms
typedef unsigned short word;       // 16-bit unsigned (used for char+attribute pairs)
typedef unsigned int   uint;       // 32-bit unsigned (general purpose)
typedef unsigned char  byte;       // 8-bit unsigned (file data, color components)
typedef unsigned long long qword;  // 64-bit unsigned (file positions and sizes)
typedef signed long long sqword;   // 64-bit signed (for position deltas)

// Zero-initialization templates for various data types
// Single object: zero out all bytes of the object
template <class T> void bzero( T &_p ) { int i; byte* p = (byte*)&_p; for( i=0; i<sizeof(_p); i++ ) p[i]=0; }
// Fixed-size array: call default constructor (which may zero) for each element
template <class T, int N> void bzero( T (&p)[N] ) { int i; for( i=0; i<N; i++ ) p[i]=0; }
// Pointer + count: zero out N elements
template <class T> void bzero( T* p, int N ) { int i; for( i=0; i<N; i++ ) p[i]=0; }
// 2D array: zero out all elements in the matrix
template <class T, int N, int M>  void bzero(T (&p)[N][M]) { for( int i=0; i<N; i++ ) for( int j=0; j<M; j++ ) p[i][j] = 0; }

// Utility templates
// Return minimum of two values
template <class T> T Min( T x, T y ) { return (x<y) ? x : y; }
// Return maximum of two values
template <class T> T Max( T x, T y ) { return (x>y) ? x : y; }
// Get number of elements in a static array
template <class T,int N> int DIM( T (&wr)[N] ) { return sizeof(wr)/sizeof(wr[0]); };
// Align value x up to next multiple of r (e.g., AlignUp(13, 16) = 16)
#define AlignUp(x,r) ((x)+((r)-1))/(r)*(r)

// Compile-time word constant generator for FourCC-style codes
// Packs 4 bytes into 32-bit integer in both little-endian (n) and big-endian (x) order
template<byte a,byte b,byte c,byte d> struct wc {
  static const unsigned int n=(d<<24)+(c<<16)+(b<<8)+a;  // Little-endian: d is MSB
  static const unsigned int x=(a<<24)+(b<<16)+(c<<8)+d;  // Big-endian: a is MSB
};

// Compiler-specific attributes for function inlining and alignment
#ifdef __GNUC__
 // GCC/Clang: Force function to be inlined for performance
 #define INLINE   __attribute__((always_inline))
 // GCC/Clang: Prevent function from being inlined
 #define NOINLINE __attribute__((noinline))
 // GCC/Clang: Align variable/struct to n-byte boundary (for SIMD, cache optimization)
 #define ALIGN(n) __attribute__((aligned(n)))
 // GCC/Clang: No-op - GCC doesn't need explicit alignment hints for pointers
 #define __assume_aligned(x,y)
#else
 // MSVC: Force function to be inlined
 #define INLINE   __forceinline
 // MSVC: Prevent function from being inlined
 #define NOINLINE __declspec(noinline)
 // MSVC: Align variable/struct to n-byte boundary
 #define ALIGN(n) __declspec(align(n))
#endif

// Compiler compatibility for __builtin_expect and __assume
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 // MSVC: No branch prediction hints, just return the expression
 #define __builtin_expect(x,y) (x)
 // MSVC: No pointer alignment hints needed
 #define __assume_aligned(x,y)
#endif

#if !defined(_MSC_VER) && !defined(__INTEL_COMPILER)
 // GCC/Clang: Map __assume to expression (MSVC provides optimizer hints via __assume)
 #define __assume(x) (x)
#endif

// Get file length using stdio (unused in this program - we use Win32 API instead)
static uint flen( FILE* f ) {
  // Seek to end of file
  fseek( f, 0, SEEK_END );
  // Get current position = file length
  uint len = ftell(f);
  // Restore position to beginning
  fseek( f, 0, SEEK_SET );
  return len;
}

// Platform detection macros - detect if compiling for 64-bit or 32-bit
#if defined(__x86_64) || defined(_M_X64)
 // 64-bit build: define X64 macro
 #define X64
 #define X64flag 1
#else
 // 32-bit build: undefine X64 macro
 #undef X64
 #define X64flag 0
#endif

// Restore previous packing state
#pragma pack(pop)

#endif // COMMON_H
