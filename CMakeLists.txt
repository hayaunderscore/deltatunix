cmake_minimum_required(VERSION 3.12)
include(cmake/CPM.cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_SYSTRAY "Builds the system tray component (requires Qt)" ON)

# Used for versioning purposes
set(VERSION_OUT_DIR    "${CMAKE_BINARY_DIR}/sources/")
set(VERSION_PREFIX     "DELTATUNIX_")
set(VERSION_H_FILENAME "version.hpp")
set(VERSION_NAMESPACE  "deltatunix::version")
CPMAddPackage("gh:BareCpper/Version.cmake@0.4")

project(deltatunix VERSION ${VERSION_SEMANTIC} LANGUAGES CXX)

if(BUILD_SYSTRAY)
	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTORCC ON)
	# Find Qt 6
	find_package(Qt6 REQUIRED COMPONENTS Widgets)
endif()

# Libraries

# raylib
CPMAddPackage(
	URI "gh:raysan5/raylib#6.0"
	OPTIONS "BUILD_EXAMPLES OFF" "BUILD_GAMES OFF" "GLFW_BUILD_WAYLAND OFF"
)
# ckdl
CPMAddPackage(
	NAME ckdl
	GITHUB_REPOSITORY "tjol/ckdl"
	GIT_TAG main
	OPTIONS "BUILD_TESTS OFF"
)
execute_process(
	COMMAND git rev-parse --short HEAD
	WORKING_DIRECTORY ${ckdl_SOURCE_DIR}
	OUTPUT_VARIABLE CKDL_COMMIT_SHA
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

# sdbus-cpp
find_package(sdbus-c++ REQUIRED)

# Adding our source files
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/sources/*.cpp") # Define PROJECT_SOURCES as a list of all source files
set(PROJECT_INCLUDE "${CMAKE_CURRENT_LIST_DIR}/sources/") # Define PROJECT_INCLUDE to be the path to the include directory of the project

# Declaring our executable
if(NOT VERSION_SET)
    message(FATAL_ERROR "Version.cmake is required")
endif()
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
target_link_libraries(${PROJECT_NAME} PRIVATE raylib SDBusCpp::sdbus-c++ kdl kdlpp version::version)
if(BUILD_SYSTRAY)
	target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)
	target_compile_definitions(${PROJECT_NAME} PUBLIC BUILD_SYSTRAY)
	qt_add_resources(RESOURCE_FILES resources.qrc)
	target_sources(${PROJECT_NAME} PRIVATE ${RESOURCE_FILES})
endif()

# Make sure the executable lands in the release folder
set_target_properties(${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/release"
)

# Also set the .x86_64 extensions for linux
if((LINUX) AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
	message(STATUS "Using .x86_64 extension for Linux builds")
	set(CMAKE_EXECUTABLE_SUFFIX ".x86_64")
endif()

# Setting various compile definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="resources/")
target_compile_definitions(${PROJECT_NAME} PUBLIC SDBUSCPP_VERSION="${sdbus-c++_VERSION}")
target_compile_definitions(${PROJECT_NAME} PUBLIC CKDL_VERSION="1.0-${CKDL_COMMIT_SHA}")
