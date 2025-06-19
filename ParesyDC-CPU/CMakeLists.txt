cmake_minimum_required(VERSION 3.24)

project(ParesyDC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(-DMEASUREMENT_MODE=0)
add_definitions(-DUSER_INPUT=0)

set(SOURCES
bitMask128.hpp 
rei_util.hpp 
rei_util.cpp 
rei.hpp 
rei.cpp 
dc_paresy.hpp 
dc_paresy.cpp 
regex_match.hpp 
regex_match.cpp
)

add_executable(${PROJECT_NAME} main.cpp ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})