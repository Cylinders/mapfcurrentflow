from __future__ import annotations

import json
from pathlib import Path
from typing import Any

import numpy as np
from PIL import Image


def save_grid_png(grid: np.ndarray, path: Path) -> None:
    if grid.ndim != 2:
        raise ValueError("grid must be two-dimensional")
    if not np.isin(grid, (0, 1)).all():
        raise ValueError("grid values must be 0 (free) or 1 (obstacle)")
    path.parent.mkdir(parents=True, exist_ok=True)
    Image.fromarray((1 - grid).astype(np.uint8) * 255, mode="L").save(path)


def save_metadata(metadata: dict[str, Any], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as file:
        json.dump(metadata, file, indent=2, sort_keys=True)
        file.write("\n")
