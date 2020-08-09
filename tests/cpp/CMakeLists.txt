cmake_minimum_required(VERSION 3.0)
project(gmumr_tests)

file (GLOB TEST_SOURCES
    "${TESTS_DIR}/ann/ann.cpp")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${R_LD_FLAGS}")
set(CMAKE_CXX_FLAGS "${R_CXX_FLAGS} -std=c++14")
add_definitions(-DTEST_DATA_PATH="${TEST_DATA_PATH}")
add_executable(gmumr_tests ${TEST_SOURCES}) 
target_link_libraries(gmumr_tests gmum.r gtest gtest_main pthread -lbenchmark)
INSTALL(TARGETS gmumr_tests DESTINATION ${TESTS_DIR})
