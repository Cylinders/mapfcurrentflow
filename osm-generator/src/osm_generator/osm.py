import hashlib
import json
from collections import OrderedDict
from collections.abc import Iterator
from contextlib import contextmanager
from pathlib import Path
from typing import Any, Literal

import networkx as nx
import osmnx as ox
import requests
from osmnx import _http, _overpass, settings
from osmnx._errors import (
    InsufficientResponseError,
    ResponseStatusCodeError,
)

from .models import BoundingBox

NetworkType = Literal[
    "all",
    "all_public",
    "bike",
    "drive",
    "drive_service",
    "walk",
]

OVERPASS_BACKENDS = (
    "https://maps.mail.ru/osm/tools/overpass/api",
    "https://overpass.private.coffee/api",
    "https://overpass-api.de/api",
)

REQUEST_TIMEOUT_SECONDS = 15

def overpass_cache_key(data: OrderedDict[str, Any]) -> str:
    payload = json.dumps(data,sort_keys=True,separators=(",", ":")).encode()

    return f"overpass://{hashlib.sha256(payload).hexdigest()}"


class OverpassBackendPool:
    def __init__(self) -> None:
        self.next_backend = 0

    def request(self, data: OrderedDict[str, Any],) -> dict[str, Any]:
        failures: list[str] = []

        for _ in OVERPASS_BACKENDS:
            backend = OVERPASS_BACKENDS[self.next_backend]
            self.next_backend = (self.next_backend + 1) % len(OVERPASS_BACKENDS)

            try:
                return self._request_once(backend, data)
            except (
                requests.RequestException,
                InsufficientResponseError,
                ResponseStatusCodeError,
            ) as error:
                failures.append(f"{backend}: {error}")

        details = "\n".join(f"  - {failure}" for failure in failures)
        raise RuntimeError(
            "Every Overpass backend failed:\n"
            f"{details}"
        )

    @staticmethod
    def _request_once(
        backend: str,
        data: OrderedDict[str, Any],
    ) -> dict[str, Any]:
        cache_key = overpass_cache_key(data)

        cached = _http._retrieve_from_cache(cache_key)
        if isinstance(cached, dict):
            return cached

        _http._config_dns(backend)

        response = requests.post(
            f"{backend}/interpreter",
            data=data,
            timeout=REQUEST_TIMEOUT_SECONDS,
            headers=_http._get_http_headers(),
            **settings.requests_kwargs,
        )

        response_json = _http._parse_response(response)

        if not isinstance(response_json, dict):
            raise InsufficientResponseError("Overpass API did not return a JSON object")

        _http._save_to_cache(
            cache_key,
            response_json,
            response.ok,
        )
        return response_json


@contextmanager
def use_overpass_pool(
    pool: OverpassBackendPool,
) -> Iterator[None]:
    """
    Temporarily replace OSMnx's Overpass request function.

    This mutates process-global state and therefore is not thread-safe.
    """
    original_request = _overpass._overpass_request
    _overpass._overpass_request = pool.request

    try:
        yield
    finally:
        _overpass._overpass_request = original_request


def fetch_road_graph(
    bbox: BoundingBox,
    *,
    network_type: NetworkType = "drive",
    simplify: bool = True,
    retain_all: bool = True,
    backend_pool: OverpassBackendPool | None = None,
) -> nx.MultiDiGraph:
    bbox.validate()
    pool = backend_pool or OverpassBackendPool()

    with use_overpass_pool(pool):
        return ox.graph.graph_from_bbox(
            bbox.as_osmnx(),
            network_type=network_type,
            simplify=simplify,
            retain_all=retain_all,
            truncate_by_edge=True,
        )

def project_road_graph(
    graph: nx.MultiDiGraph,
    crs: str,
) -> nx.MultiDiGraph:
    return ox.projection.project_graph(graph, to_crs=crs)

def save_road_graph(
    graph: nx.MultiDiGraph,
    path: Path,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    ox.io.save_graphml(graph, filepath=path)

def load_road_graph(path: Path) -> nx.MultiDiGraph:
    return ox.io.load_graphml(filepath=path)
