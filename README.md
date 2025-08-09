# Kholors Station and Sink

Kholors is a project made of Kholors Station, a desktop software that
receives and display signals from a DAW, and Kholors Sink, a VST3 plugin
that sends the signals from the DAW, and automatically colors and 
names the track based on the track metadata. This project is building both.

![Watch the video](https://artifaktnd.com/kholors_video_demo.webm)

The use case is to visualise conflicts between tracks, at specific time/frequencies
where they overlap, and to know where to cut or where there is space in the mix
A [demo video](https://youtu.be/9gL8FKBfXtc) is available.


The Station (viewer) being an independant app not integrated to the DAW,
you might run into usability issues using it on a single small screen desktop,
as it is designed to run on a separate screen from the DAW, as an always-open
mixdown visualizer.


Note that the project is in early development, and while usable and perceived 
as a vital tool with no equivalent by the developer, it
was only tested on Linux and Windows with Bitwig. Feel free to open issues
on Github if you run into issues.


Moreover, a refactor is planned to make it function a bit like the Q-Pro eq,
entirely within the DAW and where each track has a VST instance capable of showing
all tracks spectrums, although it is not sure as to whem this will be ready.

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
