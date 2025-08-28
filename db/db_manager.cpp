#include "db/db_manager.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/uuid/string_generator.hpp>
#include <iostream>

DbManager::DbManager(std::shared_ptr<Db::IDbConnection> dbConn)
    : dbConn(dbConn) {}

bool DbManager::saveMessage(const Message &message) {
  if (not dbConn->isConnected()) {
    std::cerr << "[DbManager::saveMessage] Unable to save message: "
                 "has not DB connection"
              << std::endl;
    return false;
  }

  std::string json = message.toJson();
  std::string query = R"(
        insert into messages (
                id, 
                from_user_id,
                to_user_id,
                accepted,
                message
            ) values (
                '{id}', 
                '{from_user_id}',
                '{to_user_id}',
                now(),
                '{message}'
            )
        )";
  boost::replace_all(query, "{id}", boost::uuids::to_string(message.id));
  boost::replace_all(query, "{from_user_id}",
                     boost::uuids::to_string(message.from));
  boost::replace_all(query, "{to_user_id}",
                     boost::uuids::to_string(message.to));
  boost::replace_all(query, "{message}", json);

  return dbConn->insert(query);
}

Message DbManager::loadMessage(boost::uuids::uuid messageId) {
  if (not dbConn->isConnected()) {
    std::cerr << "[DbManager::loadMessage] Unable to load message for id: "
              << messageId << std::endl;

    return Message();
  }

  std::string query = R"(
        select 
            message
        from 
            messages 
        where
            id = '{id}'
        )";
  boost::replace_all(query, "{id}", boost::uuids::to_string(messageId));

  std::vector<std::vector<std::string>> data = dbConn->select(query);

  Message msg;
  if (data.empty() or data.front().empty()) {
    return msg;
  }

  msg.fromJson(data.front().front());

  return msg;
}

bool DbManager::deleteMessage(boost::uuids::uuid messageId) {
  if (not dbConn->isConnected()) {
    std::cerr << "[DbManager::deleteMessage] Unable to delete message for id: "
              << messageId << std::endl;

    return false;
  }

  std::string query = R"(
        delete from messages 
        where id = '{id}')";
  boost::replace_all(query, "{id}", boost::uuids::to_string(messageId));

  return dbConn->del(query);
}

bool DbManager::saveProcessedMessage(boost::uuids::uuid clientId, 
                                    boost::uuids::uuid messageId) {
  if (not dbConn->isConnected()) {
    std::cerr << "[DbManager::saveProcessedMessage] Unable to save processed "
                 "message: "
                 "has not DB connection"
              << std::endl;
    return false;
  }

  std::string query = R"(
        insert into processed_messages (
                registered,
                to_user_id,
                message_id
            ) values (
                now(),
                '{to_user_id}',
                '{id}'
            )
        )";
  boost::replace_all(query, "{to_user_id}", boost::uuids::to_string(clientId));
  boost::replace_all(query, "{id}", boost::uuids::to_string(messageId));

  return dbConn->insert(query);
}

std::vector<boost::uuids::uuid> DbManager::loadProcessedMessages(
        boost::uuids::uuid clientId) {
  std::vector<boost::uuids::uuid> result;

  if (not dbConn->isConnected()) {
    std::cerr << "[DbManager::loadProcessedMessages] Unable to load processed "
                 "messages: "
                 "has not DB connection"
              << std::endl;

    return result;
  }

  std::string query = R"(
        select 
            message_id
        from 
            processed_messages
        where
            to_user_id = '{to_user_id}'
        order by registered
        )";

  boost::replace_all(query, "{to_user_id}", boost::uuids::to_string(clientId));
  
  std::vector<std::vector<std::string>> data = dbConn->select(query);

  boost::uuids::string_generator gen;
  for (auto row : data) {
    if (not row.empty()) {
      result.push_back(gen(row.front()));
    }
  }

  return result;
}

bool DbManager::deleteProcessedMessage(boost::uuids::uuid messageId) {
  if (not dbConn->isConnected()) {
    std::cerr << "[DbManager::deleteProcessedMessage] Unable to delete "
                 "processed message for id: "
              << messageId << std::endl;

    return false;
  }

  std::string query = R"(
        delete from processed_messages 
        where message_id = '{id}')";
  boost::replace_all(query, "{id}", boost::uuids::to_string(messageId));

  return dbConn->del(query);
}
