from __future__ import annotations

import sys
from pathlib import Path

from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles


WORKSPACE_ROOT = Path(__file__).resolve().parent
PULSEBOOST_ROOT = WORKSPACE_ROOT / "pulseboost"
if str(PULSEBOOST_ROOT) not in sys.path:
    sys.path.insert(0, str(PULSEBOOST_ROOT))

from api.main import app  # noqa: E402


frontend_dist = PULSEBOOST_ROOT / "ui" / "dist"
frontend_assets = frontend_dist / "assets"

if frontend_assets.exists():
    app.mount("/assets", StaticFiles(directory=frontend_assets), name="frontend-assets")


if frontend_dist.exists():

    @app.get("/")
    async def frontend_index() -> FileResponse:
        return FileResponse(frontend_dist / "index.html")


    @app.get("/{full_path:path}")
    async def frontend_spa(full_path: str) -> FileResponse:
        candidate = frontend_dist / full_path
        if full_path and candidate.exists() and candidate.is_file():
            return FileResponse(candidate)
        return FileResponse(frontend_dist / "index.html")
