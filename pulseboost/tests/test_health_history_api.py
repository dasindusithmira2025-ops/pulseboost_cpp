import asyncio
import tempfile
import time
import unittest
from pathlib import Path

from fastapi import FastAPI
from tests.test_client_utils import managed_test_client

from api.routes import router
from core.database import DatabaseService


class HealthHistoryApiTests(unittest.TestCase):
    def test_health_history_endpoint_returns_snapshots(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            database = DatabaseService(Path(temp_dir) / "health.db")
            asyncio.run(database.initialize())
            now = time.time()
            asyncio.run(
                database.insert_health_history(
                    score=78.5,
                    cpu_load=33.0,
                    ram_percent=61.0,
                    gpu_temp=72.0,
                    timestamp=now - 600,
                )
            )
            asyncio.run(
                database.insert_health_history(
                    score=83.2,
                    cpu_load=21.0,
                    ram_percent=54.0,
                    gpu_temp=68.0,
                    timestamp=now - 120,
                )
            )

            app = FastAPI()
            app.include_router(router)
            app.state.foundation = {"database": database}

            with managed_test_client(app) as client:
                response = client.get("/api/health/history?days=7")
                self.assertEqual(response.status_code, 200)
                body = response.json()
                self.assertEqual(len(body), 2)
                self.assertIn("score", body[0])
                self.assertIn("timestamp", body[0])


if __name__ == "__main__":
    unittest.main()


