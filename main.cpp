#include <chrono>
#include <iostream>
#include <memory>
#include <regex>
#include <thread>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/algorithm/string/replace.hpp>
#include <boost/asio.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "tests/db_manager_test.h"
#include "tests/db_test.h"
#include "tests/json_test.h"

// #define ENABLE_TESTS

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
#endif
  return 0;
}
