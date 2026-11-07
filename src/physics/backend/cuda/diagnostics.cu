/**
 * @file
 * CUDA backend implementation for aggregating step diagnostics.
 */

#include "physics/backend/cuda/step.h"


namespace sim {
namespace {

/**
 * Kernel to aggregate diagnostics, including contact and candidate counts.
 */
__global__ void emitDiagnosticsKernel(
	StateView s, WorldConfig c, 
    const int* counts, const int* candidateCounts,
    const int* overflow,
	Diagnostics* out
) {
	Diagnostics d = emitDiagnostics(s, c);

	// Sum up the valid contacts, candidate pairs, and overflow flags across all bodies.
	for (int i = 0; i < s.count_i; ++i) {
		d.contacts_i += counts[i];
		d.candidatePairs_i += candidateCounts[i];
		d.overflowCount_i += overflow[i];
	}

	*out = d;
}

} // namespace (anonymous)


/**
 * Launch the diagnostics aggregation kernel.
 *
 * @param s Shared body-state view.
 * @param c World configuration.
 */
void CudaBackend::diagnostics(StateView s, const WorldConfig& c) {
	emitDiagnosticsKernel<<<1, 1, 0, stream.value>>>(s, c, contactCounts.ptr, candidateCounts.ptr, overflow.ptr, diagnosticsBuffer.ptr);
	CUDA_CHECK(cudaGetLastError());
}

} // namespace sim