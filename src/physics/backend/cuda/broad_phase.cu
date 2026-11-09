/**
 * @file
 * CUDA backend implementation for broad-phase collision detection.
 */

#include "settings/compute/constants.h"
#include "settings/collisions/constants.h"
#include "physics/backend/cuda/step.h"

#include <cub/cub.cuh>


namespace sim {
namespace {

/**
 * Kernel to compute the spatial grid cell key for each active body.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void gridKeysKernel(
	StateView s, WorldConfig c,
    std::uint64_t* __restrict__ keys, int* __restrict__ ids
) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);

	if (i < s.count_i) {
		keys[i] = bodyCellKey(s, i, c);
		ids[i] = i;
	}
}


/**
 * Kernel to find the maximum bounding radius and speed across all active bodies.
 * These are used to determine the search span during candidate gathering.
 */
__global__ void gridLimitsKernel(StateView s, Real* limits) {
	Real radius = 0;
	Real speed = 0;

	for (int i = 0; i < s.count_i; ++i) {
		radius = maximum(radius, s.boundingRadius_m[i]);
		speed = maximum(speed, length(s.velocity_mps[i]));
	}

	limits[0] = radius;
	limits[1] = speed;
}


/**
 * Kernel to gather potential collision candidates for each active body using the sorted grid keys.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void broadPhaseKernel(
	StateView s, WorldConfig c,
    const Real* __restrict__ limits,
    const std::uint64_t* __restrict__ keys, const int* __restrict__ ids,
    int* __restrict__ candidates, int* __restrict__ counts,
	int* __restrict__ overflow
) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);

	if (i >= s.count_i) {
		return;
	}

	overflow[i] = 0;
	counts[i] = gatherCandidates(
		s, i, c, c.dt_s,
        limits[0], limits[1],
        keys, ids,
		candidates + std::size_t(i) * settings::Collisions::Storage::MAX_CANDIDATES, overflow[i]
	);
}

} // namespace (anonymous)


/**
 * Execute the broad-phase collision detection.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::broadPhase(StateView s, const WorldConfig& c) {
	int blocks = std::max(1, (s.count_i + settings::Compute::Cuda::BLOCK_SIZE - 1) / settings::Compute::Cuda::BLOCK_SIZE);
	gridKeysKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(s, c, keysIn.ptr, idsIn.ptr);
	CUDA_CHECK(cudaGetLastError());

	// Stable radix sorting preserves body-ID order for equal cell keys, matching the CPU traversal.
	if (count) {
		CUDA_CHECK(
			cub::DeviceRadixSort::SortPairs(
				scratch->ptr, scratchBytes,
                keysIn.ptr, keysOut.ptr,
                idsIn.ptr, idsOut.ptr,
                count, 0, 64,
				stream.value
			)
		);
	}

	// All kernels share one stream: the gather sees both sorted keys and completed motion bounds.
	gridLimitsKernel<<<1, 1, 0, stream.value>>>(s, limits.ptr);
	CUDA_CHECK(cudaGetLastError());

	broadPhaseKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(
		s, c, limits.ptr,
        keysOut.ptr, idsOut.ptr,
        candidates.ptr, candidateCounts.ptr,
        overflow.ptr
	);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim
