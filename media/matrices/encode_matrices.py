import json
import time
from pathlib import Path

from PIL import Image
from PIL.ImageFile import ImageFile

ROOT = Path(__file__).parent

# Hardcoded crop box (left, upper, right, lower) to isolate the canvas from the UI.
CROP_BOX = (166, 130, 858, 706)
# Animation parameters.
TOTAL_TICKS = 720
TICK_STEP = 4
TOTAL_FRAMES = TOTAL_TICKS // TICK_STEP
FRAME_DURATION_MS = [30, 30, 40] * (TOTAL_FRAMES // 3)  # Averages to 33.33ms (30 FPS) for a 6000ms total.
EXPECTED_GIF_DURATION_MS = 6000
# Palette generation grid.
SAMPLE_COLS = 5
SAMPLE_ROWS = 4
TILE_WIDTH = 200
TILE_HEIGHT = 166
REFERENCE_FRAME_TICK = 176
METADATA_TIMEOUT_S = 300


def read_frame(path: Path) -> Image.Image:
	"""
	Load an image from disk, convert it to RGB, and crop it to the canvas area.

	@param path: Path to the source PNG frame.
	@return: A cropped, RGB-converted PIL Image.
	"""
	with Image.open(path) as image:
		return image.convert("RGB").crop(CROP_BOX)


def wait_for_metadata(folder: Path) -> dict[str, Any]:
	"""
	Poll for the render.json metadata file until it exists and is fully written.

	@param folder: The directory containing the simulation output.
	@return: Parsed JSON metadata.
	@raises RuntimeError: If the metadata file does not appear within the timeout.
	"""
	deadline = time.monotonic() + METADATA_TIMEOUT_S
	metadata_path = folder / "render.json"

	while time.monotonic() < deadline:
		try:
			return json.loads(metadata_path.read_text())
		except (FileNotFoundError, json.JSONDecodeError):
			time.sleep(1)

	raise RuntimeError(f"Capture did not finish within {METADATA_TIMEOUT_S}s: {folder}")


def build_palette(sample_paths: list[Path]) -> Image.Image:
	"""
	Generate a global 256-color palette by stitching together representative frames.

	@param sample_paths: List of paths to a subset of frames.
	@return: A quantized PIL Image representing the shared color palette.
	"""
	sample = Image.new("RGB", (TILE_WIDTH * SAMPLE_COLS, TILE_HEIGHT * SAMPLE_ROWS))

	for index, path in enumerate(sample_paths):
		tile = read_frame(path).resize((TILE_WIDTH, TILE_HEIGHT), Image.Resampling.BOX)
		position = ((index % SAMPLE_COLS) * TILE_WIDTH, (index // SAMPLE_COLS) * TILE_HEIGHT)
		sample.paste(tile, position)

	return sample.quantize(colors=256)


def verify_gif(gif_path: Path) -> None:
	"""
	Verify the exported GIF is animated, loops infinitely, and has the correct total duration.

	@param gif_path: Path to the saved GIF.
	@raises RuntimeError: If any GIF property is invalid.
	"""
	with cast(ImageFile, Image.open(gif_path)) as image:
		if not getattr(image, "is_animated", False):
			raise RuntimeError(f"{gif_path.name} is not animated.")

		if image.info.get("loop") != 0:
			raise RuntimeError(f"{gif_path.name} does not loop infinitely.")

		duration = 0
		for index in range(getattr(image, "n_frames")):
			image.seek(index)
			duration += image.info.get("duration", 0)

		if duration != EXPECTED_GIF_DURATION_MS:
			raise RuntimeError(f"{gif_path.name} duration is {duration}ms, expected {EXPECTED_GIF_DURATION_MS}ms.")

		print(
			f"{gif_path.name}: {getattr(image, "n_frames")} frames, {duration} ms, "
			f"{gif_path.stat().st_size:,} bytes",
			flush=True
		)


def export(name: str) -> str:
	"""
	Process a simulation run into a GIF and a reference PNG, and return the state hash.

	@param name: The scenario identifier (e.g., 'cpu-A', 'cuda-B').
	@return: The final state hash string from the render metadata.
	"""
	source = ROOT / "frames" / name
	metadata = wait_for_metadata(source)

	frames = [source / f"render-{tick}.png" for tick in range(0, TOTAL_TICKS, TICK_STEP)]

	# Build a shared palette from every 9th frame to ensure consistent colors.
	palette = build_palette(frames[::9])

	# Quantize all frames using the shared palette and disable dithering to prevent noise.
	quantized_frames = [
		read_frame(path).quantize(palette=palette, dither=Image.Dither.NONE)
		for path in frames
	]

	gif_path = ROOT / f"{name}.gif"
	quantized_frames[0].save(
		gif_path,
		save_all=True,
		append_images=quantized_frames[1:],
		duration=FRAME_DURATION_MS,
		loop=0,
		optimize=True
	)

	# Save a high-quality reference frame as a standard PNG.
	png_path = ROOT / f"{name}.png"
	read_frame(source / f"render-{REFERENCE_FRAME_TICK}.png").save(png_path)

	# Validate outputs.
	verify_gif(gif_path)
	with Image.open(png_path) as image:
		image.verify()

	return metadata["stateHash"]


def main() -> None:
	"""Run the export pipeline for all matrix scenarios."""
	for case in ("A", "B", "C"):
		cpu_hash = export(f"cpu-{case}")
		cuda_hash = export(f"cuda-{case}")

		print(f"Matrix {case} final state hashes: CPU {cpu_hash}, CUDA {cuda_hash}", flush=True)


if __name__ == "__main__":
	main()
