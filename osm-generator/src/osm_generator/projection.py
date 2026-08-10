from __future__ import annotations

from pyproj import CRS, Transformer

from .models import BoundingBox, ProjectedSquare, SquareRegion

WGS84 = CRS.from_epsg(4326)


def local_utm_crs(lon: float, lat: float) -> CRS:
    if not (-80.0 <= lat <= 84.0):
        raise ValueError("UTM is only supported between 80°S and 84°N")
    zone = min(60, max(1, int((lon + 180.0) // 6.0) + 1))
    epsg = (32600 if lat >= 0 else 32700) + zone
    return CRS.from_epsg(epsg)


def projected_square(region: SquareRegion) -> ProjectedSquare:
    region.validate()
    crs = local_utm_crs(region.center_lon, region.center_lat)
    forward = Transformer.from_crs(WGS84, crs, always_xy=True)
    inverse = Transformer.from_crs(crs, WGS84, always_xy=True)

    center_x, center_y = forward.transform(region.center_lon, region.center_lat)
    half = region.side_length_m / 2.0
    xmin, ymin = center_x - half, center_y - half
    xmax, ymax = center_x + half, center_y + half

    corners = [
        inverse.transform(xmin, ymin),
        inverse.transform(xmin, ymax),
        inverse.transform(xmax, ymin),
        inverse.transform(xmax, ymax),
    ]
    longitudes, latitudes = zip(*corners, strict=True)
    query_bbox = BoundingBox(
        west=min(longitudes),
        south=min(latitudes),
        east=max(longitudes),
        north=max(latitudes),
    )
    query_bbox.validate()

    return ProjectedSquare(
        crs=crs.to_string(),
        xmin=xmin,
        ymin=ymin,
        xmax=xmax,
        ymax=ymax,
        query_bbox_wgs84=query_bbox,
    )


def square_region_from_bbox(bbox: BoundingBox) -> SquareRegion:
    bbox.validate()
    center_lon = (bbox.west + bbox.east) / 2.0
    center_lat = (bbox.south + bbox.north) / 2.0
    crs = local_utm_crs(center_lon, center_lat)
    forward = Transformer.from_crs(WGS84, crs, always_xy=True)

    projected_corners = [
        forward.transform(lon, lat)
        for lon, lat in (
            (bbox.west, bbox.south),
            (bbox.west, bbox.north),
            (bbox.east, bbox.south),
            (bbox.east, bbox.north),
        )
    ]
    xs, ys = zip(*projected_corners, strict=True)
    side_length_m = max(max(xs) - min(xs), max(ys) - min(ys))

    return SquareRegion(
        center_lon=center_lon,
        center_lat=center_lat,
        side_length_m=side_length_m,
    )
