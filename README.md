# Kholors Station and Sink

**THIS PROJECT IS IN EARLY ALPHA AND WAS NOT TESTED, IT MAY BE UNSTABLE OR INCOMPATIBLE WITH YOUR DAW, PLEASE REPORT ISSUES.**

Kholors is a project made of Kholors Station, a desktop software that
receives and display signals from a DAW, and Kholors Sink, a VST3 plugin
that sends the signals from the DAW, and automatically colors and
names the track based on the track metadata. This project is building both.

The spectrum is drawing live as your daw project is playing, with the top half
being the left track drawn, and the bottom half being the right track.
The centered horizontal line is the 0Hz frequency, while the top is the maximum displayable
(Nyquist) frequency of the left track, and the bottom the maximum displayable frequency
of the right track. THe viewer horizontal axis is just time, similar to what your DAW is showing.

[![Watch the video](https://img.youtube.com/vi/9gL8FKBfXtc/maxresdefault.jpg)](https://youtu.be/9gL8FKBfXtc)

The use case is to visualise conflicts between tracks, at specific time/frequencies
where they overlap, and to know where to cut or where there is space in the mix
A [demo video](https://youtu.be/9gL8FKBfXtc) is available.

To use it, open the Kholors Station app, and open your DAW, then for each track or group,
add an instance of the Kholors Sink plugin, which will create a new coloured track live drawn
on the Station app.

The Station (viewer) being an independant app not integrated to the DAW,
you might run into usability issues using it on a single small screen desktop,
as it is designed to run on a separate screen from the DAW, as an always-open
mixdown visualizer.

## Downloads

Check the github releases on the right side of the Github UI to download binaries for your operating system (linux deb, linux tarball, or windows .exe).

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
