/**
 * @file
 * CUDA backend implementation for the sequential impulse solver.
 */

#include "settings/compute/constants.h"
#include "settings/collisions/constants.h"
#include "physics/backend/cuda/step.h"


namespace sim {
namespace {

/**
 * Kernel to assign colors to contacts.
 *
 * Ensures contacts of the same color do not share dynamic bodies, allowing
 * for parallel resolution without data races.
 */
__global__ void colorContactsKernel(
	StateView s, Contact* contacts,
    const int* counts, std::uint32_t* masks,
    int* colors, int* indices,
	int* colorCounts, int* offsets, int* number,
    int* overflow
) {
	*number = colorContacts(s, contacts, counts, masks, colors, indices, colorCounts, offsets);

	// Handle graph coloring overflow (exceeds maximum allowed colors).
	if (*number < 0) {
		overflow[0] = 1;
		*number = 0;
	}
}


/**
 * Kernel to execute the iterative constraint solver over the colored contacts.
 */
__global__ void solveContactsKernel(
	StateView s, WorldConfig c, Contact* contacts,
    const int* indices, const int* offsets, const int* number,
	ImpactMeasure* measures, ImpactEvent* impacts
) {
	// Clear previous impact records if tracking is enabled.
	if (impacts) {
		for (int i = int(threadIdx.x); i < s.count_i * settings::Collisions::Impacts::EVENTS_PER_BODY; i += int(blockDim.x)) {
			impacts[i] = {};
		}
		__syncthreads();
	}

	// Initialize contacts and measure pre-solve impact severity.
	for (int color = 0; color < *number; ++color) {
		for (int k = offsets[color] + int(threadIdx.x); k < offsets[color + 1]; k += int(blockDim.x)) {
			initializeContact(s, contacts[indices[k]], c.dt_s, c);
			if (impacts) {
				measures[indices[k]] = measureImpact(s, contacts[indices[k]]);
			}
		}
		__syncthreads();
	}

	// Iterate constraint solver over the colored contacts.
	// One block owns the solve. Each color has disjoint body writes; a block barrier publishes
	// its velocity changes before the next color reads them. A multi-block launch would need
	// a different global synchronization scheme, so do not increase the grid size here.
	for (int iteration = 0; iteration < c.solverIterations_i; ++iteration) {
		for (int color = 0; color < *number; ++color) {
			for (int k = offsets[color] + int(threadIdx.x); k < offsets[color + 1]; k += int(blockDim.x)) {
				solveContactConstraint(s, contacts[indices[k]], c);
			}
			__syncthreads();
		}
	}

	// Record confirmed impacts after the solve is complete.
	if (impacts) {
		for (int color = 0; color < *number; ++color) {
			for (int k = offsets[color] + int(threadIdx.x); k < offsets[color + 1]; k += int(blockDim.x)) {
				recordImpact(s, contacts[indices[k]], measures[indices[k]], c, impacts);
			}
			__syncthreads();
		}
	}

	// Apply split positional correction to resolve remaining penetrations.
	for (int color = 0; color < *number; ++color) {
		for (int k = offsets[color] + int(threadIdx.x); k < offsets[color + 1]; k += int(blockDim.x)) {
			correctContactPosition(s, contacts[indices[k]], c);
		}
		__syncthreads();
	}

	// Project OBBs back inside world bounds.
	for (int i = int(threadIdx.x); i < s.count_i; i += int(blockDim.x)) {
		correctWallPosition(s, i, c);
	}
}

} // namespace (anonymous)


/**
 * Launch the contact coloring and solving kernels.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::solver(StateView s, const WorldConfig& c) {
	colorContactsKernel<<<1, 1, 0, stream.value>>>(
		s, contacts.ptr, contactCounts.ptr,
        colorMasks.ptr, colors.ptr, coloredIndices.ptr, colorCounts.ptr,
		colorOffsets.ptr, colorNumber.ptr, overflow.ptr
	);
	CUDA_CHECK(cudaGetLastError());

	solveContactsKernel<<<1, settings::Compute::Cuda::BLOCK_SIZE, 0, stream.value>>>(
		s, c, contacts.ptr, coloredIndices.ptr, colorOffsets.ptr, colorNumber.ptr,
		trackImpacts ? impactMeasures->ptr : nullptr,
        trackImpacts ? impactEvents->ptr : nullptr
	);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim
