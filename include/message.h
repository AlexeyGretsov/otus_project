#pragma once

#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

struct MessageJson {
  MessageJson() = default;
  virtual ~MessageJson() {}

  virtual std::string toString() const = 0;

  std::string type;
};

struct TextMessageJson : public MessageJson {
  TextMessageJson();

  std::string toString() const override;

  std::string text;
};

struct StatusMessageJson : public MessageJson {
  StatusMessageJson();

  std::string toString() const override;

  boost::uuids::uuid message_id;
  std::string status;
};

struct Message {
  Message();
  ~Message();

  bool fromJson(std::string_view source);
  std::string toJson() const;

  boost::uuids::uuid id;
  boost::uuids::uuid from;
  boost::uuids::uuid to;
  time_t date{0};
  MessageJson *json{nullptr};
};
