#!/bin/bash

# Install minimal prerequisites
sudo apt update
sudo apt install -y cmake g++ wget unzip

# Install GStreamer dependencies
sudo apt install -y \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-x \
    gstreamer1.0-alsa \
    gstreamer1.0-gl \
    gstreamer1.0-gtk3 \
    gstreamer1.0-qt5 \
    gstreamer1.0-pulseaudio

# Install OpenCV dependencies
pushd $(pwd)
cd ~/Documents/
wget -O opencv.zip https://github.com/opencv/opencv/archive/4.x.zip
unzip opencv.zip
mkdir -p build && cd build
cmake ../opencv-4.x
cmake --build .
popd

# Install DeepStream dependencies
sudo apt install -y \
    libssl1.1 \
    libgstreamer1.0-0 \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    libgstrtspserver-1.0-0 \
    libjansson4 \
    libyaml-cpp-dev
