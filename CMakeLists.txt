cmake_minimum_required(VERSION 3.11)
project(search_server)

set(CMAKE_CXX_STANDARD 17)
include_directories("search-server")
file(GLOB SEARCH_SERVER_SRC
        "search-server/*.h"
        "search-server/*.cpp"
        )

add_executable(search_server ${SEARCH_SERVER_SRC})