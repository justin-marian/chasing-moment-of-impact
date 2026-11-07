/**
 * @file
 * Explicit CUDA-unavailable behavior for CPU-only builds.
 *
 * GPU requests do not silently fall back to the CPU; they fail loudly.
 */

#include "shared/sim/backend.h"

#include <stdexcept>


namespace sim {

/**
 * Throw an error indicating the CUDA backend was disabled at build time.
 *
 * @param capacity_i Unused placeholder for the requested body capacity.
 * @return Never returns normally.
 * @throws std::runtime_error Always thrown to indicate the backend is missing.
 */
std::unique_ptr<Backend> createCudaBackend(int capacity_i) {
	// Suppress unused parameter warning since the backend cannot be created.
	(void)capacity_i; 
	throw std::runtime_error("CUDA backend was disabled at build time; configure ENABLE_CUDA=ON");
}


/**
 * Report that no CUDA devices are available.
 *
 * @return False, as CUDA support is compiled out.
 */
bool hasCudaDevice() {
	return false;
}

} // namespace sim
