// Configuration implementation
#include "config.h"
#include "file_win.h"

// Default configuration: Consolas font, 32 bytes/line, no selection, 32-bit addresses, no help, no terminal, combined mode
viewstate lf = { {-19,-10, 0, 0, 400, 0, 0, 0, 204, 3, 2, 1, 49, "Consolas"}, 32,255, -1, 0, 0, 0, 0 };
viewstate lf_old;  // Backup of old config (to detect changes)

// Save configuration to registry (HKCU\Software\SRC\cmp_01\config)
void SaveConfig( ) {
  uint r,disp;
  HKEY hk;
  // Create or open registry key
  r = RegCreateKeyEx(
    HKEY_CURRENT_USER,           // Root key: current user's settings
    "Software\\SRC\\cmp_01", 0, 0,  // Subkey path
    REG_OPTION_NON_VOLATILE,     // Persist across reboots
    KEY_ALL_ACCESS,              // Full access rights
    0,                           // Default security
    &hk, (LPDWORD)&disp          // Returns handle and disposition
  );
  if( r==ERROR_SUCCESS ) {
    // Write entire viewstate structure as binary blob
    r = RegSetValueEx( hk, "config", 0, REG_BINARY, (byte*)&lf, sizeof(lf) );
    RegCloseKey( hk );
  } else {
    SetLastError(r);
    printf( "r=%X disp=%X\nError=<%s>\n", r, disp, GetErrorText() );
  }
}

// Load configuration from registry
void LoadConfig( ) {
  uint r,l=sizeof(lf);  // Expected data size
  HKEY hk;
  // Open registry key for reading
  r = RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\SRC\\cmp_01", 0, KEY_READ, &hk );
  if( r==ERROR_SUCCESS ) {
    // Read binary blob into viewstate structure
    r = RegQueryValueEx( hk, "config", 0, 0, (byte*)&lf, (LPDWORD)&l );
    RegCloseKey( hk );
  } else {
    SetLastError(r);
    printf( "r=%X\nConfig Error=<%s>\n", r, GetErrorText() );
  }
}
