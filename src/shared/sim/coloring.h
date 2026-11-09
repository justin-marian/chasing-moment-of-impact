/**
 * @file
 * Shared deterministic host/device coloring operations.
 *
 * Inline definitions are required to allow CUDA device compilation while
 * sharing the exact same implementation logic with the host CPU.
 */

#pragma once

#include "settings/collisions/constants.h"
#include "shared/sim/state.h"


namespace sim {

/**
 * Greedily color contact constraints so contacts in one color never share a body.
 *
 * The deterministic body/slot traversal produces stable color assignment suitable
 * for parallel CPU/CUDA solving without write conflicts.
 *
 * @param s             Shared body-state view.
 * @param contacts      Fixed per-body contact storage.
 * @param contactCounts Valid contact count for each body.
 * @param masks         Per-body color bit masks.
 * @param colors        Per-contact assigned color.
 * @param indices       Output contact indices grouped by color.
 * @param counts        Scratch/output counts per color.
 * @param offsets       Output prefix offsets for each color group.
 * @return Number of used colors, or -1 when the maximum solver colors are exhausted.
 */
SIM_HD inline int colorContacts(
    StateView s,
    const Contact* contacts,
    const int* contactCounts,
    std::uint32_t* masks,
    int* colors,
    int* indices,
    int* counts,
    int* offsets
) {
    constexpr int BITS_PER_WORD = 32;
    constexpr std::uint32_t ALL_BITS_SET = 0xffffffffU;

    // Clear the per-body bitmasks and per-color counters before assignment.
    for (int i = 0; i < s.count_i * settings::Collisions::Solver::COLOR_MASK_WORDS; ++i) {
        masks[i] = 0;
    }

    for (int i = 0; i < settings::Collisions::Solver::MAX_SOLVER_COLORS; ++i) {
        counts[i] = 0;
    }

    int colorCount = 0;

    // Assign the lowest available color to each contact.
    for (int i = 0; i < s.count_i; ++i) {
        for (int slot = 0; slot < contactCounts[i]; ++slot) {
            int index = i * settings::Collisions::Storage::MAX_CONTACTS + slot;
            const Contact& ct = contacts[index];
            int color = -1;

            // Iterate through 32-bit words to find a color not used by either body.
            for (int word = 0; word < settings::Collisions::Solver::COLOR_MASK_WORDS && color < 0; ++word) {
                std::uint32_t used = masks[ct.a_i * settings::Collisions::Solver::COLOR_MASK_WORDS + word];

                if (ct.b_i >= 0) {
                    used |= masks[ct.b_i * settings::Collisions::Solver::COLOR_MASK_WORDS + word];
                }

                if (used == ALL_BITS_SET) {
                    continue;
                }

                // Scan individual bits within the word to find the first free slot.
                for (int bit = 0; bit < BITS_PER_WORD; ++bit) {
                    if (!(used & (1U << bit))) {
                        color = word * BITS_PER_WORD + bit;
                        break;
                    }
                }
            }

            if (color < 0) {
                return -1;
            }

            // Mark the chosen color as used by setting its bit in the body masks.
            std::uint32_t bit = 1U << (color % BITS_PER_WORD);

            masks[ct.a_i * settings::Collisions::Solver::COLOR_MASK_WORDS + color / BITS_PER_WORD] |= bit;
            if (ct.b_i >= 0) {
                masks[ct.b_i * settings::Collisions::Solver::COLOR_MASK_WORDS + color / BITS_PER_WORD] |= bit;
            }

            colors[index] = color;
            ++counts[color];

            if (color + 1 > colorCount) {
                colorCount = color + 1;
            }
        }
    }

    // Prefix sums so each color group gets a contiguous range in the output array.
    // The counters are then reset to zero to act as write cursors.
    offsets[0] = 0;
    for (int color = 0; color < colorCount; ++color) {
        offsets[color + 1] = offsets[color] + counts[color];
        counts[color] = 0;
    }

    // Scatter the contact indices into the output array grouped by their assigned color.
    for (int i = 0; i < s.count_i; ++i) {
        for (int slot = 0; slot < contactCounts[i]; ++slot) {
            int index = i * settings::Collisions::Storage::MAX_CONTACTS + slot;
            int color = colors[index];
            indices[offsets[color] + counts[color]++] = index;
        }
    }

    return colorCount;
}

} // namespace sim
