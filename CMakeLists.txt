cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)

# ensure that git is available (we need it for version retrieval)
find_package(Git)
if(NOT GIT_EXECUTABLE)
  message(FATAL_ERROR "Missing git")
endif()

# use git describe to generate a version tag
execute_process(
  COMMAND ${GIT_EXECUTABLE} describe --tags --dirty --match "v*"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
  RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if(GIT_DESCRIBE_ERROR_CODE)
  message(FATAL_ERROR "Unable to call git describe")
endif()
# we inject the project version as a preprocessor variable into the code
add_compile_definitions(GIT_DESCRIBE_VERSION="${GIT_DESCRIBE_VERSION}")

# we don't really use this version
project(Kholors2 VERSION 0.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

enable_testing()

set(CMAKE_FIND_STATIC_PREFER TRUE)

find_package(nlohmann_json 3.9.1 REQUIRED)

include(FetchContent)

option(RE2_BUILD_TESTING OFF)
option(gRPC_BUILD_TESTS OFF)
option(ZLIB_BUILD_EXAMPLES OFF)

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
  GIT_CONFIG advice.detachedHead=false
             SOURCE_DIR
             "${FETCHCONTENT_BASE_DIR}/JUCE"
             SUBBUILD_DIR
             "${FETCHCONTENT_BASE_DIR}/JUCE-Subbuild"
             BINARY_DIR
             "${FETCHCONTENT_BASE_DIR}/JUCE-Build"
             EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(juce)

# gRPC lib download and setup
set(ABSL_ENABLE_INSTALL ON)
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG v1.50.2
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(gRPC)
set(protobuf_MSVC_STATIC_RUNTIME ON)
set(gRPC_MSVC_STATIC_RUNTIME ON)

# Configure the FetchContent for spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.1)
FetchContent_MakeAvailable(spdlog)
set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Fetchcontent for muFFT
set(MUFFT_ENABLE_FFTW OFF)
FetchContent_Declare(
  muFFT
  GIT_REPOSITORY https://github.com/Themaister/muFFT.git
  GIT_TAG master)
FetchContent_MakeAvailable(muFFT)

# include the Kholors main and test targets
add_subdirectory(src)