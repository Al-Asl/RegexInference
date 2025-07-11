cmake_minimum_required(VERSION 3.24)

project(Paresy-S LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# only work if BATCH_MODE is off, 
# this is used for quick testing to write examples inside the editor
# add_definitions(-DHARD_CODED_INPUT)

function(define_enum_option VAR_NAME DEFAULT_VALUE DESCRIPTION VALUES INDEX)
    # Create the cached variable
    set(${VAR_NAME} "${DEFAULT_VALUE}" CACHE STRING "${DESCRIPTION}")

    # Store the valid values in the cache property for GUI tools
    set_property(CACHE ${VAR_NAME} PROPERTY STRINGS ${VALUES})

    # Show status message with current value and options
    string(REPLACE ";" ", " VALUES_COMMA "${VALUES}")
    message(STATUS "${VAR_NAME} is set to: ${${VAR_NAME}}")
    message(STATUS "Valid values for ${VAR_NAME}: ${VALUES_COMMA}")

    # Validate user input
    list(FIND VALUES "${${VAR_NAME}}" _index)
    if(_index EQUAL -1)
        message(FATAL_ERROR
            "${VAR_NAME} has invalid value: '${${VAR_NAME}}'.\n"
            "Valid options are: ${VALUES_COMMA}"
        )
    endif()
    set(${INDEX} ${_index} PARENT_SCOPE)
endfunction()

message(STATUS "=================== Options ===================")

option(GUIDE_TABLE_CONSTANT_MEMORY "Allocate the guide table on constant memory" OFF)
message(STATUS "GUIDE_TABLE_CONSTANT_MEMORY is set to: ${GUIDE_TABLE_CONSTANT_MEMORY}")

option(BATCH_MODE "Process a whole directory" OFF)
message(STATUS "BATCH_MODE is set to: ${BATCH_MODE}")

option(PROFILE_MODE "Show the source code when using Nsight Compute" OFF)
message(STATUS "PROFILE_MODE is set to: ${PROFILE_MODE}")

define_enum_option(
    RELAX_UNIQUENESS_CHECK_TYPE           
    "2"                
    "The algorthim for relax uniqueness check" 
    "0;1;2;3"
    RELAX_UNIQUENESS_CHECK_TYPE_INDEX
)

define_enum_option(
    LOG_LEVEL           
    OFF                
    "Logging verbosity" 
    "OFF;DC;REI_BASIC;REI_KERNELS"
    LOG_LEVEL_INDEX
)

define_enum_option(
    CS_BIT_COUNT           
    "2048"                
    "Logging verbosity" 
    "128;256;512;1024;2048;4096"
    CS_BIT_COUNT_INDEX
)

message(STATUS "===============================================")

set(HEADERS
include/bitmask.h 
include/pair.h 
include/rei_util.hpp 
include/rei.h 
include/interval_splitter.h
include/rei_dc.hpp 
include/regex_match.hpp 
)

set(SOURCES
src/main.cu
src/rei_util.cpp 
src/rei.cu
src/rei_dc.cpp 
src/regex_match.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${SOURCES}

    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include include
        FILES
            ${HEADERS}
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE 
    LOG_LEVEL=${LOG_LEVEL_INDEX}
    CS_BIT_COUNT=${CS_BIT_COUNT_INDEX}
    RELAX_UNIQUENESS_CHECK_TYPE=${RELAX_UNIQUENESS_CHECK_TYPE_INDEX}
    $<$<BOOL:${BATCH_MODE}>:BATCH_MODE>
    $<$<BOOL:${GUIDE_TABLE_CONSTANT_MEMORY}>:GUIDE_TABLE_CONSTANT_MEMORY>
)

# cuda section
set_property(TARGET ${PROJECT_NAME} PROPERTY CUDA_ARCHITECTURES ALL)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--extended-lambda>)
target_link_libraries(${PROJECT_NAME} PRIVATE cuda cudart)
if(PROFILE_MODE)
	set(CMAKE_CUDA_FLAGS_RELEASE "${CMAKE_CUDA_FLAGS_RELEASE} --generate-line-info")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/modified_libraries)

# Set the startup project for Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# enable_testing()

# add_subdirectory(tests)