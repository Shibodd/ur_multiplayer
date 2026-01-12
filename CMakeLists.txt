cmake_minimum_required (VERSION 3.21)

project (ur_multiplayer LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Ice REQUIRED CONFIG)

add_executable(client src/client.cpp idl/Greeter.ice)
slice2cpp_generate(client)
target_link_libraries(client PRIVATE Ice::Ice)

add_executable(server src/server.cpp idl/Greeter.ice)
slice2cpp_generate(server)
target_link_libraries(server PRIVATE Ice::Ice)