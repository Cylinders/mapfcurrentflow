from __future__ import annotations

import math
import random
import shutil
import tempfile
import zipfile
from collections.abc import Sequence
from dataclasses import dataclass
from pathlib import Path

import numpy as np

from osm_generator import MapfExport, NetworkType
from osm_generator.models import GridSpec, SquareRegion
from osm_generator.pipeline import Export, NativeExport, generate_road_grid


@dataclass(frozen=True)
class UrbanAnchor:
    name: str
    latitude: float
    longitude: float


@dataclass(frozen=True)
class GeneratedMap:
    anchor_name: str
    center_lat: float
    center_lon: float
    obstacle_coverage: float
    output_dir: Path

SAMPLE_RADIUS_M = 30_000

DEFAULT_URBAN_ANCHORS: tuple[UrbanAnchor, ...] = (
    # North America
    UrbanAnchor("new_york", 40.7128, -74.0060),
    UrbanAnchor("philadelphia", 39.9526, -75.1652),
    UrbanAnchor("boston", 42.3601, -71.0589),
    UrbanAnchor("washington_dc", 38.9072, -77.0369),
    UrbanAnchor("chicago", 41.8781, -87.6298),
    UrbanAnchor("detroit", 42.3314, -83.0458),
    UrbanAnchor("atlanta", 33.7490, -84.3880),
    UrbanAnchor("dallas", 32.7767, -96.7970),
    UrbanAnchor("houston", 29.7604, -95.3698),
    UrbanAnchor("denver", 39.7392, -104.9903),
    UrbanAnchor("los_angeles", 34.0522, -118.2437),
    UrbanAnchor("san_francisco", 37.7749, -122.4194),
    UrbanAnchor("san_jose", 37.3382, -121.8863),
    UrbanAnchor("seattle", 47.6062, -122.3321),
    UrbanAnchor("phoenix", 33.4484, -112.0740),
    UrbanAnchor("toronto", 43.6532, -79.3832),
    UrbanAnchor("montreal", 45.5017, -73.5673),
    UrbanAnchor("vancouver", 49.2827, -123.1207),
    UrbanAnchor("mexico_city", 19.4326, -99.1332),

    # Europe
    UrbanAnchor("london", 51.5074, -0.1278),
    UrbanAnchor("paris", 48.8566, 2.3522),
    UrbanAnchor("berlin", 52.5200, 13.4050),
    UrbanAnchor("hamburg", 53.5511, 9.9937),
    UrbanAnchor("munich", 48.1351, 11.5820),
    UrbanAnchor("amsterdam", 52.3676, 4.9041),
    UrbanAnchor("brussels", 50.8503, 4.3517),
    UrbanAnchor("madrid", 40.4168, -3.7038),
    UrbanAnchor("barcelona", 41.3874, 2.1686),
    UrbanAnchor("milan", 45.4642, 9.1900),
    UrbanAnchor("rome", 41.9028, 12.4964),
    UrbanAnchor("vienna", 48.2082, 16.3738),
    UrbanAnchor("prague", 50.0755, 14.4378),
    UrbanAnchor("warsaw", 52.2297, 21.0122),
    UrbanAnchor("stockholm", 59.3293, 18.0686),

    # Asia
    UrbanAnchor("tokyo", 35.6762, 139.6503),
    UrbanAnchor("osaka", 34.6937, 135.5023),
    UrbanAnchor("seoul", 37.5665, 126.9780),
    UrbanAnchor("taipei", 25.0330, 121.5654),
    UrbanAnchor("beijing", 39.9042, 116.4074),
    UrbanAnchor("shanghai", 31.2304, 121.4737),
    UrbanAnchor("guangzhou", 23.1291, 113.2644),
    UrbanAnchor("bangkok", 13.7563, 100.5018),
    UrbanAnchor("singapore", 1.3521, 103.8198),
    UrbanAnchor("delhi", 28.6139, 77.2090),
    UrbanAnchor("mumbai", 19.0760, 72.8777),
    UrbanAnchor("bengaluru", 12.9716, 77.5946),
    UrbanAnchor("istanbul", 41.0082, 28.9784),

    # South America
    UrbanAnchor("sao_paulo", -23.5505, -46.6333),
    UrbanAnchor("rio_de_janeiro", -22.9068, -43.1729),
    UrbanAnchor("buenos_aires", -34.6037, -58.3816),
    UrbanAnchor("santiago", -33.4489, -70.6693),
    UrbanAnchor("bogota", 4.7110, -74.0721),
    UrbanAnchor("lima", -12.0464, -77.0428),

    # Oceania
    UrbanAnchor("sydney", -33.8688, 151.2093),
    UrbanAnchor("melbourne", -37.8136, 144.9631),
    UrbanAnchor("brisbane", -27.4698, 153.0251),
    UrbanAnchor("perth", -31.9523, 115.8613),

    # Africa
    UrbanAnchor("johannesburg", -26.2041, 28.0473),
    UrbanAnchor("cape_town", -33.9249, 18.4241),
    UrbanAnchor("nairobi", -1.2921, 36.8219),
    UrbanAnchor("cairo", 30.0444, 31.2357),
)

