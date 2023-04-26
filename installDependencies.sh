#!/bin/bash
# Installs dependencies required for our DeepStream plugin on Ubuntu,
# either on a Jetson system or on a non-Jetson system with an NVIDIA dGPU.

# Check for cmdline argument for location of tar file of DeepStream
if [ -z "$1" ]; then
    echo "No tar file location specified, installing dependencies without DeepStream."
    TAR_SPECIFIED=false
else
    TAR_SPECIFIED=true
    TAR_FILE_PATH="$1"
    # Validate TAR_FILE_PATH:
    # Check if the file exists
    if [ ! -f "$TAR_FILE_PATH" ]; then
        echo "Error: $TAR_FILE_PATH does not exist or is not a file."
        echo "Usage: ./installDependencies path/to/file.tbz2"
        exit 1
    fi
    # Check if the file has a .tbz2 extension
    if [[ "$TAR_FILE_PATH" != *.tbz2 ]]; then
        echo "Error: $TAR_FILE_PATH is not a .tbz2 file."
        echo "Usage: ./installDependencies path/to/file.tbz2"
        exit 1
    fi
fi
# Check for cmdline argument for location of .run file (Display Driver)
if [ -z "$2" ]; then
    echo "No .run file location specified, installing dependencies without NVIDIA Driver 525.85.12."
    RUN_SPECIFIED=false
else
    RUN_SPECIFIED=true
    RUN_FILE_PATH="$2"
    # Validate RUN_FILE_PATH:
    # Check if the file exists
    if [ ! -f "$RUN_FILE_PATH" ]; then
        echo "Error: $RUN_FILE_PATH does not exist or is not a file."
        echo "Usage: ./installDependencies path/to/file.tbz2 path/to/driver.run"
        exit 1
    fi
    # Check if the file has a .run extension
    if [[ "$RUN_FILE_PATH" != *.run ]]; then
        echo "Error: $RUN_FILE_PATH is not a .run file."
        echo "Usage: ./installDependencies path/to/file.tbz2 path/to/driver.run"
        exit 1
    fi
fi

# Install CppSdk components if needed
git submodule init
git submodule update

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

# Install OpenCV dependencies if OpenCV not found or < 4.2.0
# opencv_version=$(opencv_version)
# if [[ -z "$opencv_version" ]] || [[ "$(dpkg --compare-versions "$opencv_version" lt "4.2.0"; echo $?)" -eq 0 ]]; then
#   pushd $(pwd)
#   cd ~/Documents/
#   wget -O opencv.zip https://github.com/opencv/opencv/archive/4.x.zip
#   unzip opencv.zip
#   mkdir -p build && cd build
#   cmake ../opencv-4.x
#   cmake --build .
#   popd
# fi
sudo apt-get install libopencv-dev

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
    libyaml-cpp-dev \
    libjsoncpp-dev \
    protobuf-compiler \
    gcc \
    make \
    git \
    python3

# Runs the command to check if nvidia-jetpack is found (for checking if Jetson system)
if sudo apt-cache show nvidia-jetpack 2> /tmp/apt_error && grep -q "N: Unable to locate package" /tmp/apt_error; then
  echo "nvidia-jetpack package not found, installing CUDA/drivers/TensorRT."
  JETPACK_FOUND=false
else
  echo "Found nvidia-jetpack, no need for CUDA/drivers/TensorRT installation."
  JETPACK_FOUND=true
fi
# Clean up the temporary file
rm /tmp/apt_error

if $JETPACK_FOUND; then
  # Handle Jetson installation:
  if $TAR_SPECIFIED; then
    sudo tar -xvf "$TAR_FILE_PATH" -C /
    cd /opt/nvidia/deepstream/deepstream-6.2
    sudo ./install.sh
    sudo ldconfig
    echo "Finished installation of dependencies."
    deepstream-app --version
  else
    echo "Dependencies installed, now install DeepStream from NVIDIA's website."
  fi
else
  # Handle non-Jetson installation: (dGPU platform)

  # CUDA Toolkit 11.8
  sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/3bf863cc.pub
  sudo add-apt-repository "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/ /"
  sudo apt-get update
  sudo apt-get install -y cuda-toolkit-11-8

  # NVIDIA driver 525.85.12
  if $RUN_SPECIFIED; then
    # kill services prior to installation of driver
    sudo service gdm stop 
    sudo service lightdm stop 
    sudo pkill -9 Xorg
    # Set permissions and run NVIDIA installer
    chmod 755 "$RUN_FILE_PATH"
    RUN_DIR="$(dirname "$RUN_FILE_PATH")"
    cd "$RUN_DIR"
    sudo ./NVIDIA-Linux-x86_64-525.85.12.run --no-cc-version-check
    # restart killed services
    sudo service gdm start
    sudo service lightdm start
  fi

  # TensorRT 8.5.2.2
  sudo apt-get install libnvinfer8=8.5.2-1+cuda11.8 libnvinfer-plugin8=8.5.2-1+cuda11.8 libnvparsers8=8.5.2-1+cuda11.8 \
  libnvonnxparsers8=8.5.2-1+cuda11.8 libnvinfer-bin=8.5.2-1+cuda11.8 libnvinfer-dev=8.5.2-1+cuda11.8 \
  libnvinfer-plugin-dev=8.5.2-1+cuda11.8 libnvparsers-dev=8.5.2-1+cuda11.8 libnvonnxparsers-dev=8.5.2-1+cuda11.8 \
  libnvinfer-samples=8.5.2-1+cuda11.8 libcudnn8=8.7.0.84-1+cuda11.8 libcudnn8-dev=8.7.0.84-1+cuda11.8 \
  python3-libnvinfer=8.5.2-1+cuda11.8 python3-libnvinfer-dev=8.5.2-1+cuda11.8

  # Finally, unpack DeepStream tar file.
  if $TAR_SPECIFIED; then
    sudo tar -xvf "$TAR_FILE_PATH" -C /
    cd /opt/nvidia/deepstream/deepstream-6.2
    sudo ./install.sh
    sudo ldconfig
    echo "Finished installation of dependencies."
    deepstream-app --version
  else
    echo "Dependencies installed, now install DeepStream from NVIDIA's website."
  fi
fi
