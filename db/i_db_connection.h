#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace Db {
class IDbConnection {
public:
  IDbConnection() {}
  virtual ~IDbConnection() {}

  virtual bool connect(std::string_view dbHost, int dbPort,
                       std::string_view dbName, std::string_view dbUser,
                       std::string_view dbPassword) = 0;
  virtual bool isConnected() const = 0;

  virtual bool disconnect() = 0;

  virtual std::vector<std::vector<std::string>>
  select(std::string_view query) = 0;
  virtual bool insert(std::string_view query) = 0;
  virtual bool del(std::string_view query) = 0;
};
} // namespace Db
