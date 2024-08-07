# Kholors Station and Sink

Kholors is a project made of Kholors Station, a desktop software that
receives and display signals from a DAW, and Kholors Sink, a VST3 plugin
that sends the signals from the DAW.

This project is building both.

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
