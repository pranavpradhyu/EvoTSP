cmake_minimum_required(VERSION 3.14)
project(evotsp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(evotsp server.cpp)

find_package(Threads REQUIRED)
target_link_libraries(evotsp PRIVATE Threads::Threads)

# On Windows, cpp-httplib needs the Winsock library.
if (WIN32)
  target_link_libraries(evotsp PRIVATE ws2_32)
endif()
