#include <iostream>
#include <memory>
#include <regex>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <chrono>
#include <thread>

using boost::asio::ip::tcp;

class Client {
public:
  Client(boost::asio::io_context &io_context, const std::string &host,
         const std::string &port)
      : io_context_(io_context), socket_(io_context) {

    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, port);

    boost::asio::async_connect(
        socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint) {
          if (!ec) {
            std::cout << "Connected to server!" << std::endl;
            is_connected_ = true;
            do_write();
          } else {
            std::cerr << "Connection failed: " << ec.message() << std::endl;
          }
        });
  }

  void send_message(const std::string &message) {
    if (is_connected_) {
      boost::asio::post(io_context_, [this, message]() {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(message);
        if (!write_in_progress) {
          do_write();
        }
      });
    }
  }

private:
  void do_write() {
    if (!write_msgs_.empty()) {
      boost::asio::async_write(
          socket_,
          boost::asio::buffer(write_msgs_.front().data(),
                              write_msgs_.front().length()),
          [this](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
              write_msgs_.pop_front();
              if (!write_msgs_.empty()) {
                do_write();
              }
            } else {
              std::cerr << "Write error: " << ec.message() << std::endl;
              socket_.close();
            }
          });
    }
  }

  boost::asio::io_context &io_context_;
  tcp::socket socket_;
  std::deque<std::string> write_msgs_;
  bool is_connected_ = false;
};

int main() {
  try {
    boost::asio::io_context io_context;

    Client client(io_context, "127.0.0.1", "8080");

    // Запускаем io_context в отдельном потоке

    // Отправляем сообщения
    for (int i = 0; i < 5; ++i) {
      std::string message =
          "Hello from client! Message #" + std::to_string(i + 1);
      client.send_message(message);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::thread io_thread([&io_context]() { io_context.run(); });

    // Ждем завершения
    std::this_thread::sleep_for(std::chrono::seconds(2));
    io_context.stop();
    io_thread.join();

  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  return 0;
}
