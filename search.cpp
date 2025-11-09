//### search.cpp
#include <cstring>

#include "common.h"
#include "search.h"

// Derived class with all methods
template<uint CAPACITY>
struct Search : Search0<CAPACITY> {
  using Search0<CAPACITY>::match_data;
  using Search0<CAPACITY>::mask_data;
  using Search0<CAPACITY>::length;
  
  template<uint CHECK_CAPACITY>
  void AppendByteImpl(byte value, byte mask) {
    if (CHECK_CAPACITY && length >= CAPACITY) {
      return;
    }
    match_data[length] = value;
    mask_data[length] = mask;
    length++;
  }
  
  template<uint CHECK_CAPACITY>
  void AppendWordImpl(word value, word mask) {
    if (CHECK_CAPACITY && length + 2 > CAPACITY) {
      return;
    }
    AppendByteImpl<0>((byte)(value & 0xFF), (byte)(mask & 0xFF));
    AppendByteImpl<0>((byte)((value >> 8) & 0xFF), (byte)((mask >> 8) & 0xFF));
  }
  
  template<uint CHECK_CAPACITY>
  void AppendUintImpl(uint value, uint mask) {
    if (CHECK_CAPACITY && length + 4 > CAPACITY) {
      return;
    }
    AppendWordImpl<0>((word)(value & 0xFFFF), (word)(mask & 0xFFFF));
    AppendWordImpl<0>((word)((value >> 16) & 0xFFFF), (word)((mask >> 16) & 0xFFFF));
  }
  
  template<uint CHECK_CAPACITY>
  void AppendQwordImpl(qword value, qword mask) {
    if (CHECK_CAPACITY && length + 8 > CAPACITY) {
      return;
    }
    AppendUintImpl<0>((uint)(value & 0xFFFFFFFF), (uint)(mask & 0xFFFFFFFF));
    AppendUintImpl<0>((uint)((value >> 32) & 0xFFFFFFFF), (uint)((mask >> 32) & 0xFFFFFFFF));
  }
  
  template<uint CHECK_CAPACITY>
  void AppendWordBEImpl(word value, word mask) {
    if (CHECK_CAPACITY && length + 2 > CAPACITY) {
      return;
    }
    AppendByteImpl<0>((byte)((value >> 8) & 0xFF), (byte)((mask >> 8) & 0xFF));
    AppendByteImpl<0>((byte)(value & 0xFF), (byte)(mask & 0xFF));
  }
  
  template<uint CHECK_CAPACITY>
  void AppendUintBEImpl(uint value, uint mask) {
    if (CHECK_CAPACITY && length + 4 > CAPACITY) {
      return;
    }
    AppendWordBEImpl<0>((word)((value >> 16) & 0xFFFF), (word)((mask >> 16) & 0xFFFF));
    AppendWordBEImpl<0>((word)(value & 0xFFFF), (word)(mask & 0xFFFF));
  }
  
  template<uint CHECK_CAPACITY>
  void AppendQwordBEImpl(qword value, qword mask) {
    if (CHECK_CAPACITY && length + 8 > CAPACITY) {
      return;
    }
    AppendUintBEImpl<0>((uint)((value >> 32) & 0xFFFFFFFF), (uint)((mask >> 32) & 0xFFFFFFFF));
    AppendUintBEImpl<0>((uint)(value & 0xFFFFFFFF), (uint)(mask & 0xFFFFFFFF));
  }
  
  void AppendByte(byte value, byte mask) {
    AppendByteImpl<1>(value, mask);
  }
  
  void AppendWord(word value, word mask) {
    AppendWordImpl<1>(value, mask);
  }
  
  void AppendUint(uint value, uint mask) {
    AppendUintImpl<1>(value, mask);
  }
  
  void AppendQword(qword value, qword mask) {
    AppendQwordImpl<1>(value, mask);
  }
  
  void AppendWordBE(word value, word mask) {
    AppendWordBEImpl<1>(value, mask);
  }
  
  void AppendUintBE(uint value, uint mask) {
    AppendUintBEImpl<1>(value, mask);
  }
  
  void AppendQwordBE(qword value, qword mask) {
    AppendQwordBEImpl<1>(value, mask);
  }
  
public:
  void Init() {
    uint i;
    
    length = 0;
    i = 0;
    while (i < CAPACITY) {
      match_data[i] = 0;
      mask_data[i] = 0;
      i++;
    }
  }
  
  uint InitPattern(const char* pattern) {
    Init();
    return ParsePattern(pattern);
  }
  
  void Quit() {
    length = 0;
  }
  
  uint GetLength() const { return length; }
  const byte* GetMatchData() const { return match_data; }
  const byte* GetMaskData() const { return mask_data; }
  
