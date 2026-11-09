/**
 * @file
 * CUDA backend implementation for applying external forces.
 */

#include "settings/compute/constants.h"
#include "physics/backend/cuda/step.h"


namespace sim {
namespace {

/**
 * Kernel to apply gravity and reset transient per-step accumulators.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void applyForcesKernel(StateView s, WorldConfig c) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);

	if (i < s.count_i) {
		applyForces(s, i, c);
	}
}

} // namespace (anonymous)


/**
 * Launch the apply forces kernel.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::forces(StateView s, const WorldConfig& c) {
	int blocks = std::max(1, (s.count_i + settings::Compute::Cuda::BLOCK_SIZE - 1) / settings::Compute::Cuda::BLOCK_SIZE);
	applyForcesKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(s, c);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim
