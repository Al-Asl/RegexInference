cmake_minimum_required(VERSION 3.24)

project(ParesyDC LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# add_definitions(-DBATCH_MODE)
# add_definitions(-DMEASUREMENT_MODE)
# only used if the mode is not set to benchmark, this is used for quick testing to write examples inside the editor
add_definitions(-DUSER_INPUT)
add_definitions(-DRELAX_UNIQUENESS_CHECK_TYPE=2)

set(SOURCES
bitmask.h 
pair.h 
rei_util.hpp 
rei_util.cpp 
rei.h 
rei.cu
interval_splitter.h
dc_paresy.hpp 
dc_paresy.cpp 
regex_match.hpp 
regex_match.cpp
)

add_executable(${PROJECT_NAME} main.cu ${SOURCES})

# cuda section
set_property(TARGET ${PROJECT_NAME} PROPERTY CUDA_ARCHITECTURES 75)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--extended-lambda>)
target_link_libraries(${PROJECT_NAME} PRIVATE cuda cudart)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/modified_libraries)

# Set the startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# enable_testing()

# add_subdirectory(tests)