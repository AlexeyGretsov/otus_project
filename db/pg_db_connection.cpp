#include "db/pg_db_connection.h"

#include <iostream>

namespace Db
{
PgDbConnection::PgDbConnection()
{

}

PgDbConnection::~PgDbConnection()
{
    disconnect();
}

bool PgDbConnection::connect(std::string_view dbHost, int dbPort, std::string_view dbName, 
    std::string_view dbUser, std::string_view dbPassword)
{
    dbConn = PQsetdbLogin(
      dbHost.data(), std::to_string(dbPort).c_str(), nullptr, nullptr,
      dbName.data(), dbUser.data(), dbPassword.data());
    
    if (PQstatus(dbConn) != CONNECTION_OK && PQsetnonblocking(dbConn, 1) != 0) 
    {
        std::cerr << "[PgDbConnection::connect] Failed to connect to DB:"
                << PQerrorMessage(dbConn) << std::endl;
        
        return false;
    }

    std::cout << "PgDbConnection::connect: connection opened with host '"
            << dbHost << "'" << std::endl;

    return true;
}

bool PgDbConnection::disconnect()
{
    std::cout << "PgDbConnection::disconnect ..." << std::endl;

    PQfinish(dbConn);

    return true;
}

std::vector<std::vector<std::string>> PgDbConnection::select(std::string_view query)
{
    std::vector<std::vector<std::string>> result;

    if (not dbConn)
    {
        std::cerr << "[PgDbConnection::select] unable to select data. "
                    "Connection not extablished" << std::endl;

        return result;
    }

    if (not PQsendQuery(dbConn, query.data()))
    {
        std::cerr << "[PgDbConnection::select] query error: "
            << PQerrorMessage(dbConn) << std::endl;

        return result;
    }

    while (auto res = PQgetResult(dbConn)) 
    {
        if (PQresultStatus(res) == PGRES_TUPLES_OK) 
        {
            for (int i = 0; i < PQntuples(res); i++) 
            {
                std::vector<std::string> row;

                for (int j = 0; j < PQnfields(res); j++)
                {
                    row.push_back(PQgetvalue(res, i, j));
                }

                result.push_back(row);
            }
        }

        if (PQresultStatus(res) == PGRES_FATAL_ERROR) 
        {
            std::cerr << "[PgDbConnection::select] Read select result error:"
                << PQresultErrorMessage(res) << std::endl;
        }

        PQclear(res);
    }

    return result;
}

bool PgDbConnection::insert(std::string_view query)
{
    if (not dbConn)
    {
        std::cerr << "[PgDbConnection::insert] unable to insert data. "
                    "Connection not extablished" << std::endl;

        return false;
    }

    if (not PQsendQuery(dbConn, query.data()))
    {
        std::cerr << "[PgDbConnection::insert] query error: "
            << PQerrorMessage(dbConn) << std::endl;

        return false;
    }

    while (auto res = PQgetResult(dbConn)) 
    {
        if (PQresultStatus(res) != PGRES_COMMAND_OK) 
        {
            std::cerr << "[PgDbConnection::insert] query result error: " 
                << PQerrorMessage(dbConn) << std::endl;
        }

        PQclear(res);
    }

    return true;
} 

bool PgDbConnection::del(std::string_view query)
{
    if (not dbConn)
    {
        std::cerr << "[PgDbConnection::del] unable to delete data. "
                    "Connection not extablished" << std::endl;

        return false;
    }

    if (not PQsendQuery(dbConn, query.data()))
    {
        std::cerr << "[PgDbConnection::del] query error: "
            << PQerrorMessage(dbConn) << std::endl;

        return false;
    }

    while (auto res = PQgetResult(dbConn)) 
    {
        if (PQresultStatus(res) != PGRES_COMMAND_OK) 
        {
            std::cerr << "[PgDbConnection::del] query result error: " 
                << PQerrorMessage(dbConn) << std::endl;
        }

        PQclear(res);
    }

    return true;
}
}
