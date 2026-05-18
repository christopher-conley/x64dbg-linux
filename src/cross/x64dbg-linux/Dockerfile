FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Base tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ninja-build \
    git \
    wget \
    file \
    fuse \
    libfuse2 \
    software-properties-common \
    && rm -rf /var/lib/apt/lists/*

# GCC-13
RUN add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-get update && \
    apt-get install -y --no-install-recommends gcc-13 g++-13 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100 && \
    rm -rf /var/lib/apt/lists/*

# CMake 3.25+
RUN wget -q "https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0-linux-x86_64.sh" \
    -O /tmp/cmake.sh && \
    chmod +x /tmp/cmake.sh && \
    /tmp/cmake.sh --skip-license --prefix=/usr/local && \
    rm /tmp/cmake.sh

# Qt5
RUN apt-get update && apt-get install -y --no-install-recommends \
    qtbase5-dev \
    qttools5-dev \
    libqt5svg5-dev \
    libqt5websockets5-dev \
    && rm -rf /var/lib/apt/lists/*

# Symbol parsing dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libelf-dev \
    libdw-dev \
    libunwind-dev \
    binutils-dev \
    && rm -rf /var/lib/apt/lists/*

# linuxdeploy + Qt plugin (for AppImage)
RUN wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    -O /usr/local/bin/linuxdeploy && chmod +x /usr/local/bin/linuxdeploy
RUN wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    -O /usr/local/bin/linuxdeploy-plugin-qt && chmod +x /usr/local/bin/linuxdeploy-plugin-qt

ENV APPIMAGE_EXTRACT_AND_RUN=1

WORKDIR /build
