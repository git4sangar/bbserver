#sgn
cmake_minimum_required(VERSION 3.1)
project(BBServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_executable(BBServer
src/SGNServer_01.cpp
src/ClientManager.cpp
src/ConfigManager.cpp
src/Constants.cpp
src/FileLogger.cpp
src/FileManager.cpp
src/UDPManager.cpp
src/TCPManager.cpp
src/Protocol.cpp
src/StateMachine.cpp
src/Timer.cpp)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(BBServer -lpthread) 
