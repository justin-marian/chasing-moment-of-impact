#!/bin/bash

# Go up one level from scripts/ to the project root
cd "$(dirname "$0")/.." || exit

echo "==================================================="
echo " Chasing the Moment of Impact - Build and Launch"
echo "==================================================="
echo ""

# ---------------------------------------------------------
# BUILD THE PROJECT
# ---------------------------------------------------------
echo "[1/2] Building the simulator..."

# NOTE: If you do not have an NVIDIA GPU, remove the --cuda flag below!
chmod +x ./scripts/deployment/build-linux.sh
bash ./scripts/deployment/build-linux.sh --cuda --viewer --arch 89

# Check if the build succeeded
if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Build failed. Aborting launch."
    exit 1
fi

echo ""
echo "[2/2] Build successful. Launching viewer..."
echo ""

# ---------------------------------------------------------
# CHOOSE YOUR RUN MODE
# ---------------------------------------------------------
ARCH=$(uname -m)
BIN_DIR="./exports/builds/linux-${ARCH}"
VIEWER="${BIN_DIR}/physics_viewer"

# Uncomment ONE of the lines below to choose your launch mode.

# Default Launch (CPU, Demo Scene)
# "$VIEWER"

# Heavy Load (CUDA, Matrix A - 850 bodies) - RECOMMENDED
"$VIEWER" --backend cuda --scenario A

# Impact Strength Test (No gravity, focused on collisions)
# "$VIEWER" --scenario impact

# Launch Paused (To inspect the initial spawn state)
# "$VIEWER" --paused

# Headless Capture (Saves a PNG without opening a window)
# "$VIEWER" --hidden --frames 60 --capture media/capture_A.png --scenario A

echo ""
echo "Viewer closed."
