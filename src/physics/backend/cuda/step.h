/**
 * @file
 * Share CUDA storage ownership, execution stages and host-side upload/download boundaries.
 */

#pragma once

#include "settings/compute/constants.h"

#include "shared/sim/backend.h"
#include "shared/sim/physics.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <span>
#include <string>

#include <cuda_runtime.h>


namespace sim {

/**
 * Report the failing CUDA call location before terminating an unusable backend.
 *
 * @param error CUDA result code; non-success reports the call site and terminates the unusable backend.
 * @param file  Source filename recorded at the call site.
 * @param line  One-based source line recorded at the call site.
 */
inline void checkCuda(cudaError_t error, const char* file, int line) {
	if (error != cudaSuccess) {
		std::fprintf(stderr, "CUDA error %s:%d: %s\n", file, line, cudaGetErrorString(error));
		std::abort();
	}
}

#define CUDA_CHECK(call) checkCuda((call), __FILE__, __LINE__)


/**
 * Own a fixed device allocation; copying would duplicate ownership.
 */
template <class T>
struct DeviceBuffer {
	T* ptr{};

	/**
	 * Allocate a fixed number of elements in exclusively owned CUDA device memory.
	 *
	 * @param count Number of elements allocated.
	 */
	explicit DeviceBuffer(std::size_t count) { CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&ptr), count * sizeof(T))); }

	/**
	 * Release this owner's resources once; borrowed state is not destroyed.
     * Disallow copying because duplicating this object would duplicate resource ownership.
     * Disallow copy assignment because this type has exclusive resource ownership.
	 */
	~DeviceBuffer() { if (ptr) { CUDA_CHECK(cudaFree(ptr)); } }
	DeviceBuffer(const DeviceBuffer&) = delete;
	DeviceBuffer& operator=(const DeviceBuffer&) = delete;
};


/**
 * Own page-locked host memory used by asynchronous transfers.
 */
template <class T>
struct PinnedBuffer {
	T* ptr{};

	/**
	 * Allocate page-locked host elements for explicit asynchronous transfer boundaries.
	 *
	 * @param count Number of elements allocated.
	 */
	explicit PinnedBuffer(std::size_t count) { CUDA_CHECK(cudaHostAlloc(reinterpret_cast<void**>(&ptr), count * sizeof(T), cudaHostAllocDefault)); }

	/**
	 * Release this owner's resources once; borrowed state is not destroyed.
	 * Disallow copying because duplicating this object would duplicate resource ownership.
	 * Disallow copy assignment because this type has exclusive resource ownership.
	 */
	~PinnedBuffer() { if (ptr) { CUDA_CHECK(cudaFreeHost(ptr)); } }
	PinnedBuffer(const PinnedBuffer&) = delete;
	PinnedBuffer& operator=(const PinnedBuffer&) = delete;
};


/**
 * Order all simulation stages on one nonblocking stream.
 */
struct Stream {
	cudaStream_t value{};

	/**
	 * Create one nonblocking CUDA stream to preserve deterministic simulation-stage ordering.
	 */
	Stream() { CUDA_CHECK(cudaStreamCreateWithFlags(&value, cudaStreamNonBlocking)); }

	/**
	 * Release this owner's resources once; borrowed state is not destroyed.
	 * Disallow copy assignment because this type has exclusive resource ownership.
	 * Disallow copy assignment because this type has exclusive resource ownership.
	 */
	~Stream() { CUDA_CHECK(cudaStreamDestroy(value)); }
	Stream(const Stream&) = delete;
	Stream& operator=(const Stream&) = delete;
};


/**
 * Mark simulation stage boundaries for device elapsed-time measurements.
 */
struct Event {
	cudaEvent_t value{};

	/**
	 * Create a CUDA timing event used to measure solver-stage boundaries.
	 */
	Event() { CUDA_CHECK(cudaEventCreate(&value)); }

	/**
	 * Release this owner's resources once; borrowed state is not destroyed.
	 * Disallow copying because duplicating this object would duplicate resource ownership.
	 * Disallow copy assignment because this type has exclusive resource ownership.
	 */
	~Event() { CUDA_CHECK(cudaEventDestroy(value)); }
	Event(const Event&) = delete;
	Event& operator=(const Event&) = delete;
};


