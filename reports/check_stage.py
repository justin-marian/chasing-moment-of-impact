"""
Run the same seeded CPU/CUDA demo and verify its initial population.

This script executes a physics simulation on both the CPU and CUDA backends,
validates that the generated initial state matches expected constraints (bounding
boxes, non-overlapping bodies, staggered spawn bands), extracts performance
metrics, and compares the final state hashes.
"""

import csv
import json
import math
import statistics
import subprocess
import sys
from pathlib import Path
from typing import Any


# Resolve the project root dynamically (assumes script is in reports/scripts/).
ROOT = Path(__file__).resolve().parents[2]
WORK_DIR = Path(__file__).parent
BIN_DIR = ROOT / "exports" / "builds" / "windows-x64"

# Simulation constraints.
EXPECTED_BOX_EXTENTS = [12, 15, 12]
EXPECTED_BODY_COUNT = 120
DEFAULT_SEED = 42
STAGGERED_BANDS = 3
STAGGERED_HORIZONTAL_FRACTION = 0.8

# Metric thresholds.
FLOAT_TOLERANCE = 1e-6
P95_PERCENTILE = 0.95


def run_simulation(*args: Any) -> str:
	"""
	Execute the compiled physics binary and return its standard output.

	@param args: Positional arguments passed to the executable.
	@return: Captured standard output string.
	@raises RuntimeError: If the subprocess exits with a non-zero status.
	"""
	try:
		return subprocess.check_output(
			[str(arg) for arg in args],
			cwd=ROOT,
			text=True,
			stderr=subprocess.PIPE
		)
	except subprocess.CalledProcessError as error:
		raise RuntimeError(f"Simulation failed: {error.stderr}") from error


def compute_radius(body: dict[str, Any]) -> float:
	"""
	Calculate the bounding radius for a given body.

	@param body: Dictionary representing a single rigid body.
	@return: The bounding radius (analytic for spheres, diagonal for OBBs).
	"""
	extents = body["halfExtents_m"]

	# Shape 0 is a sphere (radius = half extent).
	if body["shape_i"] == 0:
		return extents[0]

	# Other shapes use their OBB bounding sphere.
	return math.hypot(*extents)


def check_initial_state(directory: Path, expected_radius: float) -> None:
	"""
	Validate the initial state of the simulation against physical constraints.

	@param directory: The path to the run directory containing the JSON states.
	@param expected_radius: The expected base radius of the spawned bodies.
	@raises AssertionError: If any spatial or configuration constraint is violated.
	"""
	config = json.loads((directory / "config.json").read_text())
	bodies = json.loads((directory / "initial.json").read_text())["bodies"]

	# Validate global configuration.
	assert config["boxHalfExtents_m"] == EXPECTED_BOX_EXTENTS, config
	assert abs(config["radius_m"] - expected_radius) < FLOAT_TOLERANCE, config
	assert len(bodies) == EXPECTED_BODY_COUNT, len(bodies)
	assert {body["shape_i"] for body in bodies} == {0, 1, 2, 3}, "Missing shapes"

	# Validate individual body bounds and non-overlapping placement.
	for index, body in enumerate(bodies):
		bound = compute_radius(body)

		# Check bounds against the arena walls.
		for position, limit in zip(body["position_m"], config["boxHalfExtents_m"]):
			assert abs(position) + bound <= limit + FLOAT_TOLERANCE, body

		# Check for intersections with previously spawned bodies.
		for other in bodies[:index]:
			distance = math.dist(body["position_m"], other["position_m"])
			assert distance >= bound + compute_radius(other), "Overlap detected"

	# Validate staggered spawn logic if enabled.
	if config.get("staggeredSpawn", False):
		bands = [0] * STAGGERED_BANDS

		for body in bodies:
			limit = config["boxHalfExtents_m"][1] - compute_radius(body)
			y = body["position_m"][1]

			# Determine which height band this body falls into.
			normalized_height = (y + limit) / (2 * limit)
			band_index = min(STAGGERED_BANDS - 1, int(normalized_height * STAGGERED_BANDS))
			bands[band_index] += 1

			# Ensure horizontal placement respects the stagger fraction.
			horizontal_limit = (config["boxHalfExtents_m"][0] - compute_radius(body)) * STAGGERED_HORIZONTAL_FRACTION
			assert abs(body["position_m"][0]) <= horizontal_limit + FLOAT_TOLERANCE
			assert abs(body["position_m"][2]) <= horizontal_limit + FLOAT_TOLERANCE

		assert bands == [EXPECTED_BODY_COUNT // STAGGERED_BANDS] * STAGGERED_BANDS, bands


def extract_metrics(directory: Path) -> dict[str, Any]:
	"""
	Parse the generated frame logs to extract timing and collision metrics.

	@param directory: The path to the run directory containing the CSV logs.
	@return: A dictionary of aggregated statistical metrics.
	"""
	with (directory / "frames.csv").open(newline="") as stream:
		frames = list(csv.DictReader(stream))

	assert all(int(row["nanCount_i"]) == 0 for row in frames), "NaN detected"

	times = sorted(float(row["total_wall_ms"]) for row in frames)

	return {
		"ticks": len(frames),
		"mean_contacts": round(statistics.mean(int(row["contacts_i"]) for row in frames), 2),
		"contact_ticks": sum(int(row["contacts_i"]) > 0 for row in frames),
		"median_ms": round(statistics.median(times), 4),
		"p95_ms": round(times[math.ceil(P95_PERCENTILE * len(times)) - 1], 4),
		"max_wall_projection_m": max(float(row["maxWallProjection_m"]) for row in frames),
		"final_hash": json.loads((directory / "manifest.json").read_text())["replayHash"],
	}


def main() -> None:
	"""
	Run the CPU/CUDA parity test and output the comparative metrics.

	Expects three command-line arguments: stage_name, expected_radius, total_ticks.
	"""
	if len(sys.argv) != 4:
		print(f"Usage: {sys.argv[0]} <stage_name> <expected_radius> <ticks>", file=sys.stderr)
		sys.exit(1)

	stage_name = sys.argv[1]
	expected_radius = float(sys.argv[2])
	ticks = int(sys.argv[3])

	summary = {}

	for backend in ("cpu", "cuda"):
		directory = WORK_DIR / f"{stage_name}-{backend}"

		# Run the simulation.
		result = run_simulation(
			BIN_DIR / "physics.exe",
			"--scenario", "demo",
			"--backend", backend,
			"--seed", str(DEFAULT_SEED),
			"--ticks", str(ticks),
			"--output", directory
		)

		assert "escapeFrames=0" in result, "Bodies escaped the arena"

		# Validate the initial state and extract metrics.
		check_initial_state(directory, expected_radius)
		summary[backend] = extract_metrics(directory)

		assert summary[backend]["ticks"] == ticks
		print(result.strip(), flush=True)

	# Output and save the final summary.
	print(json.dumps(summary, indent=2))
	(WORK_DIR / f"{stage_name}-metrics.json").write_text(json.dumps(summary, indent=2))


if __name__ == "__main__":
	main()
