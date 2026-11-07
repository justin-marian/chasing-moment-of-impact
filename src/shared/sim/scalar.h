/**
 * @file
 * Scalar storage shared by host/device types.
 *
 * Defines the fundamental floating-point type used across the physics
 * simulation. Allows toggling between single and double precision at
 * compile time to balance performance and numerical accuracy.
 */

#pragma once


namespace sim {

#ifdef SIM_REFERENCE_DOUBLE
/* Reference builds use double precision for high-accuracy validation. */
using Real = double;
#else
/* Default builds use single precision for maximum CPU/GPU performance. */
using Real = float;
#endif

} // namespace sim
