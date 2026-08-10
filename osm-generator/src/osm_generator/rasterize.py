from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import networkx as nx
import numpy as np
import osmnx as ox
from rasterio.features import rasterize
from rasterio.transform import Affine, from_bounds

from .models import GridSpec, ProjectedSquare

DEFAULT_ROAD_WIDTHS_M = {
    "motorway": 14.0,
    "motorway_link": 8.0,
    "trunk": 12.0,
    "trunk_link": 8.0,
    "primary": 10.0,
    "primary_link": 7.0,
    "secondary": 8.0,
    "secondary_link": 7.0,
    "tertiary": 7.0,
    "tertiary_link": 6.0,
    "residential": 6.0,
    "living_street": 5.0,
    "unclassified": 5.0,
    "service": 4.0,
}


@dataclass(frozen=True)
class RasterizedMap:
    grid: np.ndarray
    transform: Affine
    crs: str
    meters_per_cell: float
    minimum_width_applied: bool


def _first_tag(value: Any) -> str | None:
    if isinstance(value, list):
        return str(value[0]) if value else None
    return str(value) if value is not None else None


def road_width_m(
    edge_data: dict[str, Any],
    *,
    default_width_m: float,
    widths_by_class: dict[str, float],
) -> float:
    explicit = _first_tag(edge_data.get("width"))
    if explicit is not None:
        try:
            parsed = float(explicit.removesuffix(" m").strip())
            if parsed > 0:
                return parsed
        except ValueError:
            pass

    highway = _first_tag(edge_data.get("highway"))
    if highway is not None and highway in widths_by_class:
        return widths_by_class[highway]
    return default_width_m


def rasterize_roads(
    graph: nx.MultiDiGraph,
    extent: ProjectedSquare,
    grid: GridSpec,
    *,
    default_road_width_m: float = 6.0,
    widths_by_class: dict[str, float] | None = None,
    enforce_minimum_cell_width: bool = True,
) -> RasterizedMap:
    grid.validate()
    if default_road_width_m <= 0:
        raise ValueError("default_road_width_m must be positive")

    widths = widths_by_class or DEFAULT_ROAD_WIDTHS_M
    edges = ox.convert.graph_to_gdfs(
        graph,
        nodes=False,
        edges=True,
        fill_edge_geometry=True,
    )
    transform = from_bounds(
        extent.xmin,
        extent.ymin,
        extent.xmax,
        extent.ymax,
        grid.width,
        grid.height,
    )
    meters_per_cell = extent.side_length_m / grid.size
    minimum_applied = False
    shapes = []

    for _, edge in edges.iterrows():
        geometry = edge.geometry
        if geometry is None or geometry.is_empty:
            continue
        width = road_width_m(
            edge.to_dict(),
            default_width_m=default_road_width_m,
            widths_by_class=widths,
        )
        if enforce_minimum_cell_width and width < meters_per_cell:
            width = meters_per_cell
            minimum_applied = True
        shapes.append((geometry.buffer(width / 2.0), 1))

    traversable = rasterize(
        shapes=shapes,
        out_shape=(grid.height, grid.width),
        transform=transform,
        fill=0,
        dtype=np.uint8,
        all_touched=True,
    )
    return RasterizedMap(
        grid=1 - traversable,
        transform=transform,
        crs=extent.crs,
        meters_per_cell=meters_per_cell,
        minimum_width_applied=minimum_applied,
    )
