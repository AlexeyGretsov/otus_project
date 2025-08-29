#include "db_test.h"

#include <iostream>
#include <memory>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "db/pg_db_connection.h"

namespace Tests {
bool dbTest() {
  boost::uuids::random_generator gen;
  std::string dbHost = "localhost";
  int dbPort = 5432;
  std::string dbName = "otus_messendger";
  std::string dbUser = "postgres";
  std::string dbPass = "postgres";

  std::shared_ptr<Db::IDbConnection> dbConn(new Db::PgDbConnection());
  if (not dbConn->connect(dbHost, dbPort, dbName, dbUser, dbPass)) {
    std::cerr << "Failed to connect DB" << std::endl;
    return false;
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
    std::cerr << "Failed to insert" << std::endl;
    return false;
  }

  selectUsers();

  query = "delete from users where name = 'user 555'";
  if (not dbConn->del(query)) {
    std::cerr << "Failed to delete" << std::endl;
    return false;
  }

  selectUsers();

  return true;
}
} // namespace Tests
