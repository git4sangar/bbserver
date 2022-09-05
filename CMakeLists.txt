#sgn
cmake_minimum_required(VERSION 3.1)
project(BBServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_executable(BBServer
SGNServer_01.cpp
ClientManager.cpp
ConfigManager.cpp
Constants.cpp
FileLogger.cpp
FileManager.cpp
NetworkManager.cpp
Protocol.cpp
StateMachine.cpp
Timer.cpp)

target_link_libraries(BBServer -lpthread) 