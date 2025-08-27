#include "message_test.h"

#include <iostream>

#include <boost/uuid/random_generator.hpp>

#include "include/message.h"

namespace Tests {
bool messageTest() {
  boost::uuids::random_generator gen;

  boost::uuids::uuid from = gen();
  boost::uuids::uuid to = gen();

  std::cout << "UUID from: " << boost::uuids::to_string(from) << std::endl;
  std::cout << "UUID to:   " << boost::uuids::to_string(to) << std::endl;

  TextMessage textMessage(from, to, "hello");

  std::cout << "Message:\n" << textMessage.toJson() << std::endl;

  TextMessage copyMessage(textMessage);

  std::cout << "Copy message:\n" << copyMessage.toJson() << std::endl;

  TextMessage copyMessage2(gen(), gen(), "");
  copyMessage2 = textMessage;

  std::cout << "Copy message2:\n" << copyMessage2.toJson() << std::endl;

  return true;
}
} // namespace Tests
