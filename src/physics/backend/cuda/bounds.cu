/**
 * @file
 * CUDA backend implementation for computing bounding volumes.
 */

#include "settings/compute/constants.h"
#include "physics/backend/cuda/step.h"


namespace sim {
namespace {

/**
 * Kernel to compute the axis-aligned bounding box (AABB) for each active body.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void computeAabbKernel(StateView s) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);
	// Ensure thread index is within active body count.
	if (i < s.count_i) {
		computeAabb(s, i);
	}
}

} // namespace (anonymous)


/**
 * Launch the bounding volume computation kernel.
 *
 * @param s Shared body-state view.
 */
void CudaBackend::bounds(StateView s) {
	int blocks = std::max(1, (s.count_i + settings::Compute::Cuda::BLOCK_SIZE - 1) / settings::Compute::Cuda::BLOCK_SIZE);
	computeAabbKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(s);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim
