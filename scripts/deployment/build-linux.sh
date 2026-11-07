#!/usr/bin/env bash
#
# Build the Linux CPU/CUDA executables and OpenGL viewer.
#
# Sets up the build environment, configures CMake with Ninja, and builds 
# the project. For CUDA, it verifies that NVCC is available in the PATH 
# and passes the appropriate architecture flags.

set -euo pipefail

# Default values
CUDA="OFF"
VIEWER="OFF"
MMO="OFF"
REF_DOUBLE="OFF"
ARCH="89"
JOBS=6
BUILD_DIR=""

# Resolve project root (two levels up from this script's directory)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

# Default the build directory to a project-local tmp/ so multiple build
# flavors can coexist without clobbering each other.
DEFAULT_BUILD_DIR="${PROJECT_ROOT}/tmp/build/linux"


usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --cuda               Enable the CUDA backend"
    echo "  --viewer             Build the OpenGL viewer"
    echo "  --mmo                Enable the optional 3D MMO targets"
    echo "  --reference-double   Use the CPU-only float64 reference implementation"
    echo "  --arch <number>      CUDA architecture number (default: 89)"
    echo "  --build-dir <path>   Override the default build directory"
    echo "  --jobs <number>      Number of parallel build jobs (default: 6)"
    echo "  -h, --help           Show this help message"
    exit 0
}


# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --cuda)
            CUDA="ON"
            shift
            ;;
        --viewer)
            VIEWER="ON"
            shift
            ;;
        --mmo)
            MMO="ON"
            shift
            ;;
        --reference-double)
            REF_DOUBLE="ON"
            shift
            ;;
        --arch)
            ARCH="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Unknown parameter: $1"
            usage
            ;;
    esac
done


if [[ -z "${BUILD_DIR}" ]]; then
    BUILD_DIR="${DEFAULT_BUILD_DIR}"
fi


# The float64 reference build has no CUDA path; rejecting the combination
# here avoids a long build followed by a late linker error.
if [[ "${CUDA}" == "ON" && "${REF_DOUBLE}" == "ON" ]]; then
    echo "Error: Float64 reference is CPU-only; omit --cuda."
    exit 1
fi


# Verify required build tools are available
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is not installed or not in PATH."
    exit 1
fi

if ! command -v ninja &> /dev/null; then
    echo "Error: ninja is not installed or not in PATH."
    exit 1
fi


# Verify NVCC is available if CUDA is requested
CUDA_COMPILER_OPTIONS=()
if [[ "${CUDA}" == "ON" ]]; then
    if ! command -v nvcc &> /dev/null; then
        echo "Error: nvcc is not installed or not in PATH, but --cuda was requested."
        exit 1
    fi
    NVCC_PATH="$(command -v nvcc)"
    echo "Found NVCC: ${NVCC_PATH}"
    
    # Pass the explicit compiler path to avoid CMake picking up system wrappers
    CUDA_COMPILER_OPTIONS=(
        "-DCMAKE_CUDA_COMPILER=${NVCC_PATH}"
    )
fi


# Helper to copy necessary runtime libraries for app-local deployment
copy_runtime_libraries() {
    local output_dir="$1"
    mkdir -p "${output_dir}"
    
    if [[ "${CUDA}" == "ON" ]]; then
        echo "Copying CUDA runtime libraries to ${output_dir}..."
        
        # Find the CUDA toolkit directory by tracing the nvcc executable
        local nvcc_real
        nvcc_real="$(readlink -f "$(command -v nvcc)")"
        local cuda_root
        cuda_root="$(dirname "$(dirname "${nvcc_real}")")"
        
        # Copy libcudart.so and libcuda.so if they exist
        find "${cuda_root}" -name "libcudart.so*" -exec cp -P {} "${output_dir}/" \; 2>/dev/null || true
        
        # Best-effort copy of the driver API library
        find /usr/lib /usr/lib64 /usr/local/cuda -name "libcuda.so*" -exec cp -P {} "${output_dir}/" \; 2>/dev/null || true
    fi
}


# 1. Print configuration summary
echo "Configuring in ${BUILD_DIR}"
echo "  CMake Options:"
echo "    CUDA=${CUDA}"
echo "    Viewer=${VIEWER}"
echo "    MMO=${MMO}"
echo "    ReferenceDouble=${REF_DOUBLE}"
if [[ "${CUDA}" == "ON" ]]; then
    echo "    Architecture=${ARCH}"
fi
echo ""


# 2. Configure all options explicitly, so old CMake cache choices cannot override this run.
cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TRY_COMPILE_CONFIGURATION=Release \
    -DENABLE_CUDA="${CUDA}" \
    -DBUILD_VIEWER="${VIEWER}" \
    -DBUILD_MMO="${MMO}" \
    -DREFERENCE_DOUBLE="${REF_DOUBLE}" \
    -DCMAKE_CUDA_ARCHITECTURES="${ARCH}" \
    "${CUDA_COMPILER_OPTIONS[@]}"

    
# 3. Build, then copy redistributable libraries only after a successful build.
echo ""
echo "Building with ${JOBS} parallel jobs"
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

OUTPUT_DIR="${PROJECT_ROOT}/exports/builds/linux-x64"
copy_runtime_libraries "${OUTPUT_DIR}"

echo ""
echo "Build successful. Artifacts deployed to ${OUTPUT_DIR}"
