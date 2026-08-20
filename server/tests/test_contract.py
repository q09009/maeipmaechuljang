import os
import unittest
from datetime import date


os.environ.setdefault(
    "DATABASE_URL", "postgresql+psycopg://test:test@127.0.0.1:5432/test"
)
os.environ.setdefault("MAEIP_API_KEY", "test-key")

from fastapi import HTTPException  # noqa: E402
from pydantic import ValidationError  # noqa: E402

from app.main import app  # noqa: E402
from app.schemas import RecordCreate  # noqa: E402
from app.security import require_api_key  # noqa: E402


class ApiContractTests(unittest.TestCase):
    def test_expected_routes_exist(self) -> None:
        paths = set(app.openapi()["paths"])
        self.assertTrue(
            {
                "/health",
                "/customers",
                "/items",
                "/records",
                "/records/search",
                "/records/monthly",
                "/records/{record_id}/payment",
                "/records/bulk-payment",
                "/transfers/snapshot",
                "/transfers/backups",
                "/transfers/replace",
                "/transfers/restore/{backup_file}",
            }.issubset(paths)
        )

    def test_api_key_is_required(self) -> None:
        require_api_key("test-key")
        with self.assertRaises(HTTPException) as context:
            require_api_key(None)
        self.assertEqual(context.exception.status_code, 401)

    def test_record_type_is_validated(self) -> None:
        with self.assertRaises(ValidationError):
            RecordCreate(
                gubun="기타",
                tr_date=date(2026, 8, 20),
                customer="거래처",
                item="품목",
                price=1000,
                amount=1,
            )


if __name__ == "__main__":
    unittest.main()
