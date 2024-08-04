#!/bin/sh

if [ "$(id -u)" -ne 0 ]; then
        echo 'This script must be run by root' >&2
        exit 1
fi

chown -R root:root tree/usr
chmod -R 533 tree/usr