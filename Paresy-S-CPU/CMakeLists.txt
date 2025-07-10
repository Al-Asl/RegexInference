cmake_minimum_required(VERSION 3.24)

project(Paresy-S LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 0 : measurement
# 1 : log
# 2 : benchmark
add_definitions(-DMODE=1)
# only used if the mode is not set to benchmark, this is used for quick testing to write examples inside the editor
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

# Set the startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})