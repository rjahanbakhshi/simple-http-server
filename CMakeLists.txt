cmake_minimum_required(VERSION 3.16)

option(HUNTER_ENABLED "Enable Hunter package manager" ON)
# Setting up Hunter.pm
include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.24.3.tar.gz"
    SHA1 "10738b59e539818a01090e64c2d09896247530c7"
)

project(simple-http-server VERSION 1.0)

hunter_add_package(Boost COMPONENTS system)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(
  Boost 1.78 REQUIRED COMPONENTS
  system
)

find_package(Threads REQUIRED)

add_executable(simple-http-server simple-http-server.cpp)

target_link_libraries(
  simple-http-server PRIVATE
  Boost::system
  Threads::Threads
)

target_include_directories(
  simple-http-server PRIVATE
  "${Boost_INCLUDE_DIRS}"
)

install(
    TARGETS simple-http-server
    LIBRARY DESTINATION bin
)

