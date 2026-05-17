from __future__ import annotations

from contextlib import contextmanager, suppress
from typing import Iterator

import anyio
from fastapi import FastAPI
from fastapi.testclient import TestClient


@contextmanager
def managed_test_client(app: FastAPI) -> Iterator[TestClient]:
    """Ensure TestClient transport streams are explicitly closed on teardown."""
    client = TestClient(app)
    try:
        with client as active_client:
            yield active_client
    finally:
        with suppress(Exception):
            client.close()
        for attr in ("stream_receive", "stream_send"):
            stream = getattr(client, attr, None)
            if stream is None:
                continue
            with suppress(Exception):
                anyio.run(stream.aclose)

