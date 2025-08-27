#include <boost/asio.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>

#include "include/transfer_message.h"

using boost::asio::ip::tcp;

typedef std::deque<TransferMessage> TransferMessage_queue;

class chat_client {
public:
  chat_client(boost::asio::io_context &io_context,
              const tcp::resolver::results_type &endpoints)
      : io_context_(io_context), socket_(io_context) {
    do_connect(endpoints);
  }

  void write(const TransferMessage &msg) {
    std::cout << "write msg" << msg.getBody() << std::endl;
    boost::asio::post(io_context_, [this, msg]() {
      bool write_in_progress = !write_msgs_.empty();

      std::cout << "write in progress" << write_in_progress << std::endl;

      write_msgs_.push_back(msg);
      if (!write_in_progress) {
        std::cout << "do_write" << std::endl;
        do_write();
      }
    });
  }

  void close() {
    boost::asio::post(io_context_, [this]() { socket_.close(); });
  }

private:
  void do_connect(const tcp::resolver::results_type &endpoints) {
    boost::asio::async_connect(
        socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint) {
          if (!ec) {
            std::cout << "Connection extablished" << std::endl;
            do_read_header();
          }
        });
  }

  void do_read_header() {
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_msg_.getData(), TransferMessage::headerLength),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec && read_msg_.decodeHeader()) {
            do_read_body();
          } else {
            socket_.close();
          }
        });
  }

  void do_read_body() {
    boost::asio::async_read(
        socket_,
        boost::asio::buffer(read_msg_.getBody(), read_msg_.getBodyLength()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            std::cout.write(read_msg_.getBody(), read_msg_.getBodyLength());
            std::cout << "\n";
            do_read_header();
          } else {
            socket_.close();
          }
        });
  }

  void do_write() {
    std::cout << "do_write" << std::endl;

    boost::asio::async_write(
        socket_,
        boost::asio::buffer(write_msgs_.front().getData(),
                            write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/) {
          if (!ec) {
            std::cout << "do_write done" << std::endl;
            write_msgs_.pop_front();
            if (!write_msgs_.empty()) {
              std::cout << "do_write more" << std::endl;
              do_write();
            }
          } else {
            socket_.close();
          }
        });
  }

private:
  boost::asio::io_context &io_context_;
  tcp::socket socket_;
  TransferMessage read_msg_;
  TransferMessage_queue write_msgs_;
};

int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    chat_client c(io_context, endpoints);

    std::thread t([&io_context]() { io_context.run(); });

    char line[TransferMessage::MAX_BODY_LENGTH + 1];
    while (std::cin.getline(line, TransferMessage::MAX_BODY_LENGTH + 1)) {
      TransferMessage msg;
      msg.setBodyLength(std::strlen(line));
      std::memcpy(msg.getBody(), line, msg.getBodyLength());
      msg.encodeHeader();

      c.write(msg);
    }

    c.close();
    t.join();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
