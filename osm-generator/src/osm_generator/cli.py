from __future__ import annotations

import argparse
from pathlib import Path

from .mapf import MapfExport
from .models import GridSpec, SquareRegion
from .pipeline import Export, NativeExport, generate_road_grid


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a square MAPF road grid from OpenStreetMap.",
    )
    parser.add_argument("--center-lat", type=float, required=True)
    parser.add_argument("--center-lon", type=float, required=True)
    parser.add_argument("--side-length-m", type=float, required=True)
    parser.add_argument("--grid-size", type=int, required=True)
    parser.add_argument(
        "--network-type",
        choices=["all", "all_public", "bike", "drive", "drive_service", "walk"],
        default="drive",
    )
    parser.add_argument("--default-road-width-m", type=float, default=6.0)
    parser.add_argument(
        "--allow-subcell-roads",
        action="store_true",
        help="Do not widen roads below one grid cell before rasterization.",
    )
    parser.add_argument(
        "--exporter",
        choices=["native", "mapf"],
        default="native",
    )
    parser.add_argument("--scenario-count", type=int)
    parser.add_argument("--agents-start", type=int)
    parser.add_argument("--agents-increment", type=int)
    parser.add_argument("--seed", type=int)
    parser.add_argument(
        "--map-name",
        help="MAPF map filename stem; defaults to the output directory name.",
    )
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(argv)

    mapf_only_values = (
        args.scenario_count,
        args.agents_start,
        args.agents_increment,
        args.seed,
        args.map_name,
    )
    if args.exporter == "native" and any(value is not None for value in mapf_only_values):
        parser.error(
            "--scenario-count, --agents-start, --agents-increment, --seed, and --map-name "
            "require --exporter mapf"
        )

    return args


def export_from_args(args: argparse.Namespace) -> Export:
    if args.exporter == "native":
        return NativeExport()
    elif args.exporter == "mapf":
        return MapfExport(
            scenario_count=args.scenario_count if args.scenario_count is not None else 10,
            agents_start=args.agents_start if args.agents_start is not None else 10,
            agents_increment=args.agents_increment if args.agents_increment is not None else 0,
            seed=args.seed if args.seed is not None else 0,
            map_name=args.map_name,
        )
    else:
        raise ValueError(f"Unknown exporter: {args.exporter}")

def main() -> None:
    args = parse_args()
    generated = generate_road_grid(
        region=SquareRegion(
            center_lon=args.center_lon,
            center_lat=args.center_lat,
            side_length_m=args.side_length_m,
        ),
        grid=GridSpec(size=args.grid_size),
        output_dir=args.output,
        network_type=args.network_type,
        default_road_width_m=args.default_road_width_m,
        enforce_minimum_cell_width=not args.allow_subcell_roads,
        export=export_from_args(args),
    )
    dimensions = f"{generated.raster.grid.shape[1]}x{generated.raster.grid.shape[0]}"
    print(f"Saved {dimensions} MAPF dataset to {args.output}")


if __name__ == "__main__":
    main()
