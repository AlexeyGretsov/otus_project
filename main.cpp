#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include "tests/db_manager_test.h"
#include "tests/db_test.h"
#include "tests/json_test.h"
#include "tests/message_test.h"

#include "db/db_manager.h"
#include "db/pg_db_connection.h"
#include "include/message.h"
#include "include/transfer_message.h"

// #define ENABLE_TESTS

using boost::asio::ip::tcp;


class MessagesProcessor;

typedef std::deque<TransferMessage> TransferMessageQueue;
using DbConnectionPtr = std::shared_ptr<Db::IDbConnection>;
using MessagesProcessorPtr = std::shared_ptr<MessagesProcessor>;

inline DbConnectionPtr createDbConnection() {
  std::string dbHost = "localhost";
  int dbPort = 5432;
  std::string dbName = "otus_messendger";
  std::string dbUser = "postgres";
  std::string dbPass = "postgres";

  DbConnectionPtr dbConn(new Db::PgDbConnection());
  if (not dbConn->connect(dbHost, dbPort, dbName, dbUser, dbPass)) {
    std::cerr << "Failed to connect DB" << std::endl;
    return nullptr;
  }

  return dbConn;
}

class MessagesProcessor
{
  public:
  virtual ~MessagesProcessor(){}

  virtual void checkProcessedMessages() = 0;
};

class SessionsManager
{
  public:
    void addSession(MessagesProcessorPtr processor)
    {
      processors.insert(processor);
    }
    void removeSession(MessagesProcessorPtr processor)
    {
      processors.erase(processor);
    }
    void checkProcessedMessages()
    {
      std::cout << "SessionsManager::checkProcessedMessages. Processors count: " 
          << processors.size() << std::endl;
      for (auto processor : processors)
      {
        processor->checkProcessedMessages();
      }
    }

  private:
    std::set<MessagesProcessorPtr> processors;
};

class Session : public MessagesProcessor, public std::enable_shared_from_this<Session> {
public:
  Session(tcp::socket socket, SessionsManager& sessionManager) : socket(std::move(socket)),
      sessionManager(sessionManager) {}

  void start() {
    sessionManager.addSession(shared_from_this());

    dbConnectionPtr = createDbConnection();
    if (not dbConnectionPtr) {
      std::cerr << "Failed to start session: no DB connection" << std::endl;

      return;
    }
    std::cout << "Session starting" << std::endl;
    doReadHeader();
  }

private:
  void doReadHeader() {
    auto self(shared_from_this());
    boost::asio::async_read(
        socket,
        boost::asio::buffer(readMessage.getData(),
                            TransferMessage::headerLength),
        [this, self](boost::system::error_code ec, std::size_t size) {
          std::cout << "async_read header: " << size << std::endl;
          if (!ec)
          { 
                      std::cout << "no errors" << std::endl;
            if (readMessage.decodeHeader()) {
              std::cout << "body length = " << readMessage.getBodyLength() << std::endl;
            doReadBody();
            }
          }
          else
          {
            std::cout << "Remove session for client: " << clientUuid << std::endl;
            sessionManager.removeSession(shared_from_this());
          }
        });
  }

  void doReadBody() {
    auto self(shared_from_this());
    boost::asio::async_read(
        socket,
        boost::asio::buffer(readMessage.getBody(), readMessage.getBodyLength()),
        [this, self](boost::system::error_code ec, std::size_t size) {
          std::cout << "async_read body: " << size << std::endl;
          if (!ec) {
            std::cout << "save message" << std::endl;
            saveMessage(readMessage);

            doReadHeader();
          }
          else
          {
            std::cout << "Remove session for client: " << clientUuid << std::endl;
            sessionManager.removeSession(shared_from_this());
          }
        });
  }

  void doWrite() {
    auto self(shared_from_this());
    boost::asio::async_write(
        socket,
        boost::asio::buffer(writeMessages.front().getData(),
                            writeMessages.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {

            if (not clearProcessedMessage(writeMessages.front()))
            {
              std::runtime_error("Failed to clear processed message");
            }

            writeMessages.pop_front();
            if (!writeMessages.empty()) {
              doWrite();
            }
            else
            {
              sessionManager.checkProcessedMessages();
//              checkProcessedMessages();
            }
          }
          else
          {
            std::cout << "Remove session for client: " << clientUuid << std::endl;
            sessionManager.removeSession(shared_from_this());
          }

        });
  }

