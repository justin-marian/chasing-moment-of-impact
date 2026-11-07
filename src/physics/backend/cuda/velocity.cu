/**
 * @file
 * CUDA backend implementation for integrating velocities.
 */

#include "settings/compute/constants.h"
#include "physics/backend/cuda/step.h"


namespace sim {
namespace {

/**
 * Kernel to integrate forces and apply damping to body velocities.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void integrateVelocityKernel(StateView s, WorldConfig c) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);
	if (i < s.count_i) {
		integrateVelocity(s, i, c.dt_s, c);
	}
}

} // namespace (anonymous)


/**
 * Launch the velocity integration kernel.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::velocity(StateView s, const WorldConfig& c) {
	int blocks = std::max(1, (s.count_i + settings::Compute::Cuda::BLOCK_SIZE - 1) / settings::Compute::Cuda::BLOCK_SIZE);
	integrateVelocityKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(s, c);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim
