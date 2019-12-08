cmake_minimum_required(VERSION 3.2)
project(DataExtractor)
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=std++17)
    add_definitions(-D_stricmp=stricmp)
endif()
set_property( GLOBAL PROPERTY USE_FOLDERS ON)
add_definitions(-std=gnu++17)

file(GLOB src_files  *.cpp *.h)

set(testDataPath ${CMAKE_CURRENT_SOURCE_DIR}/TestData)
message(Test data path:  ${testDataPath})
add_definitions( -DTEST_DATA_PATH="${testDataPath}")


add_subdirectory(Test)
include_directories(DataExtractor)
add_executable(${PROJECT_NAME} main.cpp DataExtractor/DataExtractor.hpp)
#target_link_libraries(${CMAKE_PROJECT_NAME} ${CMAKE_PROJECT_NAME}_Sources)