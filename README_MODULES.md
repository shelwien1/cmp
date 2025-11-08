# CMP - Modularized Build Structure

This hex file comparator has been split into separately compiled modules for better organization and maintainability.

## Module Structure

The codebase is organized into the following modules:

### Core Modules
- **common.h** - Common types, templates, and utility functions
- **file_win.h/.cpp** - Windows file I/O wrapper functions
- **thread.h** - Thread wrapper template for Windows
- **bitmap.h** - Bitmap allocation and management
- **setfont.h/.cpp** - Font management and pre-rendering
- **palette.h/.cpp** - Color palette definitions
- **textblock.h** - Text buffer for character grid display
- **textprint.h/.cpp** - Character rendering with caching
- **hexdump.h/.cpp** - Hex file viewer with caching and difference highlighting
- **window.h/.cpp** - Window utilities
- **config.h/.cpp** - Configuration save/load (registry)

### Main Application
- **cmp.cpp** - Main application entry point and message loop

## Building

### Prerequisites
- MinGW-w64 or Visual Studio (Windows)
- GNU Make (for Makefile build)

### Compilation

Using GNU Make:
```bash
make
```

To clean build artifacts:
```bash
make clean
```

To rebuild everything:
```bash
make rebuild
```

### Build Process

The Makefile performs separate compilation:
1. Each .cpp file is compiled into a .o object file
2. All object files are linked together to create cmp.exe
3. Dependencies on header files ensure proper rebuilding when headers change

## Benefits of Modular Structure

1. **Faster Incremental Builds** - Only modified modules need recompilation
2. **Better Organization** - Related functionality is grouped into modules
3. **Easier Maintenance** - Changes to one module don't require understanding the entire codebase
4. **Clearer Dependencies** - Module boundaries make dependencies explicit
5. **Potential for Reuse** - Modules like file_win, thread, and bitmap could be reused in other projects

## Module Dependencies

```
cmp.cpp
├── common.h
├── file_win.h/.cpp
│   └── common.h
├── thread.h
│   └── common.h
├── bitmap.h
│   └── common.h
├── setfont.h/.cpp
│   ├── common.h
│   └── bitmap.h
├── palette.h/.cpp
│   └── common.h
├── textblock.h
│   ├── common.h
│   ├── setfont.h
│   ├── bitmap.h
│   └── palette.h
├── textprint.h/.cpp
│   ├── common.h
│   ├── palette.h
│   ├── setfont.h
│   └── bitmap.h
├── hexdump.h/.cpp
│   ├── common.h
│   ├── file_win.h
│   └── textblock.h
├── window.h/.cpp
│   └── common.h
└── config.h/.cpp
    ├── common.h
    └── file_win.h
```

## Original File

The original monolithic cmp.cpp has been backed up as cmp.cpp.bak for reference.
