cmake_minimum_required(VERSION 3.15...3.28)

set(CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-windows/share")

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# project
project(ToDoService
    VERSION 0.1
    DESCRIPTION "Concurrent To-Do List API with Boost.Beast + PostgreSQL + Kafka"
    LANGUAGES CXX
)

# Boost
find_package(Boost 1.78 REQUIRED COMPONENTS
    system
    thread
    json
)

# libpqxx
find_package(libpqxx CONFIG REQUIRED)


# nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# ADD EXECUTABLES
add_executable(ToDoService
    src/Server.cpp
    src/Utility.hpp
    src/DbAccess.hpp
    src/ToDoService.cpp
)

# Boost
target_link_libraries(ToDoService PRIVATE Boost::system)
target_link_libraries(ToDoService PRIVATE Boost::thread)
target_link_libraries(ToDoService PRIVATE Boost::json)

# libpqxx
target_link_libraries(ToDoService PRIVATE libpqxx::pqxx)

# nlohmann_json
target_link_libraries(ToDoService PRIVATE nlohmann_json::nlohmann_json)


################# Unit Tests #################

enable_testing()
find_package(GTest CONFIG REQUIRED)

add_executable(todo_tests
    tests/todo_service_test.cpp
    src/ToDoService.cpp
    src/DbAccess.hpp   # if you have separate .cpp
    src/Utility.hpp
)

target_link_libraries(todo_tests PRIVATE
        GTest::gtest
        GTest::gtest_main
        Boost::json
        Boost::system
        libpqxx::pqxx
)

add_test(NAME TodoServiceTests COMMAND todo_tests)