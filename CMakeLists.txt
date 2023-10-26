cmake_minimum_required(VERSION 3.26)
project(balibali)

set(CMAKE_CXX_STANDARD 11)

if (DEFINED ENV{BOOSTINC})
    #message("Boost Include Dir is $ENV{BOOSTINC}")
else ()
    message(FATAL_ERROR "Boost Include Dir is Null")
endif ()
include_directories($ENV{BOOSTINC})

if (DEFINED ENV{BOOSTLIB})
    #message("Boost Lib Dir is $ENV{BOOSTLIB}")
else ()
    message(FATAL_ERROR "Boost Lib Dir is Null")
endif ()
link_directories($ENV{BOOSTLIB})

#find_package(boost)
#if (boost_FOUND)
#    message(WARNING "dir = ${Boost_INCLUDE_DIR}")
#endif ()

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

include_directories($ENV{BINC64})
include_directories(. include http/server4)
link_directories(lib)
link_directories($ENV{BLIB64})

add_executable(balibali
        main.cpp
        CApplication.cpp
        MDL2Impl.cpp
        TDImpl.cpp
        LocalConfig.cpp
        http/server4/file_handler.cpp
        http/server4/mime_types.cpp
        http/server4/reply.cpp
        http/server4/request_parser.cpp
        http/server4/server.cpp
)

#add_executable(testsqlite testsqlite.cpp)
#target_link_libraries(testsqlite sqlite3)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_link_libraries(balibali PUBLIC
            lev2mdapi fasttraderapi jsoncppd httplibd tinyxml2d sqlite3d)
else ()
    target_link_libraries(balibali PUBLIC
            lev2mdapi fasttraderapi jsoncpp httplib tinyxml2 sqlite3)
endif ()

#target_link_libraries(balibali PUBLIC
#lev2mdapi fasttraderapi jsoncpp httplib tinyxml2 sqlite3)
