#include <Arduino.h>
#include "VUEF.h"
#include "Helper.h"

void RolingAcc::reset() {
  for (size_t i=0; i<size_; i++) { buffer_[i] = 0.0; }
  sum_ = 0.0;
  fill_ = 0;
  n_ = 0;
}

float RolingAcc::avg(float v) {
  sum_ += v;
  sum_ -= buffer_[n_];
  buffer_[n_++] = v;
  if (n_ >= size_) { n_ = 0; }
  if (fill_ < size_) { fill_++; }
  return sum_ / fill_;
}

/************************************************************************\
|* String Helper Functions
\************************************************************************/
namespace StrTool {
size_t readBool(const char* buffer, bool* result, const char** errorStr/*=nullptr*/, bool arrayMode/*=false*/) {
  size_t n = 0; 
  if ((n = tryRead(FST("0"), buffer))) { *result = false; return n; }
  if ((n = tryRead(FST("false"), buffer))) { *result = false; return n; }
  if ((n = tryRead(FST("off"), buffer))) { *result = false; return n; }
  if ((n = tryRead(FST("1"), buffer))) { *result = true; return n; }
  if ((n = tryRead(FST("true"), buffer))) { *result = true; return n; }
  if ((n = tryRead(FST("on"), buffer))) { *result = true; return n; }
  if (errorStr) { *errorStr = FST("invalid boolean"); }
  return 0;
}

size_t readInteger(const char* buffer, int* result, const char** errorStr/*=nullptr*/, bool arrayMode/*=false*/) {
  size_t n = 0;
  int tmp = 0;
  bool is_negative = false;
  if (buffer[n] == '-') {
    is_negative = true;
    n++;
  }
  if (buffer[n] == '0' && buffer[n+1] == 'b') { return readBinary(buffer+n+2, result, is_negative, errorStr, arrayMode); }
  if (buffer[n] == '0' && buffer[n+1] == 'x') { return readHex(buffer+n+2, result, is_negative, errorStr, arrayMode); }

  while(!isWhiteSpaceOrEnd(buffer[n])) {
    char c = buffer[n]; 
    if (arrayMode && (c == ',' || c == ']')) break;
    if (c < '0' || c > '9') { 
      if (errorStr) { *errorStr = FST("invalid integer"); }
      return 0; 
    }
    tmp = tmp * 10 + (c - '0');
    n++;
  }
  if (is_negative) { tmp = -tmp; }
  while (isWhiteSpace(buffer[n])) { n++; } 
  *result = tmp;
  return n;
}

size_t readFloat(const char* buffer, float* result, const char** errorStr/*=nullptr*/, bool arrayMode/*=false*/) {
  size_t n = 0;
  float tmp = 0;
  float dec = 0.0;
  bool is_negative = false;
  if (buffer[n] == '-') {
    is_negative = true;
    n++;
  }

  while(!isWhiteSpaceOrEnd(buffer[n])) {
    char c = buffer[n]; 
    if (arrayMode && (c == ',' || c == ']')) break;
    if (c == '.') { 
      if (dec) {
        if (errorStr) { *errorStr = FST("invalid float"); }
        return 0;  
      }
      dec = 0.1;
    } else if (c < '0' || c > '9') { 
      if (errorStr) { *errorStr = FST("invalid float"); }
      return 0; 
    } else {
      if (dec == 0) { tmp = tmp * 10.0 + (c - '0'); }
      else { 
        tmp += (float)(c - '0') * dec; 
        dec /= 10.0; 
      }
    }  
    n++;
  }
  if (is_negative) { tmp = -tmp; }
  while (isWhiteSpace(buffer[n])) { n++; } 
  *result = tmp;
  return n;
}

size_t readBinary(const char* buffer, int* result, bool is_negative, const char** errorStr/*=nullptr*/, bool arrayMode/*=false*/) {
  size_t n = 0;
  int tmp = 0;
  if (!is_negative && buffer[n] == '-') {
    is_negative = true;
    n++;
  }
  while(!isWhiteSpaceOrEnd(buffer[n])) {
    char c = buffer[n]; 
    if (arrayMode && (c == ',' || c == ']')) break;
    if (c < '0' || c > '1') { 
      if (errorStr) { *errorStr = FST("invalid binary"); }
      return 0; 
    }
    tmp = (tmp << 1) + (c - '0');
    n++;
  }
  if (is_negative) { tmp = -tmp; }
  while (isWhiteSpace(buffer[n])) { n++; } 
  *result = tmp;
  return n;
}

size_t readHex(const char* buffer, int* result, bool is_negative, const char** errorStr/*=nullptr*/, bool arrayMode/*=false*/) {
  size_t n = 0;
  int tmp = 0;
  if (!is_negative && buffer[n] == '-') {
    is_negative = true;
    n++;
  }
  while(!isWhiteSpaceOrEnd(buffer[n])) {
    char c = toupper(buffer[n]);
    if (arrayMode && (c == ',' || c == ']')) break;
    if (c >= '0' && c <= '9') { 
      tmp = (tmp << 4) + (c - '0');    
    } else if (c >= 'A' && c <= 'F') {
      tmp = (tmp << 4) + (c - 'A' + 10);    
    } else {
      if (errorStr) { *errorStr = FST("invalid hex"); }
      return 0; 
    }
    n++;
  }
  if (is_negative) { tmp = -tmp; }
  while (isWhiteSpace(buffer[n])) { n++; } 
  *result = tmp;
  return n;
}

size_t readWord(const char* buffer, char* result, size_t size) {
  size_t n = 0;
  size_t i = 0;
  while (isWhiteSpace(buffer[n])) { n++; }
  while(!isWhiteSpaceOrEnd(buffer[n])) {
    if (i < (size-1)) { result[i++] = buffer[n]; }
    n++;
  }
  result[i] = '\0';
  while (isWhiteSpace(buffer[n])) { n++; } 
  return n;
}

size_t readIpAddr(const char* buffer, uint8_t ip[4], const char** errorStr/*=nullptr*/) {
  size_t n = 0;
  size_t i = 0;
  while(buffer[n] && i<4) {
    int tmp = atoi(&buffer[n]);
    if (tmp > 255 || tmp < -1) {
      if (errorStr) { *errorStr = FST("invalid ip address"); }
      return 0;
    }
    ip[i++] = tmp;
    while(buffer[n] == ' ' || (buffer[n] >= '0' && buffer[n] <= '9')) { n++; }
    if (buffer[n] == 0) { break; }
    if (buffer[n] == '.') { n++; }
    else {
      if (errorStr) { *errorStr = FST("invalid ip address"); }
      return 0;
    }
  }
  if (i != 4) {
    if (errorStr) { *errorStr = FST("too short ip address"); }
    return 0;
  }
  return n;
}

size_t tryRead(const char* str, const char* buffer) {
  size_t n = 0;
  while (true) {
    if (str[n] == '\0') {
      if (!isWhiteSpaceOrEnd(buffer[n])) {
        return 0;
      }
      while (isWhiteSpace(buffer[n])) { n++; } 
      return n;
    }
    if (tolower(buffer[n]) != tolower(str[n])) { return 0; }    
    n++;
  }
}

size_t toCleanName(char* buffer, size_t size, const char* name) {
  size_t n = 0;
  while (n < size-1 && *name) {
    char c = *name++;
    if (c == ' ') { c = '_'; }
    buffer[n] = c;    
    n++;
  }
  buffer[n] = '\0';
  return n;
}

size_t toJsonName(char* buffer, size_t size, const char* name) {
  size_t n = 0;
  if (n < size-1) { buffer[n++] = '"'; }
  n += toCleanName(buffer+n, size-n, name);
  if (n < size-1) { buffer[n++] = '"'; }
  if (n < size-1) { buffer[n++] = ':'; }
  buffer[n] = '\0';
  return n;
}

bool matchesCleanName(const char* jName, const char* name) {
  while (*jName && *name) {
    if (*jName != *name) {  
      if (*jName != '_' || *name != ' ') { return false; }
    }
    jName++; 
    name++;
  }
  if (*name != '\0' || *jName != '\0') { return false; }
  return true;
}


// Checks if the beginning of the pName matches name.
size_t matchesNamePart(const char* pName, const char* name, bool matchCase/*=true*/) {
  size_t n = 0;
  while (*pName && *name) {
    char pc = *pName;
    char c = *name;
    if (!matchCase) { pc = tolower(pc); c = tolower(c); }
    if (pc != c) {  
      if (pc != '_' || c != ' ') { return 0; }
    }
    pName++; 
    name++;
    n++;
  }
  if (*name != '\0' || (*pName != '\0' && *pName != ' ' && *pName != '.')) { return 0; }
  return n;
}

size_t formatBytes(char* buffer, size_t size, uint64_t bytes) {
    if (bytes < 1024) {
        return snprintf(buffer, size-1, FST("%d B"), (uint32_t)bytes);
    }
    float b = bytes;
    b /= 1024;
    if (b < 1024) {
        return snprintf(buffer, size-1, FST("%.2f KB"), b);
    }
    b /= 1024;
    if (b < 1024) {
        return snprintf(buffer, size-1, FST("%.2f MB"), b);
    }
    b /= 1024;
    return snprintf(buffer, size-1, FST("%.2f GB"), b);
}

size_t formatDurationSec(char* buffer, size_t size, int64_t duration) {
    size_t n = 0;
    if (duration < 0) { 
      if (n < size-2) { buffer[n++] = '-'; }
      duration = -duration; 
    }
    uint8_t sec = duration % 60;
    duration /= 60;
    uint8_t min = duration % 60;
    duration /= 60;
    uint8_t h = duration % 24;
    duration /= 24;
    if (duration) { n += snprintf(buffer+n, size-n-1, FST("%dd "), (uint32_t)duration); }
    if (h) { n += snprintf(buffer+n, size-n-1, FST("%d:"), h); }
    if (min) { n += snprintf(buffer+n, size-n-1, FST("%02d:"), min); }
    if (!n  || (n==1 && buffer[0] == '-') || (!min && !h && !duration)) { 
      n += snprintf(buffer+n, size-n-1, FST("%d"), sec); 
    }
    else { n += snprintf(buffer+n, size-n-1, FST("%02d"), sec); }
    buffer[n] = '\0';
    return n;
}

size_t formatDurationUs(char* buffer, size_t size, int64_t duration) {
    size_t n = 0;
    if (duration < 0) { 
      if (n < size-2) { buffer[n++] = '-'; }
      duration = -duration; 
    }
    uint32_t us = duration % 1000000;
    duration /= 1000000;
    size_t m = formatDurationSec(buffer+n, size-n, duration);
    if (m>1 || buffer[n] != '0') {
      n += m;
      n += snprintf(buffer+n, size-n-1, FST(".%06d"), us);
    } else {
      n += snprintf(buffer+n, size-n-1, FST("%d"), us);
    }
    buffer[n] = '\0';
    return n;
}

size_t formatDurationMs(char* buffer, size_t size, int64_t duration) {
    size_t n = 0;
    if (duration < 0) { 
      if (n < size-2) { buffer[n++] = '-'; }
      duration = -duration; 
    }
    uint32_t ms = duration % 1000;
    duration /= 1000;
    size_t m = formatDurationSec(buffer+n, size-n, duration);
    if (m>1 || buffer[n] != '0') {
      n += m;
      n += snprintf(buffer+n, size-n-1, FST(".%03d"), ms);
    } else {
      n += snprintf(buffer+n, size-n-1, FST("%d"), ms);
    }
    buffer[n] = '\0';
    return n;
}

uint32_t calculateCrc(const char* data, size_t size) {
  uint32_t* ptr = (uint32_t*) data;
  uint32_t crc = ~size;
  size_t n = 0;
  while (n <= (size-4)) {
    crc += *ptr++;
    n += 4;
  }
  while (n < size) {
    crc += data[n++];
  }
  return crc;
}

uint32_t calculateStrCrc(const char* data) {
  uint32_t crc = -1;
  uint32_t tmp = 0;
  size_t n = 0;
  while (data[n]) {
    tmp = tmp << 8;
    tmp |= data[n++];
    crc += 0x08040201;
    if ((n&3) == 0) { crc += tmp; }
  }
  if (n&3) { crc += tmp; }
  return crc;
}

bool hexDigit(char c, uint32_t& out) {
  c = toupper(c);
  if (c >= '0' && c <= '9') { 
    out = (out << 4) + (c - '0');    
  } else if (c >= 'A' && c <= 'F') {
    out = (out << 4) + (c - 'A' + 10);
  } else {
    return true;
  }
  return false;
}

size_t urlDecode(char* decoded, size_t size, const char* text) {
	size_t n = 0;
	while (*text && n < size-1) {
		char decodedChar;
		char encodedChar = *text++;
		if ((encodedChar == '%') && (text[0] && text[1])) {
      uint32_t tmp = 0;
      hexDigit(*text++, tmp);
      hexDigit(*text++, tmp);
			decodedChar = tmp;
		} else {
			if (encodedChar == '+') { decodedChar = ' '; }
			else { decodedChar = encodedChar; } // Normal ASCII
		}
		decoded[n++] = decodedChar;
	}
  decoded[n] = '\0';
	return n;
}


bool parseIpAddr(uint8_t ip[4], const char* str) {
  int i = 0;
  while(*str && i<4) {
    ip[i++] = atoi(str);
    while(*str == ' ' || (*str >= '0' && *str <= '9')) { str++; }
    if (*str == 0) { break; }
    if (*str == '.') { str++; }
    else {
      DEBUG_print(F("Bad IP address character: ")); 
      DEBUG_println(*str); 
      return true;
    }
  }
  return false;
}

};
