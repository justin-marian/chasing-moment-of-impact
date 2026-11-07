/**
 * @file
 * Aggregate deterministic CPU/CUDA physics stages into a single interface.
 *
 * Definitions are split into focused host/device headers so that both the
 * CPU and CUDA backends compile the exact same operations.
 */

#pragma once

#include "shared/sim/dynamics.h"
#include "shared/sim/contacts.h"
#include "shared/sim/manifolds.h"
#include "shared/sim/solver.h"
#include "shared/sim/coloring.h"
#include "shared/sim/broad_phase.h"
#include "shared/sim/diagnostics.h"