meters_per_degree = 111_320.0

def sample_near_anchor(
        anchor: UrbanAnchor,
        rng: random.Random,
) -> tuple[float, float]:
    distance_m = SAMPLE_RADIUS_M * math.sqrt(rng.random())
    direction_rad = rng.uniform(0.0, 2.0 * math.pi)

    north_m = distance_m * math.cos(direction_rad)
    east_m = distance_m * math.sin(direction_rad)

    # one degree of latitude is approximately the same physical distance everywhere on Earth.
    latitude_offset = north_m / meters_per_degree

    # longitude lines converge toward the poles, so one degree of longitude represents fewer meters as latitude increases.
    longitude_scale = meters_per_degree * math.cos(math.radians(anchor.latitude))
    longitude_offset = east_m / longitude_scale

    return (
        anchor.latitude + latitude_offset,
        anchor.longitude + longitude_offset,
    )

def obstacle_coverage(grid: np.ndarray) -> float:
    if grid.ndim != 2:
        raise ValueError(f"Expected a 2D grid, received {grid.shape}")

    if grid.size == 0:
        raise ValueError("Cannot measure an empty grid")

    return float(np.count_nonzero(grid != 0) / grid.size)


def connected_traversable_coverage(grid: np.ndarray) -> float:
    """
    Return the fraction of all grid cells contained in the largest connected
    traversable component.

    We can reject maps containing enough roads overall but only as disconnected
    fragments.
    """
    if grid.ndim != 2:
        raise ValueError(f"Expected a 2D grid, received {grid.shape}")

    free = grid == 0
    height, width = free.shape
    visited = np.zeros_like(free, dtype=bool)
    largest_component = 0

    for start_y in range(height):
        for start_x in range(width):
            if not free[start_y, start_x] or visited[start_y, start_x]:
                continue

            component_size = 0
            stack = [(start_y, start_x)]
            visited[start_y, start_x] = True

            while stack:
                y, x = stack.pop()
                component_size += 1

                for next_y, next_x in (
                        (y - 1, x),
                        (y + 1, x),
                        (y, x - 1),
                        (y, x + 1),
                ):
                    if (
                            0 <= next_y < height
                            and 0 <= next_x < width
                            and free[next_y, next_x]
                            and not visited[next_y, next_x]
                    ):
                        visited[next_y, next_x] = True
                        stack.append((next_y, next_x))

            largest_component = max(largest_component, component_size)

    return largest_component / grid.size


