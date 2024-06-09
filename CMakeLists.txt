cmake_minimum_required(VERSION 3.22)
project(Kholors2 VERSION 0.0.1)

enable_testing()

find_package(nlohmann_json 3.11.2 REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(FFTW3 REQUIRED)

include(FetchContent)
# to use gdb
set(CMAKE_BUILD_TYPE Debug)

# Juce lib setup
set(LIB_JUCE_TAG "7.0.9")
# Xcode: Disable automatic build scheme generation globally. Instead, we
# explicitely specify which targets require schemes.
set(CMAKE_XCODE_GENERATE_SCHEME OFF)
# IDEs:  Enable grouping of source files into folders in IDEs.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# IDEs:  Create a folder in the IDE with the JUCE Module code.
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS
       "Show all module sources in IDE projects" ON)
# Keep dependencies in the lib directory
set(FETCHCONTENT_BASE_DIR
    "${PROJECT_SOURCE_DIR}/libs"
    CACHE PATH "External dependencies path." FORCE)
# declare the juce dependency download
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG ${LIB_JUCE_TAG}
  GIT_SHALLOW TRUE
  GIT_CONFIG advice.detachedHead=false # Disable detached HEAD warning for
                                       # fetching a specific tag
             SOURCE_DIR
             "${FETCHCONTENT_BASE_DIR}/JUCE"
             SUBBUILD_DIR
             "${FETCHCONTENT_BASE_DIR}/JUCE-Subbuild"
             BINARY_DIR
             "${FETCHCONTENT_BASE_DIR}/JUCE-Build")

FetchContent_MakeAvailable(juce)

# gRPC lib download and setup
set(ABSL_ENABLE_INSTALL ON)
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG v1.55.0)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(gRPC)

add_subdirectory(src)
