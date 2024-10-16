FROM docker.io/debian:bullseye-slim

RUN apt-get update -y
RUN apt-get upgrade -y

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y build-essential gcc pkg-config cmake make git clang nlohmann-json3-dev ccache libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype-dev libglx-dev libgl-dev libasound2-dev

WORKDIR /src/app