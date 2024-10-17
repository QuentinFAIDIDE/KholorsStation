FROM docker.io/debian:bullseye-slim

RUN apt-get update -y
RUN apt-get upgrade -y

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential wget gcc pkg-config make git clang nlohmann-json3-dev ccache libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype-dev libglx-dev libgl-dev libasound2-dev
RUN wget https://github.com/Kitware/CMake/releases/download/v3.31.0-rc1/cmake-3.31.0-rc1-linux-x86_64.sh
RUN chmod +x cmake-3.31.0-rc1-linux-x86_64.sh
RUN ./cmake-3.31.0-rc1-linux-x86_64.sh --skip-license --prefix=/usr

WORKDIR /src/app