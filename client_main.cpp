#include <boost/asio.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>

#include <boost/uuid/string_generator.hpp>

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
    boost::asio::post(io_context, [this, msg]() {
      bool write_in_progress = !writeMessages.empty();

      writeMessages.push_back(msg);
      if (!write_in_progress) {
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
            if (not processMessage(readMessage)) {
              socket.close();
              return;
            }
            doReadHeader();
          } else {
            socket.close();
          }
        });
  }

  void doWrite() {
    boost::asio::async_write(
        socket,
        boost::asio::buffer(writeMessages.front().getData(),
                            writeMessages.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            writeMessages.pop_front();
            if (!writeMessages.empty()) {
              doWrite();
            }
          } else {
            socket.close();
          }
        });
  }

private:
  bool processMessage(const TransferMessage &readMessage) {
    Message msg;
    if (not msg.fromJson(
            std::string(readMessage.getBody(), readMessage.getBodyLength()))) {
      std::cerr << "Failed to parse received message" << std::endl;

      return false;
    }

    if (msg.isText()) {
      TextMessageJson *json = static_cast<TextMessageJson *>(msg.json);
      std::cout << "[Получено сообщение] " << json->text << std::endl;

      StatusMessage statusMessage(msg.from, msg.id, Message::STATUS_RECEIVED);
      TransferMessage transferMessage(statusMessage.toJson());
      write(transferMessage);
    } else if (msg.isStatus()) {
      StatusMessageJson *json = static_cast<StatusMessageJson *>(msg.json);
      std::cout << "[Статус сообщения] " << json->status << std::endl;
    } else {
      throw std::runtime_error("Undefined message received");
    }

    return true;
  }

  boost::asio::io_context &io_context;
  tcp::socket socket;
  TransferMessage readMessage;
  TransferMessageQueue writeMessages;
};

inline void usage() {
  std::cerr << "Usage: Client <host> <port> <my UUID> <remote UUID>\n";
}

int main(int argc, char *argv[]) {
  try {
    if (argc != 5) {
      usage();
      return EXIT_FAILURE;
    }

    boost::uuids::string_generator sgen;
    boost::uuids::uuid from = sgen(argv[3]);
    boost::uuids::uuid to = sgen(argv[4]);

    if (from.is_nil()) {
      std::cerr << "Invalid parameter: my UUID" << std::endl;
      usage();
      return EXIT_FAILURE;
    }

    if (to.is_nil()) {
      std::cerr << "Invalid parameter: remote UUID" << std::endl;
      usage();
      return EXIT_FAILURE;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    Client c(io_context, endpoints);

    std::thread t([&io_context]() { io_context.run(); });

    // Send authorize message
    TransferMessage transferMessage(AuthMessage(from).toJson());
    c.write(transferMessage);

    char line[TransferMessage::MAX_BODY_LENGTH + 1];

    while (std::cin.getline(line, TransferMessage::MAX_BODY_LENGTH + 1)) {
      if (std::strlen(line) == 0) {
        continue;
      }
      TextMessage msg(from, to, line);

      c.write(TransferMessage(msg.toJson()));
    }

    c.close();
    t.join();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
