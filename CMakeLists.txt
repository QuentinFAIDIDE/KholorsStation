cmake_minimum_required(VERSION 3.22)

# gRPC v1.50.2 vendors c-ares 1.17.2, whose CMakeLists.txt declares support
# for CMake 3.1.  CMake 4 no longer accepts that policy baseline unless the
# parent explicitly supplies a compatible minimum.  Keep this in the project,
# rather than only in a preset, so direct IDE configurations work too.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING
    "Minimum CMake policy version accepted by bundled dependencies")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

include(FetchContent)

# Keep FetchContent state with its CMake build tree.  Separate configurations
# (for example the macOS preset and a CLion profile) can then configure or
# refresh dependencies independently without replacing each other's sources.
set(FETCHCONTENT_BASE_DIR
    "${CMAKE_BINARY_DIR}/_deps"
    CACHE PATH "External dependencies path for this build tree." FORCE)

# Use an installed package when one is available (for example through vcpkg),
# but keep a fresh checkout buildable on every supported platform without a
# package-manager-specific CMake invocation.
find_package(nlohmann_json 3.9.1 QUIET)
if(NOT nlohmann_json_FOUND)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(nlohmann_json)
endif()

option(RE2_BUILD_TESTING OFF)
option(gRPC_BUILD_TESTS OFF)
option(ZLIB_BUILD_EXAMPLES OFF)

# Juce lib setup
# JUCE 8 supports the macOS 15 SDK.  JUCE 7.0.9 calls APIs that Apple has
# removed from that SDK, preventing even CMake's helper executable from
# compiling on current macOS installations.
set(LIB_JUCE_TAG "8.0.15")
# Xcode: Disable automatic build scheme generation globally. Instead, we
# explicitely specify which targets require schemes.
set(CMAKE_XCODE_GENERATE_SCHEME OFF)
# IDEs:  Enable grouping of source files into folders in IDEs.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# IDEs:  Create a folder in the IDE with the JUCE Module code.
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS
       "Show all module sources in IDE projects" ON)
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
  PATCH_COMMAND
    ${CMAKE_COMMAND}
    -DSOURCE_DIR=<SOURCE_DIR>
    -DPATCH_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/grpc-1.50.2-abseil-macos-architectures.patch
    -DSECOND_PATCH_FILE=${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/grpc-1.50.2-macos-apple-clang.patch
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/apply_patch_if_needed.cmake
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
# muFFT's legacy option() only respects this setting when it is a cache value.
# Keep FFTW optional so a clean macOS checkout does not require Homebrew FFTW.
set(MUFFT_ENABLE_FFTW OFF CACHE BOOL "Build muFFT with FFTW support" FORCE)
FetchContent_Declare(
  muFFT
  GIT_REPOSITORY https://github.com/Themaister/muFFT.git
  GIT_TAG master)
FetchContent_MakeAvailable(muFFT)

# include the Kholors main and test targets
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

if (WIN32)
  # install VST3
  get_target_property(SINK_OUTPUT_DIR SinkPlugin ARCHIVE_OUTPUT_DIRECTORY)
  if(NOT SINK_OUTPUT_DIR)
    get_target_property(SINK_OUTPUT_DIR SinkPlugin RUNTIME_OUTPUT_DIRECTORY)
  endif()
  if(NOT SINK_OUTPUT_DIR)
    get_target_property(SINK_OUTPUT_DIR SinkPlugin LIBRARY_OUTPUT_DIRECTORY)
  endif()
  if(SINK_OUTPUT_DIR)
    install(DIRECTORY ${SINK_OUTPUT_DIR}/VST3/KholorsSink.vst3
            DESTINATION ${CMAKE_INSTALL_PREFIX}/vst3)
  endif()

  # install binary
  get_target_property(STATION_OUTPUT_DIR StationApp ARCHIVE_OUTPUT_DIRECTORY)
  if(NOT STATION_OUTPUT_DIR)
    get_target_property(STATION_OUTPUT_DIR StationApp RUNTIME_OUTPUT_DIRECTORY)
  endif()
  if(NOT STATION_OUTPUT_DIR)
    get_target_property(STATION_OUTPUT_DIR StationApp LIBRARY_OUTPUT_DIRECTORY)
  endif()
  if(STATION_OUTPUT_DIR)
    install(DIRECTORY ${STATION_OUTPUT_DIR}/KholorsStation.exe
            DESTINATION ${CMAKE_INSTALL_PREFIX}/KholorsStation.exe)
    install(DIRECTORY ${STATION_OUTPUT_DIR}/KholorsStation.pdb
            DESTINATION ${CMAKE_INSTALL_PREFIX}/KholorsStation.pdb)
  endif()


endif()

if(APPLE)
  # JUCE produces platform bundles.  A staged install mirrors the conventional
  # macOS locations; use --prefix to redirect it to a distributable image.
  install(TARGETS StationApp BUNDLE DESTINATION Applications)

  get_target_property(SINK_OUTPUT_DIR SinkPlugin LIBRARY_OUTPUT_DIRECTORY)
  if(SINK_OUTPUT_DIR)
    install(DIRECTORY ${SINK_OUTPUT_DIR}/VST3/KholorsSink.vst3
            DESTINATION Library/Audio/Plug-Ins/VST3)
    install(DIRECTORY ${SINK_OUTPUT_DIR}/AU/KholorsSink.component
            DESTINATION Library/Audio/Plug-Ins/Components)
  endif()
endif()
