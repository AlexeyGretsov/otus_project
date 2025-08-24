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

void TextMessageJson::print() {
  std::cout << "type: " << type << std::endl;
  std::cout << "text: " << text << std::endl;
}

StatusMessageJson::StatusMessageJson() { type = "status"; }

void StatusMessageJson::print() {
  std::cout << "type: " << type << std::endl;
  std::cout << "message_id: " << message_id << std::endl;
  std::cout << "status: " << status << std::endl;
}

Message::Message() {}

Message::~Message() {
  if (json) {
    delete json;
  }
}

bool Message::fromJson(std::string_view source)
{
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

  std::cout << "id: " << id << std::endl;
  std::cout << "from: " << from << std::endl;
  std::cout << "to: " << to << std::endl;
  std::cout << "date: " << date << std::endl;

  json = createMessageJson(parsed_json);
  if (json) {
    json->print();
  }

  return true;
}

std::string Message::toJson() const {

  nlohmann::json msg_json;
  msg_json["message"]["id"] = boost::uuids::to_string(id);
  msg_json["message"]["from"] = boost::uuids::to_string(from);
  msg_json["message"]["to"] = boost::uuids::to_string(to);
  
  return msg_json.dump(4);
}
