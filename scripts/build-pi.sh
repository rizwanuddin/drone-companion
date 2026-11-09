#!/bin/bash
set -e

docker build --platform linux/arm64 -t drone-pi-builder -f docker/Dockerfile.pi .

docker run --rm --platform linux/arm64 \
    -v "$(pwd)":/project \
    drone-pi-builder \
    bash -c "cmake -S . -B build-pi && cmake --build build-pi"

echo "Done! Pi program is at: build-pi/telemetry"