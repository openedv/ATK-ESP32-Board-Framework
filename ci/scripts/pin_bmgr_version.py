#!/usr/bin/env python3
"""Pin espressif/esp_board_manager in the CI test app to a matrix version."""

from __future__ import annotations

import re
import sys
from pathlib import Path

MANIFEST = Path(__file__).resolve().parents[1] / "test_app" / "main" / "idf_component.yml"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: pin_bmgr_version.py <version-or-star>", file=sys.stderr)
        return 2
    requested = sys.argv[1].strip()
    version = '"*"' if requested == "*" else f'"=={requested}"'
    text = MANIFEST.read_text(encoding="utf-8")
    updated, count = re.subn(
        r"(espressif/esp_board_manager:[\s\S]*?version:\s*)([^\n]+)",
        rf"\1{version}",
        text,
        count=1,
    )
    if count != 1:
        print(f"failed to pin BMGR version in {MANIFEST}", file=sys.stderr)
        return 1
    MANIFEST.write_text(updated, encoding="utf-8")
    print(f"pinned espressif/esp_board_manager to {version}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
