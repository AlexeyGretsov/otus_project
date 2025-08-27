#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

class TransferMessage {
public:
  static constexpr std::size_t headerLength = 4;
  static constexpr std::size_t MAX_BODY_LENGTH = 512;

  TransferMessage() : bodyLength(0) {}

  const char *getData() const { return data; }

  char *getData() { return data; }

  std::size_t length() const { return headerLength + bodyLength; }

  const char *getBody() const { return data + headerLength; }

  char *getBody() { return data + headerLength; }

  std::size_t getBodyLength() const { return bodyLength; }

  void setBodyLength(std::size_t newLength) {
    bodyLength = newLength;
    if (bodyLength > MAX_BODY_LENGTH)
      bodyLength = MAX_BODY_LENGTH;
  }

  bool decodeHeader() {
    char header[headerLength + 1] = "";
    std::strncat(header, data, headerLength);
    bodyLength = std::atoi(header);
    if (bodyLength > MAX_BODY_LENGTH) {
      bodyLength = 0;
      return false;
    }
    return true;
  }

  void encodeHeader() {
    char header[headerLength + 1] = "";
    std::snprintf(header, headerLength + 1, "%4d",
                  static_cast<int>(bodyLength));
    std::memcpy(data, header, headerLength);
  }

private:
  char data[headerLength + MAX_BODY_LENGTH];
  std::size_t bodyLength;
};
