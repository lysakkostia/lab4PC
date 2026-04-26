cmake_minimum_required(VERSION 4.1)
project(Lab4PC)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)

add_executable(Server server_main.cpp)
target_link_libraries(Server PRIVATE Threads::Threads)
target_link_libraries(Server PRIVATE ws2_32)

add_executable(Client client_main.cpp)
target_link_libraries(Client PRIVATE Threads::Threads)
target_link_libraries(Client PRIVATE ws2_32)

