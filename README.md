# Kholors Station: Make better mixdowns

Achieving a perfect mxidown is hard, and there is no silver bullet. Juggling with multiple analyzer windows is a pain, and it's hard to see how different tracks are interacting with each other.

**Kholors** solves this by giving you a single, comprehensive view of your entire mix's frequency spectrum, inside a window separate from your DAW that you can run on an adjscent screen or desktop. It helps you visually identify frequency clashes, tame resonant peaks, and ultimately make more informed EQ and mixing decisions to achieve a cleaner, more professional sound.

It's composed of two parts:

- **`Kholors Station`**: A standalone desktop application that provides the main visualization.
- **`Kholors Sink`**: A lightweight VST3 plugin that you add to tracks in your DAW.

The `Sink` plugin sends audio, track names, and even your DAW's track colors to the `Station`. The result is a color-coded spectrogram where you can see exactly what's happening in your mix, track by track.

[!Watch the video](https://youtu.be/9gL8FKBfXtc)

> **⚠️ EARLY ALPHA WARNING:** This project is in its early stages and has not been widely tested. It may be unstable or incompatible with your DAW. Please report any issues you encounter.

## Getting Started

### 1. Download

Head over to the **Releases Page** to grab the latest version for your operating system.

### 2. Install

#### Windows

Run the downloaded `.exe` installer and follow the on-screen instructions.

#### Linux

You have two options:

- **DEB Package (Recommended for Debian/Ubuntu-based systems):**

  ```bash
  sudo apt install ./kholors-*.deb
  ```

  Or simply double-click the file to install it through your graphical package manager.

- **Tarball Archive:**
  Extract the downloaded `.tar.gz` file and run the installer script as root.
  ```bash
  tar -xzvf kholors_*.tar.gz
  cd kholors_*
  sudo ./install.sh
  ```
  To uninstall, run `sudo ./uninstall.sh` from the same directory.

### 3. How to Use

1.  Run the **Kholors Station** application. It's best used on a second monitor as a dedicated mixing visualizer.
2.  In your DAW, add the **Kholors Sink** VST3 plugin to each track you want to analyze (ideally all tracks or all busses).

That's it! You should now see the audio from your DAW being visualized in the Kholors Station.

## Compatibility

We're working on testing Kholors with as many DAWs as possible. Here's the current status. If your DAW is marked with a ❓, please give it a try and let us know how it works.

| DAW / OS      | Windows | Linux | macOS |
| ------------- | ------- | ----- | ----- |
| Bitwig Studio | ✅      | ✅    | ❓    |
| Ardour        | ❓      | ✅    | ❓    |
| Reaper        | ❓      | ✅    | ❓    |
| Zrythm        | ❓      | ❓    | ❓    |
| Waveform      | ❓      | ✅    | ❓    |
| Ableton Live  | ✅      |       | ❓    |
| FL Studio     | ❓      |       | ❓    |
| Studio One    | ❓      |       | ❓    |

**Legend:** ✅ = Compatible, ❌ = Not Compatible, 🐞 = Buggy, ❓ = Untested

---

## For Developers: Building from Source

Interested in contributing or just want to build it yourself? Here's how.

### Linux

**1. Configure:**

```bash
mkdir build && cd build

# Configure for a release build. You may need to install dependencies like
# build-essential, git, libx11-dev, libxrandr-dev, libxinerama-dev, libxcursor-dev, libfreetype-dev
# If you are on a debian based distro, check the basebuilder.Containerfile
cmake .. -DCMAKE_BUILD_TYPE=Release
# or you might need to set CMAKE_EXPORT_COMPILE_COMMANDS for your IDE and CMAKE_POLICY_VERSION_MINIMUM for CMake compatibility
# with newer versions.
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

**2. Build:**

```bash
make -j$(nproc)
```

**3. Run & Install Dev Build:**

```bash
# Run the Station app
./src/StationApp/StationApp_artefacts/Release/KholorsStation
# or
./src/StationApp/StationApp_artefacts/Debug/KholorsStation

# Copy the VST3 plugin to a standard location
VST3_INSTALL_LOCATION="$HOME/.vst3" # or "/usr/lib/vst3" or "/usr/local/lib/vst3"  for system-wide
mkdir -p "${VST3_INSTALL_LOCATION}"
cp -r src/SinkPlugin/SinkPlugin_artefacts/Release/VST3/KholorsSink.vst3 "${VST3_INSTALL_LOCATION}/"

# Copy the Binary to whathever suits your
```

**4. Create an Installable Package:**

From the `build` directory:

```bash
make DESTDIR=./pkg install
```

### Windows

**Prerequisites:**

- Visual Studio with C++ toolchain
- Windows SDK
- CMake, Ninja, vcpkg
- Git and NASM must be in your system's PATH.

**Build:**

Use the Visual Studio Developer Terminal to run the following commands:

```powershell
cmake --preset windows
cd build
ninja
```

### Updating GUI Binary Data

If you modify assets in `res/`, you need to rebuild the binary data file that embeds them in the application. From the `build` directory:

```bash
./../libs/JUCE/extras/BinaryBuilder/Builds/LinuxMakefile/build/BinaryBuilder ../src/GUIData ../src/GUIToolkit GUIData
```
