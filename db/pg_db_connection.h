#pragma once

#include <memory>

#include <libpq-fe.h>

#include "db/i_db_connection.h"

namespace Db
{
class PgDbConnection : public IDbConnection
{
    public:
        PgDbConnection();
        virtual ~PgDbConnection();

        bool connect(std::string_view dbHost, int dbPort, std::string_view dbName, 
                std::string_view dbUser, std::string_view dbPassword) override;

        bool disconnect() override;

        std::vector<std::vector<std::string>> select(std::string_view query) override;
        bool insert(std::string_view query) override; 
        bool del(std::string_view query) override;

    private:
        PGconn* dbConn{nullptr};
};
} // namespace Db
