#include <iostream>
#include <memory>
#include <regex>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "db/db_manager.h"
#include "db/pg_db_connection.h"
#include "include/message.h"

int main(int argc, char *argv[]) {
  boost::uuids::random_generator gen;

  /*
   * DB test
   */
#if 0
  std::string dbHost = "localhost";
  int dbPort = 5432;
  std::string dbName = "otus_messendger";
  std::string dbUser = "postgres";
  std::string dbPass = "postgres";

  std::shared_ptr<Db::IDbConnection> dbConn(new Db::PgDbConnection());
  if (not dbConn->connect(dbHost, dbPort, dbName, dbUser, dbPass)) {
    return EXIT_FAILURE;
  }

  auto selectUsers = [=]() {
    std::string query = "select * from users";
    auto usersInfo = dbConn->select(query);

    std::cout << "Users count " << usersInfo.size() << std::endl;
    for (auto row : usersInfo) {
      for (auto val : row) {
        std::cout << val << " ";
      }
      std::cout << std::endl;
    }
  };

  selectUsers();

  boost::uuids::uuid u = gen();

  std::string query =
      "insert into users (ID, name) values ('{uuid}', 'user 555')";
  boost::replace_all(query, "{uuid}", boost::uuids::to_string(u));

  if (not dbConn->insert(query)) {
    return EXIT_FAILURE;
  }

  selectUsers();

  query = "delete from users where name = 'user 555'";
  if (not dbConn->del(query)) {
    return EXIT_FAILURE;
  }

  selectUsers();
#endif

  /*
   *JSON test
   */
#if 0
  std::string json = R"(
  {
	    "message" : {
		    "id" : "cbc69714-cb4d-437b-824e-c89750dd2699",
		    "from" : "fd0c7575-e4d7-4590-b0e7-c32481f75d27",
		    "to" : "4cc7b2e7-f11a-47a3-ab35-c9d4e5f6364d",
	  	  "date" : "2025-08-23T12:13:14Z",
	  	  "json" : {
	  		  "type" : "text",
	  		  "text" : "Привет"
	  	  }
      }
  	}
  )";

  Message msg;
  msg.fromJson(json);

  std::string json2 = R"(
  {
	    "message" : {
		    "id" : "cbc69714-cb4d-437b-824e-c89750dd2699",
		    "from" : "fd0c7575-e4d7-4590-b0e7-c32481f75d27",
		    "to" : "4cc7b2e7-f11a-47a3-ab35-c9d4e5f6364d",
	  	  "date" : "2025-08-23T12:13:14Z",
	  	  "json" : {
	  		  "type" : "status",
	  		  "message_id" : "aaa32efd-8a90-4225-98a5-82f4f9b9fa8d",
          "status" : "received"
	  	  }
      }
  	}
  )";

  Message msg2;
  msg2.fromJson(json2);

  std::cout << msg.toJson() << std::endl;
  std::cout << msg2.toJson() << std::endl;

  Message msg3;
  msg3.fromJson(msg2.toJson());

  std::cout << "Msg3: " << msg3.toJson() << std::endl;
#endif

  /*
   * DB manager tests
   */
  std::string dbHost = "localhost";
  int dbPort = 5432;
  std::string dbName = "otus_messendger";
  std::string dbUser = "postgres";
  std::string dbPass = "postgres";

  std::shared_ptr<Db::IDbConnection> dbConn(new Db::PgDbConnection());
  if (not dbConn->connect(dbHost, dbPort, dbName, dbUser, dbPass)) {
    return EXIT_FAILURE;
  }

  DbManager dbManager(dbConn);

  std::string json = R"(
  {
	    "message" : {
		    "id" : "cbc69714-cb4d-437b-824e-c89750dd2699",
		    "from" : "fd0c7575-e4d7-4590-b0e7-c32481f75d27",
		    "to" : "4cc7b2e7-f11a-47a3-ab35-c9d4e5f6364d",
	  	  "date" : "2025-08-23T12:13:14Z",
	  	  "json" : {
	  		  "type" : "text",
	  		  "text" : "Привет"
	  	  }
      }
  	}
  )";

  Message msg;
  msg.fromJson(json);

  if (not dbManager.saveMessage(msg)) {
    std::cerr << "Failed to save message" << std::endl;
  }

  boost::uuids::string_generator sgen;
  boost::uuids::uuid u = sgen("cbc69714-cb4d-437b-824e-c89750dd2699");
  boost::uuids::uuid u2 = gen();
  Message msgLoded = dbManager.loadMessage(u);

  std::cout << "Loaded msg: " << msgLoded.toJson() << std::endl;

  dbManager.deleteMessage(u);

  msgLoded = dbManager.loadMessage(u);

  std::cout << "Loaded msg is valid: " << msgLoded.isValid()
            << ", msg: " << msgLoded.toJson() << std::endl;

  std::cout << "----" << std::endl;
  std::cout << "Save processed msg:  " << boost::uuids::to_string(u) << ", "
            << dbManager.saveProcessedMessage(u) << std::endl;
  std::cout << "Save processed msg2: " << boost::uuids::to_string(u2) << ", "
            << dbManager.saveProcessedMessage(u2) << std::endl;

  auto ids = dbManager.loadProcessedMessages();

  std::cout << "Loaded processedd messages: ";
  for (auto id : ids) {
    std::cout << " id: " << id << std::endl;
  }

  std::cout << "Delete processed msg:  " << boost::uuids::to_string(u)
            << dbManager.deleteProcessedMessage(u) << std::endl;
  std::cout << "Delete processed msg2:  " << boost::uuids::to_string(u2)
            << dbManager.deleteProcessedMessage(u2) << std::endl;

  ids = dbManager.loadProcessedMessages();

  std::cout << "Loaded processedd messages: ";
  for (auto id : ids) {
    std::cout << " id: " << id << std::endl;
  }
  std::cout << std::endl;

  return 0;
}
