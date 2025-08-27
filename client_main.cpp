#include <boost/asio.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>

#include "include/message.h"
#include "include/transfer_message.h"

using boost::asio::ip::tcp;

typedef std::deque<TransferMessage> TransferMessageQueue;

class Client {
public:
  Client(boost::asio::io_context &io_context,
         const tcp::resolver::results_type &endpoints)
      : io_context(io_context), socket(io_context) {
    doConnect(endpoints);
  }

  void write(const TransferMessage &msg) {
    std::cout << "write msg" << msg.getBody() << std::endl;
    boost::asio::post(io_context, [this, msg]() {
      bool write_in_progress = !writeMessages.empty();

      std::cout << "write in progress" << write_in_progress << std::endl;

      writeMessages.push_back(msg);
      if (!write_in_progress) {
        std::cout << "doWrite" << std::endl;
        doWrite();
      }
    });
  }

  void close() {
    boost::asio::post(io_context, [this]() { socket.close(); });
  }

private:
  void doConnect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(
        socket, endpoints, [this](boost::system::error_code ec, tcp::endpoint) {
          if (!ec) {
            std::cout << "Connection extablished" << std::endl;
            doReadHeader();
          }
        });
  }

  void doReadHeader() {
    boost::asio::async_read(
        socket,
        boost::asio::buffer(readMessage.getData(),
                            TransferMessage::headerLength),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec && readMessage.decodeHeader()) {
            doReadBody();
          } else {
            socket.close();
          }
        });
  }

  void doReadBody() {
    boost::asio::async_read(
        socket,
        boost::asio::buffer(readMessage.getBody(), readMessage.getBodyLength()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            std::cout.write(readMessage.getBody(), readMessage.getBodyLength());
            std::cout << "\n";
            doReadHeader();
          } else {
            socket.close();
          }
        });
  }

  void doWrite() {
    std::cout << "doWrite" << std::endl;

    boost::asio::async_write(
        socket,
        boost::asio::buffer(writeMessages.front().getData(),
                            writeMessages.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            std::cout << "doWrite done" << std::endl;
            writeMessages.pop_front();
            if (!writeMessages.empty()) {
              std::cout << "doWrite more" << std::endl;
              doWrite();
            }
          } else {
            socket.close();
          }
        });
  }

private:
  boost::asio::io_context &io_context;
  tcp::socket socket;
  TransferMessage readMessage;
  TransferMessageQueue writeMessages;
};

int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: Client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    Client c(io_context, endpoints);

    std::thread t([&io_context]() { io_context.run(); });

    boost::uuids::random_generator gen;
    boost::uuids::uuid from = gen();
    boost::uuids::uuid to = gen();

    char line[TransferMessage::MAX_BODY_LENGTH + 1];
    while (std::cin.getline(line, TransferMessage::MAX_BODY_LENGTH + 1)) {

      if (std::strlen(line) == 0) {
        continue;
      }
      TextMessage msg(from, to, line);
      std::string json = msg.toJson();

      TransferMessage transferMessage;
      transferMessage.setBodyLength(json.length());
      std::memcpy(transferMessage.getBody(), json.c_str(),
                  transferMessage.getBodyLength());
      transferMessage.encodeHeader();

      c.write(transferMessage);
    }

    c.close();
    t.join();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
