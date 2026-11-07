/**
 * @file
 * CUDA backend implementation for generating contact constraints.
 */

#include "settings/compute/constants.h"
#include "settings/collisions/constants.h"
#include "physics/backend/cuda/step.h"


namespace sim {
namespace {

/**
 * Kernel to generate body/body contacts from the broad-phase candidate lists.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void bodyContactsKernel(
	StateView s, WorldConfig c, 
    const int* __restrict__ candidates, 
    const int* __restrict__ counts,
	Contact* __restrict__ contacts, 
    int* __restrict__ contactCounts
) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);
	if (i >= s.count_i) {
		return;
	}

	int count = 0;
	
	// Iterate through gathered candidates for this body.
	for (int k = 0; k < counts[i]; ++k) {
		int j = candidates[std::size_t(i) * settings::Collisions::Storage::MAX_CANDIDATES + k];
		count += pairContacts(
			s, i, j, c.dt_s, c,
			contacts + std::size_t(i) * settings::Collisions::Storage::MAX_CONTACTS + count
		);
	}

	contactCounts[i] = count;
}


/**
 * Kernel to generate contacts between bodies and the static world-box faces.
 */
__global__ __launch_bounds__(settings::Compute::Cuda::BLOCK_SIZE) void worldContactsKernel(
	StateView s, WorldConfig c, 
    Contact* __restrict__ contacts, 
    int* __restrict__ counts
) {
	int i = int(blockIdx.x * blockDim.x + threadIdx.x);
	if (i >= s.count_i) {
		return;
	}

	// Check all 6 faces and 8 corners of the bounding box.
	for (int face = 0; face < 6; ++face) {
		for (int corner = 0; corner < 8; ++corner) {
			Contact contact{};
			if (wallVertexContact(s, i, face, corner, c.dt_s, c, contact)) {
				contacts[std::size_t(i) * settings::Collisions::Storage::MAX_CONTACTS + counts[i]++] = contact;
			}
		}
	}
}

} // namespace (anonymous)


/**
 * Launch the body/body contact generation kernel.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::bodyContacts(StateView s, const WorldConfig& c) {
	int blocks = std::max(1, (s.count_i + settings::Compute::Cuda::BLOCK_SIZE - 1) / settings::Compute::Cuda::BLOCK_SIZE);
	bodyContactsKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(s, c, candidates.ptr, candidateCounts.ptr, contacts.ptr, contactCounts.ptr);
	CUDA_CHECK(cudaGetLastError());
}


/**
 * Launch the world/body contact generation kernel.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::worldContacts(StateView s, const WorldConfig& c) {
	int blocks = std::max(1, (s.count_i + settings::Compute::Cuda::BLOCK_SIZE - 1) / settings::Compute::Cuda::BLOCK_SIZE);
	worldContactsKernel<<<blocks, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(s, c, contacts.ptr, contactCounts.ptr);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim
