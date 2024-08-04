FROM docker.io/ubuntu:oracular-20240617

RUN apt-get update -y
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential gcc pkg-config cmake make git libspdlog-dev clang nlohmann-json3-dev ccache libfftw3-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype-dev libglx-dev libgl-dev libasound2-dev

WORKDIR /src/app