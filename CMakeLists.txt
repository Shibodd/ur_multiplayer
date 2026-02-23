cmake_minimum_required (VERSION 3.21)

project (ur_multiplayer LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Ice REQUIRED CONFIG)

include (idl/sources.cmake)

add_library (server STATIC src/server.cpp ${IDL_MESSAGE_SOURCES})
slice2cpp_generate (server)
target_link_libraries (server Ice::Ice)
target_include_directories (server PUBLIC include ${CMAKE_BINARY_DIR}/generated/server)

add_library (participant STATIC src/participant.cpp ${IDL_MESSAGE_SOURCES})
slice2cpp_generate (participant)
target_link_libraries (participant Ice::Ice)
target_include_directories (participant PUBLIC include ${CMAKE_BINARY_DIR}/generated/participant)

add_executable (demo_client demo/client.cpp)
target_link_libraries (demo_client participant)

add_executable (demo_server demo/server.cpp)
target_link_libraries (demo_server server)