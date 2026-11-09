/**
 * @file
 * CUDA backend lifecycle, state transfer, and execution loop.
 */

#include "settings/compute/constants.h"
#include "settings/collisions/constants.h"
#include "physics/backend/cuda/step.h"

#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <cub/cub.cuh>


namespace sim {

/**
 * Initialize the CUDA backend by allocating all GPU/host memory and querying device hardware.
 *
 * This constructor pre-allocates every buffer required for the simulation lifecycle.
 * By satisfying all memory requirements upfront, the fixed-step execution pipeline
 * guarantees zero allocations on the hot path.
 *
 * Key architectural allocations:
 * - **SoA Storage:** Allocates the primary structure-of-arrays for body kinematics and dynamics.
 * - **Contiguous Flattening:** Candidate and contact buffers are allocated as single, massive
 *   1D arrays to ensure coalesced memory access on the GPU, avoiding pointer-chasing.
 * - **Pinned Host Memory:** Allocates page-locked (pinned) host buffers to allow
 *   asynchronous DMA transfers over PCIe at maximum bandwidth.
 * - **Dynamic CUB Scratch:** Queries the exact temporary-storage requirement for the
 *   broad-phase radix sort, then allocates the precise byte count needed.
 *
 * @param capacity Maximum number of bodies allocated by this owner.
 */
CudaBackend::CudaBackend(int capacity) :
	// Core execution state.
	capacity(capacity),
	storage(capacity), // Primary SoA arrays (pos, vel, mass).
	stream(),          // Non-blocking stream to serialize kernels.

	// Broad-phase sorting.
	keysIn(capacity),
	keysOut(capacity),
	idsIn(capacity),
	idsOut(capacity),

	// Candidates and contacts (flattened 2D arrays).
	candidates(static_cast<std::size_t>(capacity) * settings::Collisions::Storage::MAX_CANDIDATES),
	candidateCounts(capacity), // Per-body counts.
	contactCounts(capacity),
	overflow(capacity),
	contacts(static_cast<std::size_t>(capacity) * settings::Collisions::Storage::MAX_CONTACTS),

	// Solver graph coloring.
	colorNumber(1),            // GPU-side color count.
	colorMasks(static_cast<std::size_t>(capacity) * settings::Collisions::Solver::COLOR_MASK_WORDS), // Bitmasks.
	colors(static_cast<std::size_t>(capacity) * settings::Collisions::Storage::MAX_CONTACTS), // Assigned colors.
	coloredIndices(static_cast<std::size_t>(capacity) * settings::Collisions::Storage::MAX_CONTACTS), // Grouped indices.
	colorCounts(settings::Collisions::Solver::MAX_SOLVER_COLORS), // Prefix sums.
	colorOffsets(settings::Collisions::Solver::MAX_SOLVER_COLORS + 1),

