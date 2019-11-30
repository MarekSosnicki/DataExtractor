cmake_minimum_required(VERSION 3.2)
project(${CMAKE_PROJECT_NAME}_Test)

add_subdirectory(gmock)

include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${gmock_SOURCE_DIR}/include)
include_directories(./test_utils)
include_directories(../DataExtractor)

aux_source_directory(./test_cases test_cases)
aux_source_directory(./test_utils test_utils)

add_executable(${PROJECT_NAME} ${test_cases} ${test_utils})
target_link_libraries(${PROJECT_NAME}
    gtest_main
    gmock_main
#    ${CMAKE_PROJECT_NAME}_Sources
    )

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Testing")
set_property(TARGET gtest_main PROPERTY FOLDER "Testing/gtest")
set_property(TARGET gmock_main PROPERTY FOLDER "Testing/gtest")
set_property(TARGET gtest PROPERTY FOLDER "Testing/gtest")
set_property(TARGET gmock PROPERTY FOLDER "Testing/gtest")