  qword Find(const void* buffer, qword bufsize) {
    const byte* buf;
    qword i;
    uint j;
    uint match;
    
    if (buffer == 0) {
      return (qword)(-1LL);
    }
    
    if (length == 0) {
      return 0;
    }
    
    if (bufsize < length) {
      return (qword)(-1LL);
    }
    
    buf = (const byte*)buffer;
    
    i = 0;
    while (i <= bufsize - length) {
      match = 1;
      j = 0;
      
      while (j < length) {
        if ((buf[i + j] & mask_data[j]) != (match_data[j] & mask_data[j])) {
          match = 0;
          break;
        }
        j++;
      }
      
      if (match) {
        return i;
      }
      
      i++;
    }
    
    return (qword)(-1LL);
  }
  
  uint Utf8ToUtf16(const char* utf8_str, uint utf8_len, word* utf16_out, uint utf16_max) {
    uint in_pos;
    uint out_pos;
    uint code_point;
    byte b1;
    byte b2;
    byte b3;
    byte b4;
    
    in_pos = 0;
    out_pos = 0;
    
    while (in_pos < utf8_len && out_pos < utf16_max) {
      b1 = (byte)utf8_str[in_pos];
      
      if ((b1 & 0x80) == 0) {
        // 1-byte sequence (ASCII)
        code_point = b1;
        in_pos++;
      } else if ((b1 & 0xE0) == 0xC0) {
        // 2-byte sequence
        if (in_pos + 1 >= utf8_len) {
          return 0;
        }
        b2 = (byte)utf8_str[in_pos + 1];
        code_point = ((b1 & 0x1F) << 6) | (b2 & 0x3F);
        in_pos += 2;
      } else if ((b1 & 0xF0) == 0xE0) {
        // 3-byte sequence
        if (in_pos + 2 >= utf8_len) {
          return 0;
        }
        b2 = (byte)utf8_str[in_pos + 1];
        b3 = (byte)utf8_str[in_pos + 2];
        code_point = ((b1 & 0x0F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        in_pos += 3;
      } else if ((b1 & 0xF8) == 0xF0) {
        // 4-byte sequence (surrogate pair in UTF-16)
        if (in_pos + 3 >= utf8_len) {
          return 0;
        }
        b2 = (byte)utf8_str[in_pos + 1];
        b3 = (byte)utf8_str[in_pos + 2];
        b4 = (byte)utf8_str[in_pos + 3];
        code_point = ((b1 & 0x07) << 18) | ((b2 & 0x3F) << 12) | ((b3 & 0x3F) << 6) | (b4 & 0x3F);
        
        // Convert to surrogate pair
        if (out_pos + 1 >= utf16_max) {
          return 0;
        }
        code_point -= 0x10000;
        utf16_out[out_pos++] = (word)(0xD800 + ((code_point >> 10) & 0x3FF));
        utf16_out[out_pos++] = (word)(0xDC00 + (code_point & 0x3FF));
        in_pos += 4;
        continue;
      } else {
        // Invalid UTF-8
        return 0;
      }
      
      // BMP character
      if (code_point > 0xFFFF) {
        return 0;
      }
      utf16_out[out_pos++] = (word)code_point;
    }
    
    return out_pos;
  }
  
  uint ParsePattern(const char* pattern) {
    const char* p;
    const char* token_start;
    uint token_len;
    uint in_string;
    
    p = pattern;
    token_start = p;
    token_len = 0;
    in_string = 0;
    
    while (*p != 0) {
      if (*p == '"') {
        in_string = !in_string;
        token_len++;
        p++;
        continue;
      }
      
      if (!in_string && *p == ',') {
        if (token_len > 0) {
          if (!ParseToken(token_start, token_len)) {
            return 0;
          }
        }
        p++;
        token_start = p;
        token_len = 0;
        continue;
      }
      
      token_len++;
      p++;
    }
    
    if (token_len > 0) {
      if (!ParseToken(token_start, token_len)) {
        return 0;
      }
    }
    
    return 1;
  }
  
  uint ParseToken(const char* token, uint token_len) {
    const char* start;
    const char* end;
    uint is_wide;
    uint is_big_endian;
    
    start = token;
    end = token + token_len;
    
    // Trim leading spaces
    while (start < end && (*start == ' ' || *start == '\t')) {
      start++;
    }
    
    // Trim trailing spaces
    while (end > start && (*(end - 1) == ' ' || *(end - 1) == '\t')) {
      end--;
    }
    
    if (start >= end) {
      return 1;
    }
    
    // Check if wide string literal (L"...")
    is_wide = 0;
    if (end - start >= 3 && (start[0] == 'L' || start[0] == 'l') && start[1] == '"') {
      is_wide = 1;
      start++;
    }
    
    // Check if string literal
    if (*start == '"') {
      return ParseStringLiteral(start, (uint)(end - start), is_wide);
    }
    
    // Check if big-endian hex number (0y...)
    is_big_endian = 0;
    if (end - start >= 2 && start[0] == '0' && (start[1] == 'y' || start[1] == 'Y')) {
      is_big_endian = 1;
    }
    
    // Check if hex number (0x... or 0y...)
    if (end - start >= 2 && start[0] == '0' && 
        (start[1] == 'x' || start[1] == 'X' || start[1] == 'y' || start[1] == 'Y')) {
      return ParseHexNumber(start, (uint)(end - start), is_big_endian);
    }
    
    // Otherwise decimal number or wildcard
    return ParseDecimalNumber(start, (uint)(end - start));
  }
  
  uint ParseStringLiteral(const char* str, uint len, uint is_wide) {
    uint i;
    byte ch;
    byte mask;
    word utf16_buf[256];
    uint utf16_len;
    uint j;
    
    if (len < 2 || str[0] != '"' || str[len - 1] != '"') {
      return 0;
    }
    
    if (!is_wide) {
      // Regular string - process as bytes
      i = 1;
      while (i < len - 1) {
        ch = (byte)str[i];
        mask = 0xFF;
        
        if (ch == '?') {
          ch = 0;
          mask = 0x00;
        }
        
        AppendByte(ch, mask);
        i++;
      }
    } else {
      // Wide string - convert UTF-8 to UTF-16
      utf16_len = Utf8ToUtf16(str + 1, len - 2, utf16_buf, 256);
      if (utf16_len == 0 && len > 2) {
        return 0;
      }
      
      // Append UTF-16 code units as little-endian words
      j = 0;
      while (j < utf16_len) {
        if (j < len - 2 && str[j + 1] == '?') {
          // Wildcard in wide string
          AppendWord(0, 0x0000);
        } else {
          AppendWord(utf16_buf[j], 0xFFFF);
        }
        j++;
      }
    }
    
    return 1;
  }
  
  uint ParseHexNumber(const char* str, uint len, uint is_big_endian) {
    const char* p;
    const char* end;
    char suffix[3];
    uint suffix_len;
    qword value;
    qword mask;
    uint digit_count;
    int hex_val;
    uint is_wildcard;
    
    p = str + 2; // Skip "0x" or "0y"
    end = str + len;
    
    // Check for suffix
    suffix_len = 0;
    if (end - p > 0) {
      char last_char;
      char second_last_char;
      
      last_char = *(end - 1);
      
      if (last_char == 'S' || last_char == 's') {
        suffix[suffix_len++] = last_char;
        end--;
      } else if (last_char == 'W' || last_char == 'w') {
        suffix[suffix_len++] = last_char;
        end--;
      } else if (last_char == 'L' || last_char == 'l') {
        suffix[suffix_len++] = last_char;
        end--;
        if (end - p > 0) {
          second_last_char = *(end - 1);
          if (second_last_char == 'L' || second_last_char == 'l') {
            suffix[suffix_len++] = second_last_char;
            end--;
          }
        }
      } else if (last_char == 'Q' || last_char == 'q') {
        suffix[suffix_len++] = last_char;
        end--;
      }
    }
    suffix[suffix_len] = 0;
    
    // Parse hex digits
    value = 0;
    mask = 0;
    digit_count = 0;
    
    while (p < end) {
      is_wildcard = 0;
      
      if (*p == '?') {
        hex_val = 0;
        is_wildcard = 1;
      } else if (*p >= '0' && *p <= '9') {
        hex_val = *p - '0';
      } else if (*p >= 'a' && *p <= 'f') {
        hex_val = 10 + (*p - 'a');
      } else if (*p >= 'A' && *p <= 'F') {
        hex_val = 10 + (*p - 'A');
      } else {
        return 0;
      }
      
      value = (value << 4) | hex_val;
      if (is_wildcard) {
        mask = (mask << 4);
      } else {
        mask = (mask << 4) | 0x0F;
      }
      digit_count++;
      p++;
    }
    
    // Determine size and endianness
    if (suffix_len == 1 && (suffix[0] == 'S' || suffix[0] == 's')) {
      AppendByte((byte)value, (byte)mask);
    } else if (suffix_len == 1 && (suffix[0] == 'W' || suffix[0] == 'w')) {
      if (is_big_endian) {
        AppendWordBE((word)value, (word)mask);
      } else {
        AppendWord((word)value, (word)mask);
      }
    } else if (suffix_len == 1 && (suffix[0] == 'L' || suffix[0] == 'l')) {
      if (is_big_endian) {
        AppendUintBE((uint)value, (uint)mask);
      } else {
        AppendUint((uint)value, (uint)mask);
      }
    } else if (suffix_len == 2 || (suffix_len == 1 && (suffix[0] == 'Q' || suffix[0] == 'q'))) {
      if (is_big_endian) {
        AppendQwordBE(value, mask);
      } else {
        AppendQword(value, mask);
      }
    } else {
      // No suffix, use minimum size
      if (value <= 0xFF && digit_count <= 2) {
        AppendByte((byte)value, (byte)mask);
      } else if (value <= 0xFFFF && digit_count <= 4) {
        if (is_big_endian) {
          AppendWordBE((word)value, (word)mask);
        } else {
          AppendWord((word)value, (word)mask);
        }
      } else if (value <= 0xFFFFFFFF && digit_count <= 8) {
        if (is_big_endian) {
          AppendUintBE((uint)value, (uint)mask);
        } else {
          AppendUint((uint)value, (uint)mask);
        }
      } else {
        if (is_big_endian) {
          AppendQwordBE(value, mask);
        } else {
          AppendQword(value, mask);
        }
      }
    }
    
    return 1;
  }
  
  uint ParseDecimalNumber(const char* str, uint len) {
    const char* p;
    const char* end;
    char suffix[3];
    uint suffix_len;
    qword value;
    uint is_wildcard;
    
    p = str;
    end = str + len;
    
    // Check if entire token is just "?"
    if (len == 1 && *str == '?') {
      AppendByte(0, 0x00);
      return 1;
    }
    
    // Check for suffix
    suffix_len = 0;
    if (end - p > 0) {
      char last_char;
      char second_last_char;
      
      last_char = *(end - 1);
      
      if (last_char == 'S' || last_char == 's') {
        suffix[suffix_len++] = last_char;
        end--;
      } else if (last_char == 'W' || last_char == 'w') {
        suffix[suffix_len++] = last_char;
        end--;
      } else if (last_char == 'L' || last_char == 'l') {
        suffix[suffix_len++] = last_char;
        end--;
        if (end - p > 0) {
          second_last_char = *(end - 1);
          if (second_last_char == 'L' || second_last_char == 'l') {
            suffix[suffix_len++] = second_last_char;
            end--;
          }
        }
      } else if (last_char == 'Q' || last_char == 'q') {
        suffix[suffix_len++] = last_char;
        end--;
      }
    }
    suffix[suffix_len] = 0;
    
    // Check if wildcard with suffix
    is_wildcard = 0;
    if (end - p == 1 && *p == '?') {
      is_wildcard = 1;
      value = 0;
    } else {
      // Parse decimal number
      value = 0;
      while (p < end) {
        if (*p >= '0' && *p <= '9') {
          value = value * 10 + (*p - '0');
        } else {
          return 0;
        }
        p++;
      }
    }
    
    // Append based on suffix or value size
    if (suffix_len == 1 && (suffix[0] == 'S' || suffix[0] == 's')) {
      AppendByte((byte)value, is_wildcard ? 0x00 : 0xFF);
    } else if (suffix_len == 1 && (suffix[0] == 'W' || suffix[0] == 'w')) {
      AppendWord((word)value, is_wildcard ? 0x0000 : 0xFFFF);
    } else if (suffix_len == 1 && (suffix[0] == 'L' || suffix[0] == 'l')) {
      AppendUint((uint)value, is_wildcard ? 0x00000000 : 0xFFFFFFFF);
    } else if (suffix_len == 2 || (suffix_len == 1 && (suffix[0] == 'Q' || suffix[0] == 'q'))) {
      AppendQword(value, is_wildcard ? 0x0000000000000000LL : 0xFFFFFFFFFFFFFFFFLL);
    } else {
      // No suffix, use minimum size to fit value
      if (value <= 0xFF) {
        AppendByte((byte)value, 0xFF);
      } else if (value <= 0xFFFF) {
        AppendWord((word)value, 0xFFFF);
      } else if (value <= 0xFFFFFFFF) {
        AppendUint((uint)value, 0xFFFFFFFF);
      } else {
        AppendQword(value, 0xFFFFFFFFFFFFFFFFLL);
      }
    }
    
    return 1;
  }
};

// Search0 method implementations - cast to Search and delegate
template<uint CAPACITY>
uint Search0<CAPACITY>::InitPattern(const char* pattern) {
  Search<CAPACITY>* s;
  
  if (pattern == 0) {
    return 0;
  }
  s = static_cast<Search<CAPACITY>*>(this);
  return s->InitPattern(pattern);
}

template<uint CAPACITY>
qword Search0<CAPACITY>::Find(const void* buffer, qword bufsize) {
  Search<CAPACITY>* s;
  
  s = static_cast<Search<CAPACITY>*>(this);
  return s->Find(buffer, bufsize);
}

// Explicit template instantiations for Search0 methods
template class Search0<64>;
template class Search0<128>;
template class Search0<256>;
template class Search0<512>;
template class Search0<1024>;