def generate_random_grids(
        *,
        n: int,
        output_dir: Path,
        grid_size: int,
        side_length_m: float,
        anchors: Sequence[UrbanAnchor] = DEFAULT_URBAN_ANCHORS,
        minimum_obstacle_coverage: float = 0.0,
        maximum_obstacle_coverage: float,
        minimum_connected_traversable_coverage: float = 0.0,
        network_type: NetworkType = "drive",
        default_road_width_m: float = 6.0,
        enforce_minimum_cell_width: bool = True,
        export: Export = NativeExport(),
        seed: int | None = None,
        maximum_total_attempts: int | None = None,
) -> list[GeneratedMap]:
    if n <= 0:
        raise ValueError("n must be positive")

    if not 0.0 <= minimum_obstacle_coverage <= 1.0:
        raise ValueError("minimum_obstacle_coverage must be in [0, 1]")

    if not 0.0 <= maximum_obstacle_coverage <= 1.0:
        raise ValueError("maximum_obstacle_coverage must be in [0, 1]")

    if minimum_obstacle_coverage > maximum_obstacle_coverage:
        raise ValueError("minimum_obstacle_coverage cannot exceed maximum_obstacle_coverage")

    if not 0.0 <= minimum_connected_traversable_coverage <= 1.0:
        raise ValueError("minimum_connected_traversable_coverage must be in [0, 1]")

    if maximum_total_attempts is None:
        maximum_total_attempts = max(100, n * 20)

    if maximum_total_attempts <= 0:
        raise ValueError("maximum_total_attempts must be positive")

    if not anchors:
        raise ValueError("At least one urban anchor is required")

    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    rng = random.Random(seed)

    accepted: list[GeneratedMap] = []
    attempts = 0

    while len(accepted) < n and attempts < maximum_total_attempts:
        attempts += 1
        anchor = rng.choice(anchors)
        center_lat, center_lon = sample_near_anchor(anchor, rng)

        index = len(accepted)
        final_output = output_dir / f"grid_{index:05d}"

        if final_output.exists():
            raise FileExistsError(f"Refusing to overwrite existing output: {final_output}")

        with tempfile.TemporaryDirectory(
                prefix=f".grid_{index:05d}_",
                dir=output_dir,
        ) as temporary_directory:
            temporary_output = Path(temporary_directory)

            try:
                generated = generate_road_grid(
                    region=SquareRegion(
                        center_lon=center_lon,
                        center_lat=center_lat,
                        side_length_m=side_length_m,
                    ),
                    grid=GridSpec(size=grid_size),
                    output_dir=temporary_output,
                    network_type=network_type,
                    default_road_width_m=default_road_width_m,
                    enforce_minimum_cell_width=enforce_minimum_cell_width,
                    export=export,
                )
            except Exception as error:
                print(
                    f"[{len(accepted)}/{n}] attempt {attempts}: "
                    f"{anchor.name} fetch failed: {error}"
                )
                continue

            coverage = obstacle_coverage(generated.raster.grid)
            connected_coverage = connected_traversable_coverage(generated.raster.grid)

            rejection_reason: str | None = None

            if coverage < minimum_obstacle_coverage:
                rejection_reason = f"obstacle coverage {coverage:.2%} below minimum"
            elif coverage > maximum_obstacle_coverage:
                rejection_reason = f"obstacle coverage {coverage:.2%} above maximum"
            elif connected_coverage < minimum_connected_traversable_coverage:
                rejection_reason = f"largest free component {connected_coverage:.2%} below minimum"

            if rejection_reason is not None:
                print(
                    f"[{len(accepted)}/{n}] attempt {attempts}: "
                    f"{anchor.name}, rejected: {rejection_reason}"
                )
                continue

            shutil.move(str(temporary_output), str(final_output))

            result = GeneratedMap(
                anchor_name=anchor.name,
                center_lat=center_lat,
                center_lon=center_lon,
                obstacle_coverage=coverage,
                output_dir=final_output,
            )
            accepted.append(result)

            print(
                f"[{len(accepted)}/{n}] attempt {attempts}: "
                f"{anchor.name}, accepted, "
                f"obstacles={coverage:.2%}, "
                f"largest-free-component={connected_coverage:.2%}"
            )

    if len(accepted) != n:
        raise RuntimeError(f"Generated {len(accepted)}/{n} maps after {attempts} attempts.")

    return accepted

@dataclass(frozen=True)
class PackagedDataset:
    path: Path
    map_count: int
    scenario_count: int


def package_mapf_dataset(
    generated: Sequence[GeneratedMap],
    *,
    output_path: Path,
    overwrite: bool = False,
) -> PackagedDataset:
    """
    Package generated outputs into a MAPF-formatted ZIP:
        maps/*.map
        scenarios/*.scen
    """
    if not generated:
        raise ValueError("Cannot package an empty dataset")

    archive_path = output_path.resolve().with_suffix(".zip")

    if archive_path.exists() and not overwrite:
        raise FileExistsError(f"Archive already exists: {archive_path}")

    files: list[tuple[Path, str]] = []

    # Collect the MAPF files from each structured output directory.
    for result in generated:
        source_dir = result.output_dir
        map_files = list(source_dir.glob("*.map"))

        if len(map_files) != 1:
            raise ValueError(
                f"Expected one .map file in {source_dir}, found {len(map_files)}"
            )

        map_file = map_files[0]
        files.append((map_file, f"maps/{map_file.name}"))

        files.extend(
            (scenario_file, f"scenarios/{scenario_file.name}")
            for scenario_file in sorted(
                (source_dir / "scenarios").glob("*.scen")
            )
        )


    archive_path.parent.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(
        archive_path,
        mode="w",
        compression=zipfile.ZIP_DEFLATED,
    ) as archive:
        for source_path, archive_name in files:
            archive.write(source_path, arcname=archive_name)

    map_count = len(generated)
    scenario_count = len(files) - map_count

    return PackagedDataset(
        path=archive_path,
        map_count=map_count,
        scenario_count=scenario_count,
    )

def main() -> None:
    dataset_path = Path("generated/world-dataset-512-v4")

    generated = generate_random_grids(
        n=100,
        minimum_obstacle_coverage=0.20,
        maximum_obstacle_coverage=0.85,
        minimum_connected_traversable_coverage=0.10,
        output_dir=dataset_path,
        side_length_m=2000.0,
        grid_size=512,
        network_type="drive",
        default_road_width_m=12.0,
        export=MapfExport(
            scenario_count=25,
            agents_start=10,
            agents_increment=10,
            seed=42,
        ),
        seed=42,
    )

    package = package_mapf_dataset(
        generated,
        output_path=dataset_path.with_suffix(".zip"),
        overwrite=True,
    )

    print(f"Dataset directory: {dataset_path.resolve()}")
    print(f"MAPF package: {package.path} ({package.map_count} maps, {package.scenario_count} scenarios)")

if __name__ == "__main__":
    main()
