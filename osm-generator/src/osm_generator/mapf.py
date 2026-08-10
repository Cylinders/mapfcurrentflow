import re
from collections import deque
from pathlib import Path

import numpy as np

from osm_generator import MapfExport


def _validate_grid(grid: np.ndarray) -> None:
    if grid.ndim != 2:
        raise ValueError("grid must be two-dimensional")
    if not np.isin(grid, (0, 1)).all():
        raise ValueError("grid values must be 0 (free) or 1 (obstacle)")


def _map_filename(output_dir: Path, requested_name: str | None) -> str:
    stem = requested_name or output_dir.name or "generated"
    stem = stem.removesuffix(".map")
    stem = re.sub(r"[^A-Za-z0-9_.-]+", "_", stem).strip("._")
    if not stem:
        raise ValueError("map_name must contain at least one filename-safe character")
    return f"{stem}.map"


def write_mapf_map(grid: np.ndarray, path: Path) -> None:
    _validate_grid(grid)
    height, width = grid.shape
    rows = ["".join("." if cell == 0 else "@" for cell in row) for row in grid]
    contents = "\n".join(
        [
            "type octile",
            f"height {height}",
            f"width {width}",
            "map",
            *rows,
            "",
        ]
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(contents, encoding="ascii")


def _largest_free_component(grid: np.ndarray) -> list[tuple[int, int]]:
    height, width = grid.shape
    visited = np.zeros((height, width), dtype=np.bool_)
    largest: list[tuple[int, int]] = []

    for start_row, start_col in np.argwhere(grid == 0):
        start = (int(start_row), int(start_col))
        if visited[start]:
            continue

        component: list[tuple[int, int]] = []
        queue = deque([start])
        visited[start] = True

        while queue:
            row, col = queue.popleft()
            component.append((row, col))
            for neighbor_row, neighbor_col in (
                    (row - 1, col),
                    (row + 1, col),
                    (row, col - 1),
                    (row, col + 1),
            ):
                if not (0 <= neighbor_row < height and 0 <= neighbor_col < width):
                    continue
                if visited[neighbor_row, neighbor_col] or grid[neighbor_row, neighbor_col] != 0:
                    continue
                visited[neighbor_row, neighbor_col] = True
                queue.append((neighbor_row, neighbor_col))

        if len(component) > len(largest):
            largest = component

    return largest


def _write_scenario(
        path: Path,
        *,
        map_filename: str,
        map_width: int,
        map_height: int,
        starts: list[tuple[int, int]],
        goals: list[tuple[int, int]],
) -> None:
    lines = ["version 1.0"]
    for index, ((start_row, start_col), (goal_row, goal_col)) in enumerate(zip(starts, goals, strict=True)):
        lines.append("\t".join(
            map(str, (index, map_filename, map_width, map_height, start_col, start_row, goal_col, goal_row, 0))))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def export_mapf(
        grid: np.ndarray,
        output_dir: Path,
        *,
        export: MapfExport,
) -> tuple[Path, list[Path]]:
    _validate_grid(grid)

    max_required_cells = 2 * (export.agents_start + (export.scenario_count - 1) * export.agents_increment)
    component = _largest_free_component(grid)
    if len(component) < max_required_cells:
        raise ValueError(
            "largest free-space component contains {len(component)} cells, but {max_required_cells} unique start/goal cells are required"
        )

    output_dir.mkdir(parents=True, exist_ok=True)
    map_filename = _map_filename(output_dir, export.map_name)
    map_path = output_dir / map_filename
    write_mapf_map(grid, map_path)

    rng = np.random.default_rng(export.seed)
    height, width = grid.shape
    scenario_paths = []
    agents_n = export.agents_start
    for scenario_index in range(export.scenario_count):
        selected = rng.choice(len(component), size=2*agents_n, replace=False)
        starts = [component[int(index)] for index in selected[: agents_n]]
        goals = [component[int(index)] for index in selected[agents_n:]]

        agents_n += export.agents_increment

        scenario_path = output_dir / "scenarios" / f"{Path(map_filename).stem}-{scenario_index:03d}.scen"
        _write_scenario(
            scenario_path,
            map_filename=map_filename,
            map_width=width,
            map_height=height,
            starts=starts,
            goals=goals,
        )
        scenario_paths.append(scenario_path)

    return map_path, scenario_paths
