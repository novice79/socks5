cmake_minimum_required(VERSION 3.20)

project(main)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
# use all static lib
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()
file(GLOB SOURCE_FILES 
    "src/*.cpp"
    "src/*.h"
)
include_directories("src")
# grep -r --include="CMakeLists.txt" "fvisibility" $HOME/workspace
add_library(socks5 STATIC ${SOURCE_FILES})

set(SOURCE_EXAMPLE_FILES
    "example/test.cpp"
)
add_executable(test ${SOURCE_EXAMPLE_FILES})

set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)
find_package(Boost REQUIRED COMPONENTS 
    json
    system 
    filesystem
    log_setup 
    log
)

include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(socks5 ${Boost_LIBRARIES} )
target_link_libraries(test ${Boost_LIBRARIES} socks5)

install(TARGETS socks5  LIBRARY DESTINATION lib)
install(FILES src/socks5.h DESTINATION include)
# install (
#     DIRECTORY ${CMAKE_SOURCE_DIR}/src/
#     DESTINATION include
#     FILES_MATCHING PATTERN "*.h*")
install(TARGETS test DESTINATION bin)


