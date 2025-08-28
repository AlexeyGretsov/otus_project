#pragma once

#include <string>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

struct MessageJson {
  MessageJson() = default;
  virtual ~MessageJson() {}

  virtual std::string toString() const = 0;
  virtual MessageJson *copy() const = 0;

  std::string type;
};

struct AuthMessageJson : public MessageJson {
  AuthMessageJson();

  AuthMessageJson *copy() const override;
  std::string toString() const override;
};

struct TextMessageJson : public MessageJson {
  TextMessageJson();
  TextMessageJson(std::string_view text);

  TextMessageJson *copy() const override;
  std::string toString() const override;

  std::string text;
};

struct StatusMessageJson : public MessageJson {
  StatusMessageJson();
  StatusMessageJson(const boost::uuids::uuid message_id,
                    std::string_view status);

  StatusMessageJson *copy() const override;
  std::string toString() const override;

  boost::uuids::uuid message_id;
  std::string status;
};

struct Message {
  Message();
  Message(const Message &other);
  Message(Message &&other);
  ~Message();

  Message &operator=(const Message &other);
  Message &operator=(Message &&other);

  bool isValid() const;
  bool fromJson(std::string_view source);
  std::string toJson() const;

  boost::uuids::uuid id;
  boost::uuids::uuid from;
  boost::uuids::uuid to;
  time_t date{0};
  MessageJson *json{nullptr};
};

struct TextMessage : public Message {
  TextMessage(const boost::uuids::uuid &from, const boost::uuids::uuid &to,
              std::string_view text);
};

struct AuthMessage : public Message {
  AuthMessage(const boost::uuids::uuid &my);
};
