#ifndef HELPER_H
#define HELPER_H

#include <Arduino.h>


#define CLAMP(_V, _MIN, _MAX) (_V < _MIN ? _MIN : _V > _MAX ? _MAX : _V)

class RolingAcc {
public:
    RolingAcc() : size_(10) { reset(); }
    void reset();
    float avg(float v);

protected:
    float buffer_[10];
    float sum_;
    size_t size_;
    size_t fill_;
    size_t n_;
};

/************************************************************************\
|* String Helper Functions
\************************************************************************/

namespace StrTool {
  inline bool isWhiteSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
  inline bool isWhiteSpaceOrEnd(char c) { return c == '\0' || c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
  size_t readWord(const char* buffer, char* result, size_t size);
  size_t readBool(const char* buffer, bool* result, const char** errorStr=nullptr, bool arrayMode=false);
  size_t readInteger(const char* buffer, int* result, const char** errorStr=nullptr, bool arrayMode=false);
  size_t readFloat(const char* buffer, float* result, const char** errorStr=nullptr, bool arrayMode=false);
  size_t readBinary(const char* buffer, int* result, bool isNegative=false, const char** errorStr=nullptr, bool arrayMode=false);
  size_t readHex(const char* buffer, int* result, bool isNegative=false, const char** errorStr=nullptr, bool arrayMode=false);
  size_t readIpAddr(const char* buffer, uint8_t ip[4], const char** errorStr=nullptr);
  size_t tryRead(const char* str, const char* buffer);
  size_t toJsonName(char* buffer, size_t size, const char* name);
  size_t toCleanName(char* buffer, size_t size, const char* name);
  bool matchesCleanName(const char* jName, const char* name);
  size_t matchesNamePart(const char* pName, const char* name, bool matchCase=true);
  size_t formatBytes(char* buffer, size_t size, uint64_t bytes);
  size_t formatDurationSec(char* buffer, size_t size, int64_t duration);
  size_t formatDurationUs(char* buffer, size_t size, int64_t duration);
  size_t formatDurationMs(char* buffer, size_t size, int64_t duration);
  bool hexDigit(char c, uint32_t& out);
  size_t urlDecode(char* decoded, size_t size, const char* text);
  uint32_t calculateCrc(const char* data, size_t size);
  uint32_t calculateStrCrc(const char* data);
};



/************************************************************************\
|* Print Buffer
\************************************************************************/
// This allows funtions to printf() into a buffer without having to
// keep track of index and buffer size. Those functions can also take
// other output destinations like Stream (Serial, TCP) or chunked web
// responses.
class PrintBuffer : public Print {
public: 
  // New buffer on the heap
  PrintBuffer(size_t size) : buffer_(0), size_(size), index_(0), isExternalBuffer_(false) {
    buffer_ = new char[size_];
    buffer_[index_] = '\0';
  } 

  // Buffer is provided (e.g. stack)
  PrintBuffer(char* buffer, size_t size) : buffer_(buffer), size_(size), index_(0), isExternalBuffer_(true) { buffer_[index_] = '\0'; }

  ~PrintBuffer() {
    if (buffer_ && !isExternalBuffer_) {
      delete [] buffer_;
      buffer_ = nullptr;
    }
  }

  virtual size_t write(const uint8_t *buffer, size_t size) {
      size_t n = std::min(size, space());
      memcpy(buffer_ + index_, buffer, n);
      index_ += n;
      buffer_[index_] = '\0';
      return n;
  }

  inline virtual size_t write(uint8_t val) { if (index_ < size_-2) {buffer_[index_++] = val; buffer_[index_] = '\0'; return 1; } return 0; }
  using Print::write; // pull in write(str) and write(buf, size) from Print
  inline const char* c_str() { return buffer_; }
  inline size_t size() { return size_; }
  inline size_t length() { return index_; }
  inline size_t space() { return size_ - index_ - 1; }
  inline void clear() { index_ = 0; buffer_[index_] = '\0'; }
  inline void flush() { clear(); };
  inline bool isEmpty() { return index_ == 0; }
  inline bool isFull() { return index_ >= size_ -1; }

protected:
  char* buffer_;
  size_t size_;
  size_t index_;
  bool isExternalBuffer_;
};


#endif // HELPER_H