from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class BoundingBox:
    west: float
    south: float
    east: float
    north: float

    def validate(self) -> None:
        if not (-180.0 <= self.west < self.east <= 180.0):
            raise ValueError("require -180 <= west < east <= 180")
        if not (-90.0 <= self.south < self.north <= 90.0):
            raise ValueError("require -90 <= south < north <= 90")

    def as_osmnx(self) -> tuple[float, float, float, float]:
        self.validate()
        return self.west, self.south, self.east, self.north


@dataclass(frozen=True)
class SquareRegion:
    center_lon: float
    center_lat: float
    side_length_m: float

    def validate(self) -> None:
        if not (-180.0 <= self.center_lon <= 180.0):
            raise ValueError("center_lon must be in [-180, 180]")
        if not (-80.0 <= self.center_lat <= 84.0):
            raise ValueError("center_lat must be in [-80, 84] for UTM projection")
        if self.side_length_m <= 0:
            raise ValueError("side_length_m must be positive")


@dataclass(frozen=True)
class GridSpec:
    size: int

    def validate(self) -> None:
        if self.size <= 0:
            raise ValueError("grid size must be positive")

    @property
    def width(self) -> int:
        return self.size

    @property
    def height(self) -> int:
        return self.size


@dataclass(frozen=True)
class ProjectedSquare:
    crs: str
    xmin: float
    ymin: float
    xmax: float
    ymax: float
    query_bbox_wgs84: BoundingBox

    @property
    def side_length_m(self) -> float:
        return self.xmax - self.xmin

@dataclass(frozen=True)
class NativeExport:
    pass

@dataclass(frozen=True)
class MapfExport:
    scenario_count: int = 10
    agents_start: int = 10
    agents_increment: int = 10
    seed: int = 0
    map_name: str | None = None

Export = NativeExport | MapfExport
