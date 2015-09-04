cmake_minimum_required(VERSION 2.8)

project (tob)

set (JASL "${PROJECT_SOURCE_DIR}/../jasl")
set (JSON "${PROJECT_SOURCE_DIR}/json")

include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${JASL}" "${JSON}")
set (CMAKE_INCLUDE_PATH "/usr/local/include")
set (CMAKE_LIBRARY_PATH "/usr/local/lib")

LINK_DIRECTORIES(${PROJECT_BINARY_DIR})
LINK_DIRECTORIES(${JASL}/build)

set (CMAKE_CXX_COMPLILER "clang++")
set (CMAKE_CXX_FLAGS "-std=c++0x -ggdb3 -Wall")

set (TARGET main)

set (SOURCE_LIST main.cpp
                 frame.cpp
                 order_book.cpp
                 receive_order_book_logic.cpp
                 json/jsoncpp.cpp
    )

add_executable (${TARGET} ${SOURCE_LIST})
target_link_libraries(${TARGET}
                        jasl
                        pthread
                        ssl
                        crypto
                     )

