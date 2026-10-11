#!/bin/sh
# Build a project for the AN521 board and run it in QEMU, inside the course container.
# Usage: ./run.sh [project folder]    (default: the folder this script is in)
set -eu

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
APP_DIR=$(cd "${1:-${SCRIPT_DIR}}" && pwd)
IMAGE="${IMAGE:-zephyr-sb:3.7.2}"
TIMEOUT="${TIMEOUT:-10}"
BOARD="mps2/an521/cpu0"

if ! docker image inspect "${IMAGE}" >/dev/null 2>&1; then
    echo "error: docker image ${IMAGE} not found, build it with:" >&2
    echo "  docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -t ${IMAGE} ${SCRIPT_DIR}/docker" >&2
    exit 1
fi

if [ -t 0 ] && [ -t 1 ]; then
    TTY_FLAGS="-it"
else
    TTY_FLAGS=""
fi

exec docker run --rm ${TTY_FLAGS} \
    --volume "${APP_DIR}:/opt/zephyr-ws/app" \
    "${IMAGE}" \
    bash -c '
        set -e
        west build -d build -p always -b "$1" .

        echo
        echo "=== Running in QEMU for $2 seconds ==="
        status=0
        timeout --foreground "$2" \
            "${ZEPHYR_SDK_INSTALL_DIR}/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-arm" \
            -machine mps2-an521 -cpu cortex-m33 -m 16 -nographic -vga none \
            -device loader,file=build/zephyr/zephyr.elf || status=$?

        # 124 means the timeout stopped QEMU: the board has no power-off, so this is a normal end.
        [ "${status}" -eq 124 ] || exit "${status}"
    ' run "${BOARD}" "${TIMEOUT}"
