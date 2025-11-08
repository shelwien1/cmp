// Character rendering cache implementation
#include "textprint.h"

// Character rendering cache - stores pointers to last rendered positions
// This is a key optimization: if we already rendered char X with palette Y, just copy from previous location
uint* cache_ptr[pal_MAX][256];  // [palette index][character] -> pointer to previously rendered pixels

// Clear the character rendering cache (called before each frame)
void SymbOut_Init( void ) {
  bzero( cache_ptr );  // Set all cache pointers to NULL
}