/**
 * Device structure-of-arrays (SOA) storage in the same schema as the host view.
 */
struct Storage {
	/* Kinematics. */
	DeviceBuffer<Vec3> position, velocity, angular;
	DeviceBuffer<Quat> orientation;

	/* Geometry. */
	DeviceBuffer<Vec3> extent, aabb;
	DeviceBuffer<Real> radius;

	/* Dynamics. */
	DeviceBuffer<Vec3> inertia, force;
	DeviceBuffer<Real> invMass, gravity;

	/* Solver state. */
	DeviceBuffer<int> shape, sleepCounter, sleeping, hasContact;
	DeviceBuffer<Real> projection;

	/**
	 * Allocate each typed array in the shared device state schema for the same body capacity.
	 *
	 * @param count Number of elements allocated.
	 */
    explicit Storage(int count) :
        // 1. Kinematics
        position(count), velocity(count), angular(count),
        // 2. Mass, Geometry & Orientation
        extent(count), inertia(count), force(count), aabb(count), orientation(count),
        // 3. Scalar Properties
        invMass(count), gravity(count), radius(count),
        // 4. Solver State
        projection(count), shape(count), sleepCounter(count), sleeping(count), hasContact(count) {}

	/**
	 * Build the non-owning state view over the active device-buffer range.
	 *
	 * @param count Number of elements exposed through this non-owning view.
	 * @return Borrowed typed pointers and active count in the shared StateView layout.
	 */
	StateView view(int count) {
		return {
			count,
			// Linear and angular motion.
			position.ptr, velocity.ptr, angular.ptr,
			// Mass, geometry, and orientation.
			extent.ptr, inertia.ptr, force.ptr, aabb.ptr, orientation.ptr,
			// Scalar body properties.
			invMass.ptr, gravity.ptr, radius.ptr,
			// Solver and collision state.
			projection.ptr, shape.ptr, sleepCounter.ptr, sleeping.ptr, hasContact.ptr
		};
	}
};



/**
 * GPU-resident implementation of the fixed-step physics pipeline.
 *
 * This class fulfills the Backend interface by allocating dedicated
 * structure-of-arrays (SOA) device memory and executing the simulation entirely
 * on the GPU. It maintains strict determinism by mirroring the exact same ten-stage
 * execution order as the CPU backend, ensuring reliable cross-backend parity testing.
 *
 * Design constraints:
 * - **Explicit Boundaries:** State lives on the device during execution. Host/device
 *   synchronization occurs only at the explicit upload() and download() boundaries,
 *   minimizing PCIe bus traffic.
 * - **Serialized Stages:** All kernels run on a single non-blocking stream. This
 *   preserves the strict inter-stage dependencies (e.g., broad-phase must finish before narrow-phase)
 *   without requiring expensive global syncs.
 * - **Scratch Ownership:** Pre-allocates all temporary buffers, CUB radix-sort
 *   scratch space, and pinned host staging buffers on construction to guarantee
 *   zero-allocation hot paths.
 */
class CudaBackend final : public Backend {
	/* Core state. */
	int capacity;
	int count{};
	Stream stream;
	Storage storage;

	/* Broad-phase grids. */
	DeviceBuffer<std::uint64_t> keysIn, keysOut;
	DeviceBuffer<int> idsIn, idsOut;
	DeviceBuffer<int> candidates, candidateCounts, overflow;

	/* Contacts and solver. */
	DeviceBuffer<int> contactCounts;
	DeviceBuffer<Contact> contacts;
	DeviceBuffer<int> colorNumber;
	DeviceBuffer<std::uint32_t> colorMasks;
	DeviceBuffer<int> colors, coloredIndices, colorCounts, colorOffsets;

	/* Diagnostics and staging. */
	DeviceBuffer<Diagnostics> diagnosticsBuffer;
	DeviceBuffer<Real> limits;
	PinnedBuffer<Diagnostics> hostDiagnostics;
	PinnedBuffer<unsigned char> staging;

