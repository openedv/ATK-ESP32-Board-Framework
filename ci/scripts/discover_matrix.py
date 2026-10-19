#!/usr/bin/env python3
"""Build a GitHub Actions matrix from boards in this repo and the BMGR constraint."""

from __future__ import annotations

import json
import os
import re
import sys
import urllib.error
import urllib.request
from pathlib import Path

import yaml

REPO_ROOT = Path(__file__).resolve().parents[2]
IDF_VERSIONS = ["v5.5.4", "latest"]
IDF_COMPONENT_MANAGER_SPECS = {
    # idf-component-manager 2.4.8 in the v5.5.4 image mishandles BMGR's
    # Kconfig-based dependency conditions. Keep the workaround in the IDF 5.x
    # series; ESP-IDF latest provides its own compatible Component Manager.
    "v5.5.4": "idf-component-manager>=2.5.0,<3.0.0",
    "latest": "",
}
BMGR_DEP_KEYS = (
    "espressif/esp_board_manager",
    "esp_board_manager",
)
MAX_BOARD_DIRECTORY_DEPTH = 3
REGISTRY_VERSIONS_URL = (
    "https://components.espressif.com/api/v1/components/espressif/esp_board_manager/versions"
)


def _load_manifest() -> dict:
    path = REPO_ROOT / "idf_component.yml"
    with path.open(encoding="utf-8") as fh:
        data = yaml.safe_load(fh) or {}
    if not isinstance(data, dict):
        raise SystemExit(f"invalid manifest: {path}")
    return data


def _bmgr_spec(manifest: dict) -> str:
    deps = manifest.get("dependencies") or {}
    for key in BMGR_DEP_KEYS:
        entry = deps.get(key)
        if entry is None:
            continue
        if isinstance(entry, str):
            return entry.strip()
        if isinstance(entry, dict):
            version = str(entry.get("version", "")).strip()
            if version:
                return version
    raise SystemExit(
        "idf_component.yml is missing dependencies.espressif/esp_board_manager.version"
    )


def _floor_version(spec: str) -> str | None:
    spec = spec.strip()
    if spec in {"*", "==*", ">=*"}:
        return None
    match = re.search(r"(\d+\.\d+\.\d+(?:~\d+)?)", spec)
    if match:
        return match.group(1)
    match = re.search(r"(\d+\.\d+)", spec)
    if match:
        return f"{match.group(1)}.0"
    match = re.search(r"(\d+)", spec)
    if match:
        return f"{match.group(1)}.0.0"
    return None


def _latest_registry_version() -> str | None:
    try:
        with urllib.request.urlopen(REGISTRY_VERSIONS_URL, timeout=20) as resp:
            payload = json.load(resp)
    except (urllib.error.URLError, TimeoutError, json.JSONDecodeError, ValueError):
        return None
    versions = payload.get("versions") or payload.get("items") or []
    names = []
    for item in versions:
        if isinstance(item, str):
            names.append(item)
        elif isinstance(item, dict):
            ver = item.get("version") or item.get("name")
            if ver:
                names.append(str(ver))
    if not names:
        return None
    try:
        from packaging.version import Version

        def _key(value: str):
            try:
                return Version(value.replace("~", "."))
            except Exception:
                return Version("0")

        names.sort(key=_key)
        return names[-1]
    except Exception:
        names.sort()
        return names[-1]


def _discover_boards() -> list[dict[str, str]]:
    boards = []
    for info_path in sorted(REPO_ROOT.rglob("board_info.yaml")):
        board_dir = info_path.parent.relative_to(REPO_ROOT)
        if (
            len(board_dir.parts) > MAX_BOARD_DIRECTORY_DEPTH
            or any(part.startswith(".") for part in board_dir.parts)
        ):
            continue
        with info_path.open(encoding="utf-8") as fh:
            info = yaml.safe_load(fh) or {}
        board = str(info.get("board") or info_path.parent.name)
        chip = str(info.get("chip") or "").replace("-", "").lower()
        if not chip:
            raise SystemExit(f"{info_path} is missing chip")
        if board != info_path.parent.name:
            raise SystemExit(
                f"board name {board!r} must match directory {info_path.parent.name!r}"
            )
        boards.append({"board": board, "target": chip})
    if not boards:
        raise SystemExit("no board_info.yaml found; add at least one board directory")
    return boards


def _bmgr_cells(spec: str) -> list[str]:
    floor = _floor_version(spec)
    latest = "*"
    cells = []
    if floor:
        cells.append(floor)
    registry_latest = _latest_registry_version()
    if registry_latest and floor and registry_latest == floor:
        return cells
    if latest not in cells:
        cells.append(latest)
    return cells


def main() -> int:
    manifest = _load_manifest()
    spec = _bmgr_spec(manifest)
    include = []
    for board in _discover_boards():
        for idf in IDF_VERSIONS:
            for bmgr in _bmgr_cells(spec):
                include.append(
                    {
                        "board": board["board"],
                        "target": board["target"],
                        "idf": idf,
                        "bmgr": bmgr,
                        "component_manager": IDF_COMPONENT_MANAGER_SPECS[idf],
                    }
                )
    matrix = {"include": include}
    encoded = json.dumps(matrix, separators=(",", ":"))
    output_path = os.environ.get("GITHUB_OUTPUT")
    if output_path:
        with open(output_path, "a", encoding="utf-8") as fh:
            fh.write("matrix<<MATRIX_EOF\n")
            fh.write(encoded + "\n")
            fh.write("MATRIX_EOF\n")
    else:
        print(encoded)
    return 0


if __name__ == "__main__":
    sys.exit(main())
