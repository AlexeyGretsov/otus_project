#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

class TransferMessage {
public:
  static constexpr std::size_t headerLength = 4;
  static constexpr std::size_t MAX_BODY_LENGTH = 512;
  static constexpr std::size_t FULL_SIZE = headerLength + MAX_BODY_LENGTH;

  TransferMessage() : bodyLength(0) {
    data = new char[FULL_SIZE];
    std::memset(data, 0, FULL_SIZE);
  }
  TransferMessage(const std::string &source) {
    data = new char[FULL_SIZE];
    std::memset(data, 0, FULL_SIZE);
    setBodyLength(source.length());

    std::memcpy(getBody(), source.c_str(), getBodyLength());
    encodeHeader();
  }
  ~TransferMessage() { delete data; }
  TransferMessage(const TransferMessage &other) {
    data = new char[FULL_SIZE];
    bodyLength = other.bodyLength;
    std::memcpy(data, other.data, FULL_SIZE);
  }
  TransferMessage(TransferMessage &&other) {
    bodyLength = other.bodyLength;
    data = std::move(other.data);
  }
  TransferMessage &operator=(const TransferMessage &other) {
    if (this != &other) {
      bodyLength = other.bodyLength;
      std::memcpy(data, other.data, FULL_SIZE);
    }

    return *this;
  }
  TransferMessage &operator=(TransferMessage &&other) {
    if (this != &other) {
      bodyLength = other.bodyLength;
      data = std::move(other.data);
    }
    return *this;
  }

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
  char *data{nullptr};
  std::size_t bodyLength;
};
