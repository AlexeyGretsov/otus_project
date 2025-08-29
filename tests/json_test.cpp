#include "json_test.h"

#include <iostream>
#include <memory>
#include <string>

#include "include/message.h"

namespace Tests {
bool jsonTest() {
  std::string json = R"(
  {
	    "message" : {
		    "id" : "cbc69714-cb4d-437b-824e-c89750dd2699",
		    "from" : "fd0c7575-e4d7-4590-b0e7-c32481f75d27",
		    "to" : "4cc7b2e7-f11a-47a3-ab35-c9d4e5f6364d",
	  	  "date" : "2025-08-23T12:13:14Z",
	  	  "json" : {
	  		  "type" : "text",
	  		  "text" : "Привет"
	  	  }
      }
  	}
  )";

  Message msg;
  msg.fromJson(json);

  std::string json2 = R"(
  {
	    "message" : {
		    "id" : "cbc69714-cb4d-437b-824e-c89750dd2699",
		    "from" : "fd0c7575-e4d7-4590-b0e7-c32481f75d27",
		    "to" : "4cc7b2e7-f11a-47a3-ab35-c9d4e5f6364d",
	  	  "date" : "2025-08-23T12:13:14Z",
	  	  "json" : {
	  		  "type" : "status",
	  		  "message_id" : "aaa32efd-8a90-4225-98a5-82f4f9b9fa8d",
          "status" : "received"
	  	  }
      }
  	}
  )";

  Message msg2;
  msg2.fromJson(json2);

  std::cout << msg.toJson() << std::endl;
  std::cout << msg2.toJson() << std::endl;

  Message msg3;
  msg3.fromJson(msg2.toJson());

  std::cout << "Msg3: " << msg3.toJson() << std::endl;

  return true;
}
} // namespace Tests
