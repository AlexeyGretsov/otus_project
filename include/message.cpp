#include "message.h"

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
  }

  return nullptr;
}
} // namespace

TextMessageJson::TextMessageJson() { type = "text"; }

std::string TextMessageJson::toString() const {
  nlohmann::json json;
  json["type"] = type;
  json["text"] = text;

  return json.dump();
}

StatusMessageJson::StatusMessageJson() { type = "status"; }

std::string StatusMessageJson::toString() const {
  nlohmann::json json;
  json["type"] = type;
  json["message_id"] = boost::uuids::to_string(message_id);
  json["status"] = status;

  return json.dump();
}

Message::Message() {}

Message::~Message() {
  if (json) {
    delete json;
  }
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