	// Diagnostics and transfers.
	diagnosticsBuffer(1), // Device aggregated results.
	limits(2),            // Max radius/speed limits.
	hostDiagnostics(1),   // Pinned host memory for async DMA.
	staging(static_cast<std::size_t>(capacity) * settings::Compute::Cuda::BYTES_PER_BODY) // Pinned staging for PCIe.
{
	// Critical: raw byte-copying requires exact, padding-free layouts.
	static_assert(sizeof(Real) == 4, "CUDA physics is float32");
	static_assert(sizeof(Contact) == 56);
	static_assert(offsetof(StateView, position_m) == 8);

	// Query device properties for diagnostics.
	cudaDeviceProp properties{};
	CUDA_CHECK(cudaGetDeviceProperties(&properties, 0));
	device = properties.name;

	// Query required scratch bytes for CUB Radix Sort.
	CUDA_CHECK(cub::DeviceRadixSort::SortPairs(
		nullptr, scratchBytes,
		keysIn.ptr, keysOut.ptr,
		idsIn.ptr, idsOut.ptr,
		capacity, 0, 64,
		stream.value
	));

	// Allocate scratch.
	scratch = std::make_unique<DeviceBuffer<unsigned char>>(scratchBytes);
}


/**
 * Copy the active structure-of-arrays (SOA) state across the explicit host/device ownership boundary.
 *
 * @param state  Host simulation arrays, active body count and authoritative tick in SI units.
 * @param upload True for host-to-device transfer; false for copying device state back to the host.
 */
void CudaBackend::transfer(State& state, bool upload) {
	auto host = state.view();
	auto deviceState = storage.view(state.count_i);
	std::size_t offset = 0;

	auto copy = [&](auto* hostPointer, auto* devicePointer) {
		using T = std::remove_pointer_t<decltype(hostPointer)>;
		std::size_t bytes = std::size_t(state.count_i) * sizeof(T);
		auto* pinned = staging.ptr + offset;

		if (upload) {
			std::memcpy(pinned, hostPointer, bytes);
			CUDA_CHECK(cudaMemcpyAsync(devicePointer, pinned, bytes, cudaMemcpyHostToDevice, stream.value));
		} else {
			CUDA_CHECK(cudaMemcpyAsync(pinned, devicePointer, bytes, cudaMemcpyDeviceToHost, stream.value));
		}

		offset += bytes;
	};

#define COPY_FIELD(field) copy(host.field, deviceState.field)
	COPY_FIELD(position_m);
	COPY_FIELD(velocity_mps);
	COPY_FIELD(angularVelocity_rad_s);
	COPY_FIELD(halfExtents_m);
	COPY_FIELD(inverseInertia_kg_m2);
	COPY_FIELD(force_N);
	COPY_FIELD(aabbExtent_m);
	COPY_FIELD(orientation);
	COPY_FIELD(inverseMass_per_kg);
	COPY_FIELD(gravityScale_unitless);
	COPY_FIELD(boundingRadius_m);
	COPY_FIELD(shape_i);
	COPY_FIELD(sleepCounter_i);
	COPY_FIELD(isSleeping);
	COPY_FIELD(hasContact);
	COPY_FIELD(wallProjection_m);
#undef COPY_FIELD

	// Synchronize only at the explicit host/device ownership boundary.
	CUDA_CHECK(cudaStreamSynchronize(stream.value));

	if (upload) {
		return;
	}

	offset = 0;
	auto read = [&](auto* pointer) {
		using T = std::remove_pointer_t<decltype(pointer)>;
		std::size_t bytes = std::size_t(state.count_i) * sizeof(T);
		std::memcpy(pointer, staging.ptr + offset, bytes);
		offset += bytes;
	};

#define READ_FIELD(field) read(host.field)
	READ_FIELD(position_m);
	READ_FIELD(velocity_mps);
	READ_FIELD(angularVelocity_rad_s);
	READ_FIELD(halfExtents_m);
	READ_FIELD(inverseInertia_kg_m2);
	READ_FIELD(force_N);
	READ_FIELD(aabbExtent_m);
	READ_FIELD(orientation);
	READ_FIELD(inverseMass_per_kg);
	READ_FIELD(gravityScale_unitless);
	READ_FIELD(boundingRadius_m);
	READ_FIELD(shape_i);
	READ_FIELD(sleepCounter_i);
	READ_FIELD(isSleeping);
	READ_FIELD(hasContact);
	READ_FIELD(wallProjection_m);
#undef READ_FIELD
}


/**
 * Enable or disable collection of confirmed collision-impact records.
 *
 * @param enabled Whether confirmed impact records should be retained by the backend.
 */
void CudaBackend::setImpactTracking(bool enabled) {
	trackImpacts = enabled;

	if (enabled && !impactEvents) {
		impactMeasures = std::make_unique<DeviceBuffer<ImpactMeasure>>(std::size_t(capacity) * settings::Collisions::Storage::MAX_CONTACTS);
		impactEvents = std::make_unique<DeviceBuffer<ImpactEvent>>(std::size_t(capacity) * settings::Collisions::Impacts::EVENTS_PER_BODY);
		hostImpacts = std::make_unique<PinnedBuffer<ImpactEvent>>(std::size_t(capacity) * settings::Collisions::Impacts::EVENTS_PER_BODY);
	}
}


/**
 * Return a borrowed view of the most recently retained visual impact data.
 *
 * @return Borrowed retained impact data; its lifetime follows the owning backend/renderer.
 */
std::span<const ImpactEvent> CudaBackend::impacts() const {
	return trackImpacts ?
    std::span<const ImpactEvent>(hostImpacts->ptr, std::size_t(count) * settings::Collisions::Impacts::EVENTS_PER_BODY) :
    std::span<const ImpactEvent>{};
}


/**
 * Transfer the active host state into backend execution storage.
 *
 * @param state Host simulation arrays, active body count and authoritative tick in SI units.
 */
void CudaBackend::upload(const State& state) {
	if (state.count_i > capacity) {
		throw std::runtime_error("CUDA body capacity exceeded");
	}

	count = state.count_i;
	transfer(const_cast<State&>(state), true);
}


/**
 * Copy the current backend body arrays into the caller's host state.
 *
 * @param state Host simulation arrays, active body count and authoritative tick in SI units.
 */
void CudaBackend::download(State& state) {
	state.count_i = count;

	if (count) {
		transfer(state, false);
	}
}


/**
 * Execute one configured fixed simulation tick and return its measured stage diagnostics.
 *
 * @param state  Host simulation arrays, active body count and authoritative tick in SI units.
 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
 * @return Stage timings and diagnostics for the completed fixed simulation tick.
 */
StepResult CudaBackend::step(State& state, const WorldConfig& config) {
	if (state.count_i != count) {
		upload(state);
	}

	auto view = storage.view(count);
	StepResult result{};

	using Clock = std::chrono::steady_clock;
	auto start = Clock::now();

	auto mark = [&](int stage) {  CUDA_CHECK(cudaEventRecord(events[stage].value, stream.value)); };

	// Event boundaries mirror configured stage names and the CPU timing sequence exactly.
	mark(0);
	forces(view, config); mark(1);
	velocity(view, config); mark(2);
	bounds(view); mark(3);
	broadPhase(view, config); mark(4);
	bodyContacts(view, config); mark(5);
	worldContacts(view, config); mark(6);
	solver(view, config); mark(7);
	position(view, config); mark(8);
	sleep(view, config); mark(9);
	diagnostics(view, config); mark(10);

	CUDA_CHECK(cudaMemcpyAsync(
        hostDiagnostics.ptr, diagnosticsBuffer.ptr,
        sizeof(Diagnostics),
        cudaMemcpyDeviceToHost, stream.value
    ));

	if (trackImpacts && count) {
		CUDA_CHECK(cudaMemcpyAsync(
			hostImpacts->ptr, impactEvents->ptr,
			std::size_t(count) * settings::Collisions::Impacts::EVENTS_PER_BODY * sizeof(ImpactEvent),
			cudaMemcpyDeviceToHost, stream.value
		));
	}

	CUDA_CHECK(cudaStreamSynchronize(stream.value));

	for (int stage = 0; stage < settings::Compute::Stages::COUNT; ++stage) {
		float milliseconds = 0;
		CUDA_CHECK(cudaEventElapsedTime(&milliseconds, events[stage].value, events[stage + 1].value));
		result.stage_wall_ms[stage] = milliseconds;
	}

	result.diagnostics = *hostDiagnostics.ptr;
	result.total_wall_ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
	++state.tick_i;

	if (result.diagnostics.nanCount_i || result.diagnostics.overflowCount_i) {
		throw std::runtime_error("CUDA frame failed: nonfinite state or candidate capacity exceeded");
	}

	return result;
}


/**
 * Factory function to create the CUDA backend.
 *
 * @param capacity_i Maximum number of bodies.
 * @return Unique pointer to the backend.
 * @throws std::runtime_error If no CUDA device is available.
 */
std::unique_ptr<Backend> createCudaBackend(int capacity_i) {
	if (!hasCudaDevice()) {
		throw std::runtime_error("No NVIDIA CUDA device available");
	}

	return std::make_unique<CudaBackend>(capacity_i);
}


/**
 * Query if a CUDA device is available on this system.
 *
 * @return True if at least one device is present.
 */
bool hasCudaDevice() {
	int count = 0;
	auto status = cudaGetDeviceCount(&count);

	if (status == cudaErrorNoDevice || status == cudaErrorInsufficientDriver) {
		return false;
	}

	CUDA_CHECK(status);
	return count > 0;
}

} // namespace sim
