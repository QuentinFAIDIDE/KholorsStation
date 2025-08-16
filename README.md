# Kholors Station and Sink

**THIS PROJECT IS IN EARLY ALPHA AND WAS NOT TESTED, IT MAY BE UNSTABLE OR INCOMPATIBLE WITH YOUR DAW, PLEASE REPORT ISSUES.**

Kholors is a live spectrum analyzer for your DAW, consisting of the `Kholors Station` desktop app and the `Kholors Sink` VST3 plugin. The plugin sends audio, track names, and colors from your DAW to the Station for visualization.

The Station displays a live spectrogram (left channel on top, right channel on bottom), helping you visualize frequency conflicts between tracks to aid your mixing decisions.

[![Watch the video](https://img.youtube.com/vi/9gL8FKBfXtc/maxresdefault.jpg)](https://youtu.be/9gL8FKBfXtc)

## How to Use

1.  Run the **Kholors Station** application.
2.  In your DAW, add the **Kholors Sink** VST3 plugin to each track you want to analyze (ideally all tracks or all busses).

The Station is a standalone application, so it's best used on a second monitor as a dedicated mixing visualizer.

## Downloads

Check the github releases on the right side of the Github UI to download binaries for your operating system (linux deb, linux tarball, or windows .exe).

## Compatibility

| DAW / OS      | Windows | Linux | macOS |
| ------------- | ------- | ----- | ----- |
| Bitwig Studio | ✅      | ✅    | ❓    |
| Ardour        | ❓      | ✅    | ❓    |
| Reaper        | ❓      | ✅    | ❓    |
| Zrythm        | ❓      | ❓    | ❓    |
| Waveform      | ❓      | ✅    | ❓    |
| Ableton Live  | 🐞      |       | ❓    |
| FL Studio     | ❓      |       | ❓    |
| Studio One    | ❓      |       | ❓    |

✅ = Compatible, ❌ = Not Compatible, 🐞 = Buggy, ❓ = Untested

- **Ableton**: Track names are missing [#1](https://github.com/QuentinFAIDIDE/KholorsStation/issues/1).

## Linux

### Configure, build and export IDE commands

```bash
mkdir build
cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
# to use the debug mode (with debug symbols shipped in)
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr
```

Note that you might still be missing dependencies and might need to install some packages.

### Build

```bash
make
# to do it concurrently
make -j12
```

### Run Kholors Station dev build

```bash
./src/StationApp/StationApp_artefacts/Release/KholorsStation
# or if you're using a debug build
./src/StationApp/StationApp_artefacts/Debug/KholorsStation
```

### Copy the dev build for the vst

```bash
RELEASE_TYPE="Release" # change it to "Debug" if necessary
VST3_INSTALL_LOCATION="/usr/lib/vst3" # you can also use "~/.vst3/"
rm ${VST3_INSTALL_LOCATION}/KholorsSink.vst3 -rf || true && cp -rf src/SinkPlugin/SinkPlugin_artefacts/${RELEASE_TYPE}/VST3/KholorsSink.vst3 ${VST3_INSTALL_LOCATION}/KholorsSink.vst3
```

### Generate an install tree

This will create an archive in `build/pkg` with all install files.

```bash
make DESTDIR=./pkg install -j24
```

### Install to your system

Either install with make:

```bash
sudo make install -j24
```

## Windows

### Building

The project can only be compiled with the visual studio C++ compiler, where it has been
instructed to install the Windows development kit, cmake, ninja and vcpkg. You will also need
git in your PATH in order to fetch the version tag.

A separate installation of NASM compiler is also necessary, as well as a valid time.

The best way to build the project is to use the Visual Studio Developer Terminal.

```
cmake --preset windows
cd build
ninja
```

## Updating binary data

```bash
# from the build subdirectory
cd ./../libs/JUCE/extras/BinaryBuilder/Builds/LinuxMakefile/
make
cd -
./../libs/JUCE/extras/BinaryBuilder/Builds/LinuxMakefile/build/BinaryBuilder ../src/GUIData ../src/GUIToolkit GUIData
```
