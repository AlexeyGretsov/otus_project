#pragma once

#include <memory>
#include <vector>

#include "db/i_db_connection.h"
#include "include/message.h"

class DbManager {
public:
  DbManager(std::shared_ptr<Db::IDbConnection> dbConn);

  bool saveMessage(const Message &message);
  Message loadMessage(boost::uuids::uuid messageId);
  bool deleteMessage(boost::uuids::uuid messageId);

  bool saveProcessedMessage(boost::uuids::uuid clientId, boost::uuids::uuid messageId);
  std::vector<boost::uuids::uuid> loadProcessedMessages(boost::uuids::uuid clientId);
  bool deleteProcessedMessage(boost::uuids::uuid messageId);

private:
  std::shared_ptr<Db::IDbConnection> dbConn;
};
