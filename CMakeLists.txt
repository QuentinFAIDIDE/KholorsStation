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

find_package(nlohmann_json 3.11.3 REQUIRED)
if(UNIX)
  find_package(FFTW3 CONFIG REQUIRED)
  set(PLATFORM_SPECIFIC_FFTW3_LIB fftw3f)
else()
  find_package(FFTW3f CONFIG REQUIRED)
  set(PLATFORM_SPECIFIC_FFTW3_LIB FFTW3::fftw3f)
endif()

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
set(protobuf_MSVC_STATIC_RUNTIME ON)
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG v1.50.2
  EXCLUDE_FROM_ALL)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(gRPC)

# Configure the FetchContent for spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.14.1)
set(SPDLOG_BUILD_SHARED OFF)
FetchContent_MakeAvailable(spdlog)
set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)

# try to get rid of some tests in zlib that are annoying me when running ctest
# set_target_properties(example example64 PROPERTIES EXCLUDE_FROM_ALL TRUE)

add_subdirectory(src)

if(UNIX AND NOT APPLE)
  # install icons
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/kholors.svg
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/kholors.png
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/pixmaps)
  # install launcher file
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/KholorsStation.desktop
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
  # install binary
  install(TARGETS StationApp DESTINATION bin)
  # install VST3 Retrieve the output directory for the target
  get_target_property(TARGET_OUTPUT_DIR SinkPlugin ARCHIVE_OUTPUT_DIRECTORY)
  if(NOT TARGET_OUTPUT_DIR)
    get_target_property(TARGET_OUTPUT_DIR SinkPlugin RUNTIME_OUTPUT_DIRECTORY)
  endif()

  if(NOT TARGET_OUTPUT_DIR)
    get_target_property(TARGET_OUTPUT_DIR SinkPlugin LIBRARY_OUTPUT_DIRECTORY)
  endif()

  if(TARGET_OUTPUT_DIR)
    install(DIRECTORY ${TARGET_OUTPUT_DIR}/VST3/KholorsSink.vst3
            DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/vst3)
  endif()

endif()
