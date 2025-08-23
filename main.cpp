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

  // PGconn *dbConn = PQsetdbLogin(
  //     m_dbhost.c_str(), std::to_string(m_dbport).c_str(), nullptr, nullptr,
  //     m_dbname.c_str(), m_dbuser.c_str(), m_dbpass.c_str());

  // if (PQstatus(dbConn) != CONNECTION_OK && PQsetnonblocking(dbConn, 1) != 0) {
  //   throw std::runtime_error(PQerrorMessage(dbConn));
  // }

  // std::string query = "select * from users";

  // PQsendQuery(dbConn, query.c_str());

  // while (auto res_ = PQgetResult(dbConn)) {
  //   if (PQresultStatus(res_) == PGRES_TUPLES_OK) {
  //     int numRows = PQntuples(res_);
  //     int numCols = PQnfields(res_);

  //     for (int i = 0; i < numRows; i++) {
  //       auto name = PQgetvalue(res_, i, 2);
  //       auto ID = PQgetvalue(res_, i, 1);
  //       std::cout << "User " << name << " ID " << ID << std::endl;
  //     }
  //   }

  //   if (PQresultStatus(res_) == PGRES_FATAL_ERROR) {
  //     std::cout << PQresultErrorMessage(res_) << std::endl;
  //   }

  //   PQclear(res_);
  // }

  return 0;
}
