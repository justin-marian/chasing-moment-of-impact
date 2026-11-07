/**
 * @file
 * Shared deterministic host/device broad phase operations.
 *
 * Inline definitions are required to allow CUDA device compilation while
 * sharing the exact same implementation logic with the host CPU.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/dynamics.h"


namespace sim {

/**
 * Convert one world coordinate to the containing broad-phase grid cell.
 *
 * @param position_m World coordinate in metres.
 * @param size_m     Uniform grid cell size in metres.
 * @return Signed integer cell coordinate.
 */
SIM_HD inline int cellCoordinate(Real position_m, Real size_m) {
    return static_cast<int>(floor(position_m / size_m));
}


/**
 * Pack three biased broad-phase cell coordinates into one sortable 64-bit key.
 *
 * @param x Cell X coordinate.
 * @param y Cell Y coordinate.
 * @param z Cell Z coordinate.
 * @return Packed lexicographically sortable cell key.
 */
SIM_HD inline std::uint64_t cellKey(int x, int y, int z) {
    constexpr int X_SHIFT = 42;
    constexpr int Y_SHIFT = 21;
    // Z is implicitly shifted by 0.

    // Add bias to handle negative coordinates, then pack them into a single integer.
    return (std::uint64_t(x + settings::Collisions::BroadPhase::CELL_BIAS) << X_SHIFT) |
           (std::uint64_t(y + settings::Collisions::BroadPhase::CELL_BIAS) << Y_SHIFT) |
           (std::uint64_t(z + settings::Collisions::BroadPhase::CELL_BIAS));
}


/**
 * Return the broad-phase cell key containing one body's center.
 *
 * @param s Shared body-state view.
 * @param i Body index.
 * @param c Physics configuration containing grid cell size.
 * @return Packed cell key.
 */
SIM_HD inline std::uint64_t bodyCellKey(StateView s, int i, const WorldConfig& c) {
    Vec3 p = s.position_m[i];
    
    return cellKey(
        cellCoordinate(p.x, c.cellSize_m), 
        cellCoordinate(p.y, c.cellSize_m),
        cellCoordinate(p.z, c.cellSize_m)
    );
}


/**
 * Find the first sorted broad-phase entry whose key is not less than the target.
 *
 * @param keys  Sorted cell-key array.
 * @param count Number of valid entries.
 * @param key   Cell key to locate.
 * @return Lower-bound index in the range [0, count].
 */
SIM_HD inline int findCellBegin(const std::uint64_t* keys, int count, std::uint64_t key) {
    int low = 0, high = count;
    
    // Standard binary search for the lower bound.
    while (low < high) {
        int mid = (low + high) / 2;
        
        if (keys[mid] < key) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    
    return low;
}


/**
 * Gather broad-phase body candidates from all grid cells reachable by one body.
 *
 * The search radius includes both bounding radii, contact slop and an optional
 * velocity sweep for CCD. Duplicate, self, and reverse pairs are excluded by
 * body index.
 *
 * @param s           Shared body-state view.
 * @param i           Source body index.
 * @param c           Physics configuration.
 * @param dt_s        Fixed step duration.
 * @param maxRadius_m Largest active body bounding radius.
 * @param maxSpeed_mps Largest active body speed.
 * @param keys        Sorted broad-phase cell keys.
 * @param ids         Body IDs ordered with keys.
 * @param candidates  Output candidate body IDs.
 * @param overflow    Set when the candidate buffer capacity is exhausted.
 * @return Number of candidate IDs written.
 */
SIM_HD inline int gatherCandidates(
    StateView s, 
    int i, 
    const WorldConfig& c, 
    Real dt_s, 
    Real maxRadius_m, 
    Real maxSpeed_mps,
    const std::uint64_t* keys, 
    const int* ids, 
    int* candidates, 
    int& overflow
) {
    constexpr int MIN_SPAN = 1;

    Vec3 p = s.position_m[i];
    int xyz[3] = {
        cellCoordinate(p.x, c.cellSize_m), 
        cellCoordinate(p.y, c.cellSize_m),
        cellCoordinate(p.z, c.cellSize_m)
    };

    // Expand the search radius by the maximum possible travel distance if CCD is active.
    Real reach_m = s.boundingRadius_m[i] + maxRadius_m + c.contactSlop_m;
    if (c.enableCcd) {
        reach_m += (length(s.velocity_mps[i]) + maxSpeed_mps) * dt_s;
    }

    // Determine how many grid cells to scan in each direction.
    int span = static_cast<int>(ceil(reach_m / c.cellSize_m));
    if (span < MIN_SPAN) {
        span = MIN_SPAN;
    }

    int count = 0;
    
    // Iterate over the 3D neighborhood of grid cells.
    for (int z = -span; z <= span; ++z) {
        for (int y = -span; y <= span; ++y) {
            for (int x = -span; x <= span; ++x) {
                std::uint64_t key = cellKey(xyz[0] + x, xyz[1] + y, xyz[2] + z);
                
                // Find the first entry in the sorted array for this specific cell.
                int start = findCellBegin(keys, s.count_i, key);
                for (int k = start; k < s.count_i && keys[k] == key; ++k) {
                    int j = ids[k];
                    // Skip self and already-evaluated pairs to avoid duplicates.
                    if (j <= i) {
                        continue;
                    }

                    // Recalculate precise reach for this specific pair, including CCD.
                    Real reach = s.boundingRadius_m[i] + s.boundingRadius_m[j] + c.contactSlop_m;
                    if (c.enableCcd) {
                        reach += (length(s.velocity_mps[i]) + length(s.velocity_mps[j])) * dt_s;
                    }

                    // Squared-distance check to quickly reject pairs that are too far apart.
                    if (dot(s.position_m[j] - p, s.position_m[j] - p) > reach * reach) {
                        continue;
                    }

                    // Abort if the candidate array is full.
                    if (count == settings::Collisions::Storage::MAX_CANDIDATES) {
                        overflow = 1;
                        return count;
                    }

                    candidates[count++] = j;
                }
            }
        }
    }
    
    return count;
}

} // namespace sim
