cmake_minimum_required(VERSION 3.17)

project(balibali_hft)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS BOOST_BIND_GLOBAL_PLACEHOLDERS)

set(Boost_USE_STATIC_LIBS ON)

if (WIN32)
    message(STATUS "windows system")
    message("Boost Path: $ENV{INCLUDE}")
    SET(BOOST_ROOT $ENV{INCLUDE})
    SET(BOOST_INCLUDEDIR ${BOOST_ROOT})
    SET(BOOST_LIBRARYDIR ${BOOST_ROOT}/lib64-msvc-14.2)
elseif(UNIX)
    message(STATUS "linux system")
    SET(BOOST_ROOT /usr/local/boost)
    SET(BOOST_INCLUDEDIR ${BOOST_ROOT}/include)
    SET(BOOST_LIBRARYDIR ${BOOST_ROOT}/lib)
endif()

find_package(Boost REQUIRED COMPONENTS program_options filesystem locale)
if (Boost_FOUND)
    message( STATUS "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}")
    message( STATUS "Boost_LIBRARY_DIRS = ${Boost_LIBRARY_DIRS}")
    message( STATUS "Boost_LIBRARIES = ${Boost_LIBRARIES}.")
    message( STATUS "Boost_LIB_VERSION = ${Boost_LIB_VERSION}.")
    include_directories(${Boost_INCLUDE_DIR})
    link_directories(${Boost_LIBRARY_DIRS})
endif ()

include_directories(. include)
link_directories(lib)

add_executable(balibali_hft
        balibali.cpp
        TickTradingFramework.cpp
        L2Impl.cpp
        TdImpl.cpp
        defines.cpp
        MemoryPool.cpp
        Strategy.cpp
        websocket_server_async.cpp
        EntryStrategy.cpp
        ExitStrategy.cpp
)

if (WIN32)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
    target_link_libraries(balibali_hft PUBLIC lev2mdapi fasttraderapi soci_core_4_0d.lib soci_sqlite3_4_0d.lib)
else ()
    target_link_libraries(balibali_hft PUBLIC lev2mdapi fasttraderapi soci_core_4_0.lib soci_sqlite3_4_0.lib)
endif ()
elseif (UNIX)
    target_link_libraries(balibali_hft PUBLIC lev2mdapi fasttraderapi soci_core soci_sqlite3 pthread iconv ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
endif ()
