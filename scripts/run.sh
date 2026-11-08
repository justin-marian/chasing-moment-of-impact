#!/bin/bash

# Ensure the script is running from the project root
cd "$(dirname "$0")" || exit

echo "==================================================="
echo " Chasing Moment of Impact - Viewer Launcher"
echo "==================================================="
echo ""

# Detect architecture (e.g., x86_64)
ARCH=$(uname -m)
BIN_DIR="./exports/builds/linux-${ARCH}"
VIEWER="${BIN_DIR}/physics_viewer"

if [ ! -f "$VIEWER" ]; then
    echo "Error: Viewer executable not found at $VIEWER"
    echo "Please build the project first: ./scripts/deployment/build-linux.sh --viewer --cuda"
    exit 1
fi

# ---------------------------------------------------------
# Choose your run mode by uncommenting ONE of the lines below
# ---------------------------------------------------------

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
