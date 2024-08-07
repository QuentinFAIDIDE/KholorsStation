#!/bin/sh

if [ "$(id -u)" -ne 0 ]; then
        echo 'This script must be run by root' >&2
        exit 1
fi

rm -rf /usr/share/icons/hicolor/scalable/apps/kholors.svg
rm -rf /usr/share/pixmaps/kholors.png
rm -rf /usr/share/applications/KholorsStation.desktop
rm -rf /usr/bin/KholorsStation
rm -rf /usr/lib/vst3/KholorsSink.vst3