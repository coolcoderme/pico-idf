#!/usr/bin/env bash
#
# Idempotent Cloud Agent bootstrap for pico-idf.
#
# Installs the ARM cross toolchain, Raspberry Pi Pico SDK, FreeRTOS-Kernel
# (RP2040 / RP2350 SMP ports), and picotool. Configures the claw edge_agent
# example so `pidf.py build` works immediately.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

PICO_SDK_PATH="${PICO_SDK_PATH:-/opt/pico-sdk}"
PICO_SDK_VERSION="${PICO_SDK_VERSION:-2.3.0}"
FREERTOS_KERNEL_PATH="${FREERTOS_KERNEL_PATH:-/opt/FreeRTOS-Kernel}"
FREERTOS_KERNEL_REF="${FREERTOS_KERNEL_REF:-main}"

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
if command -v update-alternatives >/dev/null 2>&1; then
    sudo update-alternatives --set cc /usr/bin/gcc || true
    sudo update-alternatives --set c++ /usr/bin/g++ || true
fi

echo "==> Ensuring Raspberry Pi Pico SDK ${PICO_SDK_VERSION} at ${PICO_SDK_PATH}"
if [ ! -d "${PICO_SDK_PATH}/.git" ]; then
    sudo mkdir -p "${PICO_SDK_PATH}"
    sudo chown -R "$(id -u):$(id -g)" "${PICO_SDK_PATH}"
    git clone --depth 1 --branch "${PICO_SDK_VERSION}" \
        https://github.com/raspberrypi/pico-sdk.git "${PICO_SDK_PATH}"
fi
git -C "${PICO_SDK_PATH}" submodule update --init --depth 1
export PICO_SDK_PATH

echo "==> Ensuring FreeRTOS-Kernel (${FREERTOS_KERNEL_REF}) at ${FREERTOS_KERNEL_PATH}"
if [ ! -d "${FREERTOS_KERNEL_PATH}/.git" ]; then
    sudo mkdir -p "${FREERTOS_KERNEL_PATH}"
    sudo chown -R "$(id -u):$(id -g)" "${FREERTOS_KERNEL_PATH}"
    git clone --depth 1 --branch "${FREERTOS_KERNEL_REF}" \
        https://github.com/raspberrypi/FreeRTOS-Kernel.git "${FREERTOS_KERNEL_PATH}"
fi
# Official FreeRTOS-Kernel keeps RP2350 ports in a submodule; the
# Raspberry Pi fork vendors RP2040 + RP2350 ARM/RISC-V together.
if [ -f "${FREERTOS_KERNEL_PATH}/.gitmodules" ]; then
    git -C "${FREERTOS_KERNEL_PATH}" submodule update --init --depth 1 \
        portable/ThirdParty/Community-Supported-Ports || true
fi
export FREERTOS_KERNEL_PATH

echo "==> Ensuring picotool is installed"
if ! command -v picotool >/dev/null 2>&1; then
    PICOTOOL_SRC="$(mktemp -d)"
    git clone --depth 1 --branch "${PICO_SDK_VERSION}" \
        https://github.com/raspberrypi/picotool.git "${PICOTOOL_SRC}"
    cmake -S "${PICOTOOL_SRC}" -B "${PICOTOOL_SRC}/build"
    cmake --build "${PICOTOOL_SRC}/build" -j"$(nproc)"
    sudo cmake --install "${PICOTOOL_SRC}/build"
    rm -rf "${PICOTOOL_SRC}"
fi

export PIDF_PATH="${REPO_DIR}"
chmod +x "${REPO_DIR}/tools/pidf.py"

echo "==> Configuring claw edge_agent example for pico_w"
"${REPO_DIR}/tools/pidf.py" -C "${REPO_DIR}/examples/claw/edge_agent" set-target pico_w
"${REPO_DIR}/tools/pidf.py" -C "${REPO_DIR}/examples/claw/edge_agent" build || true

echo "==> Done."
echo "    export PIDF_PATH=${REPO_DIR}"
echo "    export PICO_SDK_PATH=${PICO_SDK_PATH}"
echo "    export FREERTOS_KERNEL_PATH=${FREERTOS_KERNEL_PATH}"
echo "    ${REPO_DIR}/tools/pidf.py -C ${REPO_DIR}/examples/claw/edge_agent build"
