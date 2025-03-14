juce_add_plugin(
  StationPlugin
  FORMATS
  VST3 # The formats to build. Valid formats: Standalone Unity VST3 AU AUv3 AAX
       # VST LV2. AU and AUv3 plugins will only be enabled when building on
       # macOS.
  PRODUCT_NAME
  "kholors-station-v1" # The name of the final executable, which can differ from the
  # target name. 
  PLUGIN_NAME # Name of the plugin
  # that will be displayed in the DAW. Can differ from PRODUCT_NAME. 
  "Kholors Station v1"
  # ICON_BIG #
  # ICON_* arguments specify a path to an image file to use as an icon for the
  # Standalone. ICON_SMALL
  COMPANY_NAME
  artifaktnd # The name of this target's author. The value is inherited
                  # from JUCE_COMPANY_NAME.
  # COMPANY_WEBSITE                           # The address of a website related
  # to this target in some way. The value is inherited from
  # JUCE_COMPANY_WEBSITE. COMPANY_EMAIL                             # An email
  # address for this target's author. The value is inherited from
  # JUCE_COMPANY_EMAIL.
  PLUGIN_MANUFACTURER_CODE
  Artk # A four-character manufacturer id with at least one upper-case
       # character. GarageBand 10.3 requires the first letter to be upper-case,
       # and the remaining letters to be lower-case.
  PLUGIN_CODE
  Khst # A unique four-character plugin id with exactly one upper-case
       # character. GarageBand 10.3 requires the first letter to be upper-case,
       # and the remaining letters to be lower-case.
  IS_SYNTH
  FALSE # Is this a synth or an effect?
  NEEDS_MIDI_INPUT
  FALSE # Does the plugin need midi input?
  NEEDS_MIDI_OUTPUT
  FALSE # Does the plugin need midi output?
  IS_MIDI_EFFECT
  FALSE # Is this plugin a MIDI effect?
  EDITOR_WANTS_KEYBOARD_FOCUS
  FALSE # Does the editor need keyboard focus?
  VST3_CATEGORIES
  Analyzer # Should be one or more, separated by spaces, of the following: Fx,
           # Instrument, Analyzer, Delay, Distortion, Drum, Dynamics, EQ,
           # External, Filter, Generator, Mastering, Modulation, Mono, Network,
           # NoOfflineProcess, OnlyOfflineProcess, OnlyRT, Pitch Shift,
           # Restoration, Reverb, Sampler, Spatial, Stereo, Surround, Synth,
           # Tools, Up-Downmix
  AU_MAIN_TYPE
  kAudioUnitType_MusicEffect # Should be one or more, separated by spaces, of
                             # the following: kAudioUnitType_Effect,
                             # kAudioUnitType_FormatConverter,
                             # kAudioUnitType_Generator,
                             # kAudioUnitType_MIDIProcessor,
                             # kAudioUnitType_Mixer, kAudioUnitType_MusicDevice,
                             # kAudioUnitType_MusicEffect,
                             # kAudioUnitType_OfflineEffect,
                             # kAudioUnitType_Output, kAudioUnitType_Panner
  COPY_PLUGIN_AFTER_BUILD
  FALSE) # Should the plugin be installed to a default location after building?
         # NOTE: Writing to the default install locations might require
         # administrator privileges on Windows.

# Set the C++ language standard requirenment for the "shared code" library
# target. Setting this to PUBLIC ensures that all dependent targets will inherit
# the specified C++ standard.
target_compile_features(StationPlugin PUBLIC cxx_std_17)

# Disable compiler extensions for the project targets (e.g. use -std=c++17
# instead of -std=gnu++17).
get_property(
  project_targets
  DIRECTORY "${PROJECT_SOURCE_DIR}"
  PROPERTY BUILDSYSTEM_TARGETS)

set_target_properties(${project_targets} PROPERTIES CXX_EXTENSIONS OFF)

file(
  GLOB_RECURSE all_sources
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.cpp")
list(FILTER all_sources EXCLUDE REGEX ".*Test\\.cpp$")

file(
  GLOB_RECURSE all_headers
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.h")
list(FILTER all_headers EXCLUDE REGEX ".*Test\\.h$")

target_sources(StationPlugin PRIVATE ${all_sources} ${all_headers})

target_compile_definitions(
  StationPlugin
  PUBLIC # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you
         # might not need them.
         JUCE_WEB_BROWSER=0 # If you remove this, add `NEEDS_WEB_BROWSER TRUE`
                            # to the `juce_add_plugin` call
         JUCE_USE_CURL=0 # If you remove this, add `NEEDS_CURL TRUE` to the
                         # `juce_add_plugin` call
         JUCE_VST3_CAN_REPLACE_VST2=0
         # JUCE_DISPLAY_SPLASH_SCREEN=0
         JUCE_MODAL_LOOPS_PERMITTED=1
)

target_link_libraries(
  StationPlugin
  PRIVATE GUIToolkit
          juce::juce_audio_utils
          juce::juce_gui_extra
          juce::juce_audio_basics
          juce::juce_opengl
          juce::juce_cryptography
          AudioTransport
          TaskManagement
          Utils
          nlohmann_json::nlohmann_json
          spdlog::spdlog
          muFFT
  PUBLIC juce::juce_recommended_config_flags
         juce::juce_recommended_lto_flags
         juce::juce_recommended_warning_flags)