  bool saveMessage(const TransferMessage &readMessage) {
    std::cout << "before parse: '" << readMessage.getBody() << "', " 
        << std::strlen(readMessage.getBody()) << std::endl;
    Message msg;
    if (not msg.fromJson(std::string(readMessage.getBody(), readMessage.getBodyLength()))) {
      std::cerr << "Failed to parse received message" << std::endl;

      return false;
    }

    std::cout << "after parse" << std::endl;
    if (msg.isAuth()) {
      std::cerr << "Received auth message " << msg.toJson();

      clientUuid = msg.from;

      std::cout << "before check" << std::endl;
      sessionManager.checkProcessedMessages();
//      checkProcessedMessages();
      std::cout << "after check" << std::endl;

      return true;
    }

    auto save = [](DbConnectionPtr dbConnectionPtr,
                   const Message *msg) -> bool {
      // TODO think about DB transaction ...

      DbManager dbManager(dbConnectionPtr);
      if (not dbManager.saveMessage(*msg)) {
        std::cerr << "Failed to save message to DB" << std::endl;

        return false;
      }
      if (not dbManager.saveProcessedMessage(msg->to, msg->id)) {
        std::cerr << "Failed to save processed message to DB" << std::endl;
        dbManager.deleteMessage(msg->id);

        return false;
      }

      return true;
    };

    if (not save(dbConnectionPtr, &msg)) {
      return false;
    }

    std::cerr << "Received message " << msg.toJson() << std::endl;

    // Message to send back with status 'processed'
    StatusMessage statusMsg(msg.from, msg.id, Message::STATUS_PROCESSED);

    if (not save(dbConnectionPtr, &statusMsg)) {
      return false;
    }

    std::cerr << "Message registered" << std::endl;

    sessionManager.checkProcessedMessages();
//checkProcessedMessages();

    return true;
  }

  void checkProcessedMessages() override
  {
    if (clientUuid.is_nil())
    {
      throw std::runtime_error("No client UUID after authirization");
    }

    std::cout << "checkProcessedMessages: " << boost::uuids::to_string(clientUuid) << std::endl;

    DbManager dbManager(dbConnectionPtr);
    std::vector<boost::uuids::uuid> ids = dbManager.loadProcessedMessages(clientUuid);

    std::cout << "Found " << ids.size() << " records" << std::endl;

    for (auto id : ids)
    {
      Message msg = dbManager.loadMessage(id);
      if (msg.isValid())
      {
        std::string s = msg.toJson();
        std::cout << "Put message " << s << ", size: " << s.size() << " to queue" << std::endl;

        TransferMessage transferMsg{msg.toJson()};

         std::cout << "Transfer message " << transferMsg.getBody() << ", length: "
         << transferMsg.getBodyLength() << std::endl;
        std::cout << "Transfer message created" << std::endl;
//        writeMessages.emplace_back(msg.toJson());
        writeMessages.push_back(transferMsg);

      }
    }

    if (not writeMessages.empty())
    {

    doWrite();
    }
  }

  bool clearProcessedMessage(const TransferMessage &transferMessage)
  {
    Message msg;
    msg.fromJson(transferMessage.getBody());

    DbManager dbManager(dbConnectionPtr);
    return dbManager.deleteProcessedMessage(msg.id);
  }

  boost::uuids::uuid clientUuid;
  tcp::socket socket;
  TransferMessage readMessage;
  TransferMessageQueue writeMessages;
  DbConnectionPtr dbConnectionPtr;
  SessionsManager &sessionManager;
};

class Server {
public:
  Server(boost::asio::io_context &io_context, const tcp::endpoint &endpoint)
      : acceptor(io_context, endpoint) {
    doAccept();
  }

private:
  void doAccept() {
    acceptor.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          if (!ec) {
            std::cout << "Accept conection" << std::endl;
            std::make_shared<Session>(std::move(socket), sessionManager)->start();
          }

          doAccept();
        });
  }

  tcp::acceptor acceptor;
  SessionsManager sessionManager;
};

int main(int argc, char *argv[]) {

#ifdef ENABLE_TESTS
  if (not Tests::dbTest()) {
    return EXIT_FAILURE;
  }
  if (not Tests::jsonTest()) {
    return EXIT_FAILURE;
  }
  if (not Tests::dbManagerTest()) {
    return EXIT_FAILURE;
  }
  if (not Tests::messageTest()) {
    return EXIT_FAILURE;
  }

  return 0;
#endif

  try {
    if (argc < 2) {
      std::cerr << "Usage: Server <port> [<port> ...]\n";
      return 1;
    }

    boost::asio::io_context io_context;

    std::list<Server> servers;
    for (int i = 1; i < argc; ++i) {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_context, endpoint);
    }

    io_context.run();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
