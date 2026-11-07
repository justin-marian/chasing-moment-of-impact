/**
 * @file
 * CUDA backend implementation for updating sleeping bodies.
 */

#include "settings/compute/constants.h"
#include "physics/backend/cuda/step.h"


namespace sim {
namespace {

/**
 * Kernel to track quiet ticks and mark eligible bodies as sleeping.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void updateSleepKernel(StateView s, WorldConfig c) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);
	
	if (i < s.count_i) {
		updateSleep(s, i, c);
	}
}

} // namespace (anonymous)


/**
 * Launch the sleep update kernel.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::sleep(StateView s, const WorldConfig& c) {
	int blocks = std::max(1, (s.count_i + settings::Compute::Cuda::BLOCK_SIZE - 1) / settings::Compute::Cuda::BLOCK_SIZE);
	updateSleepKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(s, c);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim
