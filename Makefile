# Makefile for cmp - Hex File Comparator
# Supports separate compilation of modules

# Compiler settings
CXX = g++
CXXFLAGS = -O2 -Wall
LDFLAGS = -lgdi32 -luser32 -ladvapi32 -lcomdlg32

# Target executable
TARGET = cmp.exe

# Object files (each module compiled separately)
OBJS = cmp.o \
       file_win.o \
       setfont.o \
       palette.o \
       textprint.o \
       hexdump.o \
       window.o \
       config.o

# Header dependencies
COMMON_HEADERS = common.h
FILE_WIN_HEADERS = $(COMMON_HEADERS) file_win.h
THREAD_HEADERS = $(COMMON_HEADERS) thread.h
BITMAP_HEADERS = $(COMMON_HEADERS) bitmap.h
SETFONT_HEADERS = $(COMMON_HEADERS) bitmap.h setfont.h
PALETTE_HEADERS = $(COMMON_HEADERS) palette.h
TEXTBLOCK_HEADERS = $(COMMON_HEADERS) setfont.h bitmap.h palette.h textblock.h
TEXTPRINT_HEADERS = $(COMMON_HEADERS) palette.h setfont.h bitmap.h textprint.h
HEXDUMP_HEADERS = $(COMMON_HEADERS) file_win.h textblock.h hexdump.h
WINDOW_HEADERS = $(COMMON_HEADERS) window.h
CONFIG_HEADERS = $(COMMON_HEADERS) config.h

# Default target
all: $(TARGET)

# Link all object files into executable
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compile main file
cmp.o: cmp.cpp $(COMMON_HEADERS) $(FILE_WIN_HEADERS) $(THREAD_HEADERS) $(BITMAP_HEADERS) \
       $(SETFONT_HEADERS) $(PALETTE_HEADERS) $(TEXTBLOCK_HEADERS) $(TEXTPRINT_HEADERS) \
       $(HEXDUMP_HEADERS) $(WINDOW_HEADERS) $(CONFIG_HEADERS)
	$(CXX) $(CXXFLAGS) -c cmp.cpp

# Compile file_win module
file_win.o: file_win.cpp $(FILE_WIN_HEADERS)
	$(CXX) $(CXXFLAGS) -c file_win.cpp

# Compile setfont module
setfont.o: setfont.cpp $(SETFONT_HEADERS)
	$(CXX) $(CXXFLAGS) -c setfont.cpp

# Compile palette module
palette.o: palette.cpp $(PALETTE_HEADERS)
	$(CXX) $(CXXFLAGS) -c palette.cpp

# Compile textprint module
textprint.o: textprint.cpp $(TEXTPRINT_HEADERS)
	$(CXX) $(CXXFLAGS) -c textprint.cpp

# Compile hexdump module
hexdump.o: hexdump.cpp $(HEXDUMP_HEADERS)
	$(CXX) $(CXXFLAGS) -c hexdump.cpp

# Compile window module
window.o: window.cpp $(WINDOW_HEADERS)
	$(CXX) $(CXXFLAGS) -c window.cpp

# Compile config module
config.o: config.cpp $(CONFIG_HEADERS) file_win.h
	$(CXX) $(CXXFLAGS) -c config.cpp

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild everything
rebuild: clean all

.PHONY: all clean rebuild
