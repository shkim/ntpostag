cmake_minimum_required(VERSION 3.8)
set(PROJECT_NAME_STR ntpostag)
project(${PROJECT_NAME_STR} C CXX)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-deprecated -pthread)
endif()

if(MSVC)
    set(MSVC_COMPILER_DEFS "-D_VARIADIC_MAX=10")
elseif(APPLE)
    set(BOOST_ROOT /usr/local/opt/boost@1.59)
    set(OPENSSL_ROOT /usr/local/opt/openssl)
    set(PYTHON_HOME /usr/local/Cellar/python3/3.6.3/Frameworks/Python.framework/Versions/3.6)
elseif(UNIX)
    #set(Boost_USE_MULTITHREADED OFF)
endif()

set(CMAKE_CXX_STANDARD 14)

set(CPPREST_ROOT ${PROJECT_SOURCE_DIR}/casablanca)

find_package(Threads REQUIRED)
#find_package(Boost COMPONENTS system filesystem thread-mt chrono REQUIRED)
find_package(Boost COMPONENTS system filesystem chrono REQUIRED)

set(COMMON_INCDIRS ntpostag ${BOOST_ROOT}/include ${OPENSSL_ROOT}/include ${CPPREST_ROOT}/Release/include)
set(COMMON_LIBDIRS ${BOOST_ROOT}/lib ${OPENSSL_ROOT}/lib ${CPPREST_ROOT}/build.debug/Binaries)

set(PYMOD_INCDIRS ${PYTHON_HOME}/include/python3.6m)
set(PYMOD_LIBDIRS ${PYTHON_HOME}/lib /usr/local/opt/boost-python@1.59/lib)

include_directories(${COMMON_INCDIRS})
link_directories(${COMMON_LIBDIRS} ${PYMOD_LIBDIRS})
link_libraries(cpprest crypto ssl boost_python python3.6m)

set(SOURCE_FILES
    ntpostag/stdafx.h
    ntpostag/compat.h
    ntpostag/util.h
    ntpostag/posdic.h
    ntpostag/postag.h

    ntpostag/conjugate.cpp
    ntpostag/character.cpp
    ntpostag/paragraph.cpp
    ntpostag/bpython.cpp
    ntpostag/main.cpp
    ntpostag/makedic.cpp
    ntpostag/restapi.cpp
    ntpostag/sentence.cpp
    ntpostag/token.cpp
    ntpostag/worddic.cpp
    ntpostag/util.cpp
)

add_executable(${PROJECT_NAME_STR} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME_STR} ${Boost_LIBRARIES})

enable_testing()
add_subdirectory(${PROJECT_SOURCE_DIR}/extern/gtest)
set(PROJECT_TEST_NAME ${PROJECT_NAME_STR}_test)
include_directories(${GTEST_INCLUDE_DIRS} ${COMMON_INCDIRS} ${PYMOD_INCDIRS})
add_definitions(-D_UNITTEST)

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/tests/*.cpp)

list(REMOVE_ITEM SOURCE_FILES ntpostag/main.cpp)
add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES} ${SOURCE_FILES})
add_dependencies(${PROJECT_TEST_NAME} googletest)

target_link_libraries(${PROJECT_TEST_NAME} ${Boost_LIBRARIES}
    ${GTEST_LIBS_DIR}/libgtest.a
    ${GTEST_LIBS_DIR}/libgtest_main.a
)

#target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})

add_test(test1 ${PROJECT_TEST_NAME})
