/**
 * @file
 * CUDA backend implementation for integrating positions.
 */

#include "settings/compute/constants.h"
#include "physics/backend/cuda/step.h"


namespace sim {
namespace {

/**
 * Kernel to integrate body positions and enforce world boundaries.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void integratePositionKernel(StateView s, WorldConfig c) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);

	if (i < s.count_i) {
		// Advance position and orientation based on current velocities.
		integratePosition(s, i, c.dt_s);
		// Ensure the body remains inside the world boundaries.
		projectWorldConstraints(s, i, c);
	}
}

} // namespace (anonymous)


/**
 * Launch the position integration kernel.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::position(StateView s, const WorldConfig& c) {
	int blocks = std::max(1, (s.count_i + settings::Compute::Cuda::BLOCK_SIZE - 1) / settings::Compute::Cuda::BLOCK_SIZE);
	integratePositionKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(s, c);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim
