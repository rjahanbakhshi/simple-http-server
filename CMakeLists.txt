cmake_minimum_required(VERSION 3.16)

# Conditionally enable Hunter
option(HUNTER_ENABLED "Enable Hunter package manager" ON)

# Setting up hunter
set(HUNTER_Boost_COMPONENTS system)
set(HUNTER_Boost_VERSION 1.81.0)
set(HUNTER_PACKAGES Boost)

include(FetchContent)
FetchContent_Declare(SetupHunter GIT_REPOSITORY https://github.com/cpp-pm/gate)
FetchContent_MakeAvailable(SetupHunter)

project(simple-http-server VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

find_package(
  Boost 1.81 REQUIRED COMPONENTS
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
