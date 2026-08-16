#!/usr/bin/env bash
#
# Idempotent Cloud Agent bootstrap for the pico-idf firmware project.
#
#   * installs the ARM cross toolchain used to build RP2040/RP2350 firmware
#   * makes the GNU toolchain the default host compiler (the base image's
#     default `cc`/`c++` point at a clang that cannot find libstdc++, which
#     breaks building host tools such as picotool)
#   * clones/pins the Raspberry Pi Pico SDK into a persistent path
#   * builds and installs picotool once so clean builds don't re-fetch it
#   * configures the CMake build tree so `cmake --build build` works immediately
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

PICO_SDK_PATH="${PICO_SDK_PATH:-/opt/pico-sdk}"
PICO_SDK_VERSION="${PICO_SDK_VERSION:-2.3.0}"

echo "==> Installing toolchain and host build dependencies"
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    cmake \
    build-essential \
    python3 \
    git \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    libusb-1.0-0-dev \
    pkg-config

echo "==> Selecting the GNU toolchain as the default host compiler"
# The base image's default cc/c++ resolve to a clang install that cannot
# locate libstdc++ headers/libs, which breaks CMake's host-tool builds.
sudo update-alternatives --set cc /usr/bin/gcc
sudo update-alternatives --set c++ /usr/bin/g++

echo "==> Ensuring Raspberry Pi Pico SDK ${PICO_SDK_VERSION} at ${PICO_SDK_PATH}"
if [ ! -d "${PICO_SDK_PATH}/.git" ]; then
    sudo mkdir -p "${PICO_SDK_PATH}"
    sudo chown -R "$(id -u):$(id -g)" "${PICO_SDK_PATH}"
    git clone --depth 1 --branch "${PICO_SDK_VERSION}" \
        https://github.com/raspberrypi/pico-sdk.git "${PICO_SDK_PATH}"
fi
git -C "${PICO_SDK_PATH}" submodule update --init --depth 1
export PICO_SDK_PATH

echo "==> Ensuring picotool ${PICO_SDK_VERSION} is installed"
# Installing picotool system-wide lets the SDK find it via find_package and
# skip building it from source inside every project build.
if ! command -v picotool >/dev/null 2>&1; then
    PICOTOOL_SRC="$(mktemp -d)"
    git clone --depth 1 --branch "${PICO_SDK_VERSION}" \
        https://github.com/raspberrypi/picotool.git "${PICOTOOL_SRC}"
    cmake -S "${PICOTOOL_SRC}" -B "${PICOTOOL_SRC}/build"
    cmake --build "${PICOTOOL_SRC}/build" -j"$(nproc)"
    sudo cmake --install "${PICOTOOL_SRC}/build"
    rm -rf "${PICOTOOL_SRC}"
fi

echo "==> Configuring CMake build tree"
cmake -S "${REPO_DIR}" -B "${REPO_DIR}/build"

echo "==> Done. Build with: cmake --build ${REPO_DIR}/build"
