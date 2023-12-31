﻿cmake_minimum_required(VERSION 3.17)

project(daban)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS BOOST_BIND_GLOBAL_PLACEHOLDERS)

set(Boost_USE_STATIC_LIBS ON)

if (WIN32)
    message(STATUS "windows system")
    #SET(BOOST_ROOT I:/tanisme/workspace/cpplib/boost_1_73_0)
    SET(BOOST_ROOT E:/tangang/workspace/cpplib/boost_1_73_0_142_64)
    SET(BOOST_INCLUDEDIR ${BOOST_ROOT})
    SET(BOOST_LIBRARYDIR ${BOOST_ROOT}/lib64-msvc-14.2)
elseif(UNIX)
    message(STATUS "linux system")
    SET(BOOST_ROOT /usr/local/boost)
    SET(BOOST_INCLUDEDIR ${BOOST_ROOT}/include)
    SET(BOOST_LIBRARYDIR ${BOOST_ROOT}/lib)
endif()

find_package(Boost REQUIRED COMPONENTS program_options filesystem)
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

add_executable(daban
        main.cpp
        CApplication.cpp
        MDL2Impl.cpp
        TDImpl.cpp
        Imitate.cpp
        defines.cpp
        MemoryPool.cpp
)

if (WIN32)
    target_link_libraries(daban PUBLIC lev2mdapi fasttraderapi)
elseif (UNIX)
    target_link_libraries(daban PUBLIC lev2mdapi fasttraderapi pthread ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
endif ()
