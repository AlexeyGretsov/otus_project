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

#include "include/transfer_message.h"

// #define ENABLE_TESTS

using boost::asio::ip::tcp;

typedef std::deque<TransferMessage> TransferMessageQueue;

class chat_participant {
public:
  virtual ~chat_participant() {}
  virtual void deliver(const TransferMessage &msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

class chat_room {
public:
  void join(chat_participant_ptr participant) {
    participants_.insert(participant);
    for (auto msg : recent_msgs_)
      participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant) {
    participants_.erase(participant);
  }

  void deliver(const TransferMessage &msg) {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant : participants_)
      participant->deliver(msg);
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  TransferMessageQueue recent_msgs_;
};

class Session : public chat_participant,
                public std::enable_shared_from_this<Session> {
public:
  Session(tcp::socket socket, chat_room &room)
      : socket(std::move(socket)), room_(room) {}

  void start() {
    std::cout << "Session starting" << std::endl;
    room_.join(shared_from_this());
    doReadHeader();
  }

  void deliver(const TransferMessage &msg) {
    bool write_in_progress = !writeMessages.empty();
    writeMessages.push_back(msg);
    if (!write_in_progress) {
      doWrite();
    }
  }

private:
  void doReadHeader() {
    std::cout << "doReadHeader" << std::endl;

    auto self(shared_from_this());
    boost::asio::async_read(
        socket,
        boost::asio::buffer(readMessage.getData(),
                            TransferMessage::headerLength),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          std::cout << "async_read header" << std::endl;
          if (!ec && readMessage.decodeHeader()) {

            doReadBody();
          } else {
            std::cout << "leave room" << std::endl;
            room_.leave(shared_from_this());
          }
        });
  }

  void doReadBody() {
    std::cout << "doReadBody" << std::endl;
    auto self(shared_from_this());
    boost::asio::async_read(
        socket,
        boost::asio::buffer(readMessage.getBody(), readMessage.getBodyLength()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
          std::cout << "async_read body" << std::endl;
          if (!ec) {
            room_.deliver(readMessage);
            doReadHeader();
          } else {
            room_.leave(shared_from_this());
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
            writeMessages.pop_front();
            if (!writeMessages.empty()) {
              doWrite();
            }
          } else {
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket;
  chat_room &room_;
  TransferMessage readMessage;
  TransferMessageQueue writeMessages;
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
            std::make_shared<Session>(std::move(socket), room_)->start();
          }

          doAccept();
        });
  }

  tcp::acceptor acceptor;
  chat_room room_;
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
