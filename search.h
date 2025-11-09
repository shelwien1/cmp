//### search.h
#ifndef SEARCH_H
#define SEARCH_H

// Base class with data members and public methods
template<uint CAPACITY>
struct Search0 {
  byte match_data[CAPACITY];
  byte mask_data[CAPACITY];
  uint length;

  uint InitPattern(const char* pattern);
  qword Find(const void* buffer, qword bufsize);

  uint GetLength() const { return length; }
  const byte* GetMatchData() const { return match_data; }
  const byte* GetMaskData() const { return mask_data; }
};

#endif
