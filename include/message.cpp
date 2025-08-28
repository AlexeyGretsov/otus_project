#include "message.h"

#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <iostream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace {
MessageJson *createMessageJson(nlohmann::json &parsed_json) {
  auto json = parsed_json["message"]["json"];
  std::string type(json["type"]);

  if (type == TextMessageJson{}.type) {
    TextMessageJson *result = new TextMessageJson();
    result->text = json["text"];

    return result;
  } else if (type == StatusMessageJson().type) {
    boost::uuids::string_generator gen;
    StatusMessageJson *result = new StatusMessageJson();
    result->message_id = gen(std::string(json["message_id"]));
    result->status = json["status"];

    return result;
  } else if (type == AuthMessageJson().type) {

    return new AuthMessageJson();
  }

  return nullptr;
}
} // namespace

const std::string Message::STATUS_PROCESSED = "processed";
const std::string Message::STATUS_RECEIVED = "received";

AuthMessageJson::AuthMessageJson() { type = "auth"; }

AuthMessageJson *AuthMessageJson::copy() const { return new AuthMessageJson(); }

std::string AuthMessageJson::toString() const {
  nlohmann::json json;
  json["type"] = type;

  return json.dump();
}

TextMessageJson::TextMessageJson() { type = "text"; }
TextMessageJson::TextMessageJson(std::string_view text) {
  type = "text";
  this->text = text;
}

TextMessageJson *TextMessageJson::copy() const {
  return new TextMessageJson(text);
}

std::string TextMessageJson::toString() const {
  nlohmann::json json;
  json["type"] = type;
  json["text"] = text;

  return json.dump();
}

StatusMessageJson::StatusMessageJson() { type = "status"; }
StatusMessageJson::StatusMessageJson(const boost::uuids::uuid message_id,
                                     std::string_view status) {
  type = "status";
  this->message_id = message_id;
  this->status = status;
}

StatusMessageJson *StatusMessageJson::copy() const {
  return new StatusMessageJson(message_id, status);
}

std::string StatusMessageJson::toString() const {
  nlohmann::json json;
  json["type"] = type;
  json["message_id"] = boost::uuids::to_string(message_id);
  json["status"] = status;

  return json.dump();
}

Message::Message() {
  boost::uuids::random_generator gen;
  boost::uuids::nil_generator nil_gen;
  from = to = nil_gen();
  id = gen();
  date = time(0);
}

Message::Message(const Message &other) {
  id = other.id;
  from = other.from;
  to = other.to;
  date = other.date;
  if (json) {
    delete json;
    json = nullptr;
  }
  if (other.json) {
    json = other.json->copy();
  }
}

Message::Message(Message &&other) {
  id = std::move(other.id);
  from = std::move(other.from);
  to = std::move(other.to);
  date = other.date;
  json = std::exchange(other.json, nullptr);
}

Message::~Message() {
  if (json) {
    delete json;
  }
}

Message &Message::operator=(const Message &other) {
  if (this != &other) {
    id = other.id;
    from = other.from;
    to = other.to;
    date = other.date;
    if (json) {
      delete json;
      json = nullptr;
    }
    if (other.json) {
      json = other.json->copy();
    }
  }

  return *this;
}

Message &Message::operator=(Message &&other) {
  if (this != &other) {
    id = std::move(other.id);
    from = std::move(other.from);
    to = std::move(other.to);
    date = other.date;
    json = std::exchange(other.json, nullptr);
  }
  return *this;
}

bool Message::isValid() const { return date != 0; }

bool Message::isAuth() const {
  return json and json->type == AuthMessageJson().type;
}

bool Message::fromJson(std::string_view source) {
  nlohmann::json parsed_json = nlohmann::json::parse(source);

  boost::uuids::string_generator gen;

  id = gen(std::string(parsed_json["message"]["id"]));
  from = gen(std::string(parsed_json["message"]["from"]));
  to = gen(std::string(parsed_json["message"]["to"]));

  std::string strDate = std::string(parsed_json["message"]["date"]);
  struct tm tmStruct = {};
  std::istringstream ss(strDate);
  ss >> std::get_time(&tmStruct, "%Y-%m-%dT%H:%M:%SZ");

  if (ss.fail()) {
    std::cerr << "Failed to parse date dormat: "
              << parsed_json["message"]["date"] << std::endl;
    return false;
  }

  date = mktime(&tmStruct);
  json = createMessageJson(parsed_json);

  return true;
}

std::string Message::toJson() const {

  nlohmann::json msg_json;
  msg_json["message"]["id"] = boost::uuids::to_string(id);
  msg_json["message"]["from"] = boost::uuids::to_string(from);
  msg_json["message"]["to"] = boost::uuids::to_string(to);

  char timeString[std::size("yyyy-mm-ddThh:mm:ssZ")];
  std::strftime(std::data(timeString), std::size(timeString), "%FT%TZ",
                std::gmtime(&date));
  msg_json["message"]["date"] = timeString;
  if (json) {
    msg_json["message"]["json"] = nlohmann::json::parse(json->toString());
  }

  return msg_json.dump(4);
}

TextMessage::TextMessage(const boost::uuids::uuid &from,
                         const boost::uuids::uuid &to, std::string_view text)
    : Message() {
  this->from = from;
  this->to = to;

  json = new TextMessageJson(text);
}

AuthMessage::AuthMessage(const boost::uuids::uuid &my) : Message() {
  from = my;
  json = new AuthMessageJson();
}

StatusMessage::StatusMessage(const boost::uuids::uuid clientUuid,
                             const boost::uuids::uuid message_id,
                             std::string_view status)
    : Message()

{
  to = clientUuid;
  json = new StatusMessageJson(message_id, status);
}