#include <iostream>
#include <memory>

#include <boost/asio.hpp>
//#include <libpq-fe.h>
#include "db/pg_db_connection.h"

#include <nlohmann/json.hpp>

int main(int argc, char *argv[]) {
  std::string dbHost = "localhost";
  int dbPort = 5432;
  std::string dbName = "otus_messendger";
  std::string dbUser = "postgres";
  std::string dbPass = "postgres";

  std::shared_ptr<Db::IDbConnection> dbConn(new Db::PgDbConnection());
  if (not dbConn->connect(dbHost, dbPort, dbName, dbUser, dbPass))
  {
    return EXIT_FAILURE;
  }

  auto selectUsers = [=](){
    std::string query = "select * from users";
    auto usersInfo = dbConn->select(query);

    std::cout << "Users count " << usersInfo.size() << std::endl;
    for (auto row : usersInfo)
    {
      for (auto val : row)
      {
        std::cout << val << " ";
      }
      std::cout << std::endl;
    }
  };

  selectUsers();

  std::string query = "insert into users (ID, name) values ('e3f26aab-b5e9-4c92-89bd-ae89875b04bd', 'user 555')";
  if (not dbConn->insert(query))
  {
    return EXIT_FAILURE;
  }

  selectUsers();

  query = "delete from users where name = 'user 555'";
  if (not dbConn->del(query))
  {
    return EXIT_FAILURE;
  }

  selectUsers();

  return 0;
}