	/* Optional impact tracking. */
	std::unique_ptr<DeviceBuffer<unsigned char>> scratch;
	std::unique_ptr<DeviceBuffer<ImpactMeasure>> impactMeasures;
	std::unique_ptr<DeviceBuffer<ImpactEvent>> impactEvents;
	std::unique_ptr<PinnedBuffer<ImpactEvent>> hostImpacts;
	bool trackImpacts{};

	/* Timing and metadata. */
	std::size_t scratchBytes{};
	Event events[settings::Compute::Stages::COUNT + 1];
	std::string device;

	// --- Host/Device Boundary ---
	/**
	 * Copy the active structure-of-arrays (SOA) state across the explicit host/device ownership boundary.
	 *
	 * @param state  Host simulation arrays, active body count and authoritative tick in SI units.
	 * @param upload True for host-to-device transfer; false for copying device state back to the host.
	 */
	void transfer(State& state, bool upload);

	// --- Simulation Pipeline Stages ---
	/**
	 * Apply configured external forces and reset transient contact flags for the active bodies.
	 */
	void forces(StateView state, const WorldConfig& config);
	/**
	 * Integrate forces and configured damping into awake body velocities.
	 */
	void velocity(StateView state, const WorldConfig& config);
	/**
	 * Refresh cached world-axis bounds before the broad-phase contact search.
	 */
	void bounds(StateView state);
	/**
	 * Build bounded candidate lists from stable spatial-cell keys and body ordering.
	 */
	void broadPhase(StateView state, const WorldConfig& config);
	/**
	 * Generate supported body/body contacts from the broad-phase candidate lists.
	 */
	void bodyContacts(StateView state, const WorldConfig& config);
	/**
	 * Generate contacts between active bodies and the six static world-box faces.
	 */
	void worldContacts(StateView state, const WorldConfig& config);
	/**
	 * Resolve the colored contact constraints using the configured iteration count.
	 */
	void solver(StateView state, const WorldConfig& config);
	/**
	 * Integrate body placement and apply the final world-boundary position constraint.
	 */
	void position(StateView state, const WorldConfig& config);
	/**
	 * Update quiet-body counters and sleeping flags after contact solving.
	 */
	void sleep(StateView state, const WorldConfig& config);
	/**
	 * Collect final energy, motion, contact and numerical-validity diagnostics for this step.
	 */
	void diagnostics(StateView state, const WorldConfig& config);

public:
	// --- Telemetry ---
	/**
	 * Enable or disable collection of confirmed collision-impact records.
	 *
	 * @param enabled Whether confirmed impact records should be retained by the backend.
	 */
	void setImpactTracking(bool enabled) override;
	/**
	 * Return a borrowed view of the most recently retained visual impact data.
	 */
	std::span<const ImpactEvent> impacts() const override;


	// --- Metadata ---
	/**
	 * Return the stable backend identifier used in configuration and reports.
	 */
	std::string name() const override { return "cuda";  }
	/**
	 * Return the processor or GPU description recorded with simulation evidence.
	 */
	std::string deviceInfo() const override { return device; }

	/**
	 * Allocate fixed-capacity device scratch/state buffers and query the selected CUDA device.
	 *
	 * @param capacity Maximum number of bodies allocated by this owner.
	 */
	explicit CudaBackend(int capacity);

	// --- Data Transfer ---
	/**
	 * Transfer the active host state into backend execution storage.
	 *
	 * @param state Host simulation arrays, active body count and authoritative tick in SI units.
	 */
	void upload(const State& state) override;
	/**
	 * Copy the current backend body arrays into the caller's host state.
	 *
	 * @param state Host simulation arrays, active body count and authoritative tick in SI units.
	 */
	void download(State& state) override;

	// --- Execution ---
	/**
	 * Execute one configured fixed simulation tick and return its measured stage diagnostics.
	 *
	 * @param state  Host simulation arrays, active body count and authoritative tick in SI units.
	 * @param config Fixed-step world, material and solver settings; physical quantities use SI units.
	 * @return Stage timings and diagnostics for the completed fixed simulation tick.
	 */
	StepResult step(State& state, const WorldConfig& config) override;
};

} // namespace sim
