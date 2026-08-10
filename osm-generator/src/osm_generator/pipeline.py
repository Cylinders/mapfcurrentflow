from __future__ import annotations

from dataclasses import dataclass
from datetime import UTC, datetime
from pathlib import Path

import networkx as nx
import osmnx as ox

from .export import save_grid_png, save_metadata
from .mapf import export_mapf
from .models import (
    Export,
    GridSpec,
    MapfExport,
    NativeExport,
    ProjectedSquare,
    SquareRegion,
)
from .osm import NetworkType, fetch_road_graph, project_road_graph, save_road_graph
from .projection import projected_square
from .rasterize import DEFAULT_ROAD_WIDTHS_M, RasterizedMap, rasterize_roads


@dataclass(frozen=True)
class GeneratedRoadGrid:
    raster: RasterizedMap
    extent: ProjectedSquare
    source_graph: nx.MultiDiGraph
    projected_graph: nx.MultiDiGraph

def generate_road_grid(
    *,
    region: SquareRegion,
    grid: GridSpec,
    output_dir: Path,
    network_type: NetworkType = "drive",
    default_road_width_m: float = 6.0,
    enforce_minimum_cell_width: bool = True,
    export: Export = NativeExport(),
) -> GeneratedRoadGrid:
    region.validate()
    grid.validate()
    extent = projected_square(region)

    source_graph = fetch_road_graph(
        extent.query_bbox_wgs84,
        network_type=network_type,
    )
    projected_graph = project_road_graph(source_graph, extent.crs)
    raster = rasterize_roads(
        projected_graph,
        extent,
        grid,
        default_road_width_m=default_road_width_m,
        enforce_minimum_cell_width=enforce_minimum_cell_width,
    )
    output_dir.mkdir(parents=True, exist_ok=True)
    save_road_graph(source_graph, output_dir / "graph.graphml")

    export_metadata: dict[str, object]
    match export:
        case NativeExport():
            save_grid_png(raster.grid, output_dir / "grid.png")
            export_metadata = {
                "type": "native",
                "files": ["grid.png"],
            }
        case MapfExport():
            map_path, scenario_paths = export_mapf(
                raster.grid,
                output_dir,
                export=export
            )
            export_metadata = {
                "type": "mapf",
                "map": map_path.name,
                "scenarios": [str(path.relative_to(output_dir)) for path in scenario_paths],
                "scenario_count": export.scenario_count,
                "agents_start": export.agents_start,
                "agents_increment": export.agents_increment,
                "seed": export.seed,
                "preserves_grade_separation": False,
            }

    query = extent.query_bbox_wgs84
    metadata = {
        "region": {
            "center_lon": region.center_lon,
            "center_lat": region.center_lat,
            "side_length_m": region.side_length_m,
        },
        "effective_query_bbox_wgs84": {
            "west": query.west,
            "south": query.south,
            "east": query.east,
            "north": query.north,
        },
        "projected_extent_m": {
            "xmin": extent.xmin,
            "ymin": extent.ymin,
            "xmax": extent.xmax,
            "ymax": extent.ymax,
        },
        "projection": extent.crs,
        "grid": {
            "width": grid.width,
            "height": grid.height,
            "meters_per_cell": raster.meters_per_cell,
            "array_encoding": {"free": 0, "obstacle": 1},
            "png_encoding": {"free": 255, "obstacle": 0},
        },
        "road_width_model": {
            "default_width_m": default_road_width_m,
            "widths_by_highway_class_m": DEFAULT_ROAD_WIDTHS_M,
            "enforce_minimum_cell_width": enforce_minimum_cell_width,
            "minimum_width_was_applied": raster.minimum_width_applied,
        },
        "export": export_metadata,
        "osm": {
            "network_type": network_type,
            "fetched_at": datetime.now(UTC).isoformat(),
            "osmnx_version": ox.__version__,
            "nodes": source_graph.number_of_nodes(),
            "edges": source_graph.number_of_edges(),
        },
    }
    save_metadata(metadata, output_dir / "metadata.json")

    return GeneratedRoadGrid(
        raster=raster,
        extent=extent,
        source_graph=source_graph,
        projected_graph=projected_graph,
    )
