cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "") 

project(otus_final_project)

enable_testing()
find_package(GTest REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

set(SOURCES ${SOURCES}
	main.cpp
	db/pg_db_connection.cpp
	db/db_manager.cpp
	include/message.cpp

	tests/db_test.cpp
	tests/json_test.cpp
	tests/db_manager_test.cpp
)

set(HEADERS ${HEADERS}
	db/i_db_connection.h
	db/pg_db_connection.h
	db/db_manager.h
	include/message.h

	tests/db_test.h
	tests/json_test.h
	tests/db_manager_test.h
)

set(ASIO_CLIENT_SOURCES ${ASIO_CLIENT_SOURCES}
	asio_client_main.cpp
)

add_executable(${CMAKE_PROJECT_NAME} 
	${SOURCES}
	${HEADERS}
)

add_executable(asio_client 
	${ASIO_CLIENT_SOURCES}
)

target_link_libraries(asio_client PRIVATE 
	${Boost_LIBRARIES} 
	nlohmann_json
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
	${Boost_LIBRARIES} 
	PostgreSQL::PostgreSQL 
	nlohmann_json
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON) 

install(TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION bin)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CPACK_GENERATOR TGZ)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CPACK_GENERATOR DEB)
endif()

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})

set(CPACK_PACKAGE_CONTACT alexey.gretsov@gmail.com)

include(CPack)

