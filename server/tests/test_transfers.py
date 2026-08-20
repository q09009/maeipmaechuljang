import tempfile
import unittest
from datetime import date
from decimal import Decimal
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

from fastapi import HTTPException
from sqlalchemy import create_engine
from sqlalchemy.orm import Session, sessionmaker
from sqlalchemy.pool import StaticPool

from app.database import Base
from app.models import Customer, Item, Record
from app.routers import transfers
from app.schemas import (
    ItemCreate,
    RecordImportItem,
    TransferCustomer,
    TransferSnapshot,
)


class TransferTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.engine = create_engine(
            "sqlite://",
            connect_args={"check_same_thread": False},
            poolclass=StaticPool,
        )
        Base.metadata.create_all(self.engine)
        self.session_factory = sessionmaker(bind=self.engine, expire_on_commit=False)

        self.settings_patch = patch.object(
            transfers,
            "get_settings",
            return_value=SimpleNamespace(backup_dir=Path(self.temp_dir.name)),
        )
        self.settings_patch.start()

    def tearDown(self) -> None:
        self.settings_patch.stop()
        self.engine.dispose()
        self.temp_dir.cleanup()

    @staticmethod
    def sample_snapshot(customer_name: str = "거래처") -> TransferSnapshot:
        return TransferSnapshot(
            customers=[TransferCustomer(name=customer_name, balance=1200)],
            items=[ItemCreate(item_name="품목", spec="EA", price=Decimal("1000.00"))],
            records=[
                RecordImportItem(
                    gubun="매출",
                    tr_date=date(2026, 8, 21),
                    customer=customer_name,
                    item="품목",
                    spec="EA",
                    price=1000,
                    amount=2,
                    supply_val=2000,
                    tax_val=200,
                    total_val=2200,
                    pay_date1=date(2026, 8, 22),
                    pay_amt1=1000,
                )
            ],
        )

    def seed_database(self) -> None:
        db: Session = self.session_factory()
        db.add(Customer(name="기존거래처", balance=300))
        db.add(Item(item_name="기존품목", spec="BOX", price=Decimal("500.00")))
        db.add(
            Record(
                gubun="매입",
                tr_date=date(2026, 8, 20),
                customer="기존거래처",
                item="기존품목",
                spec="BOX",
                price=500,
                amount=1,
                supply_val=500,
                tax_val=50,
                total_val=550,
            )
        )
        db.commit()
        db.close()

    def test_digest_ignores_order_and_decimal_format(self) -> None:
        first = self.sample_snapshot()
        second = self.sample_snapshot()
        second.items[0].price = Decimal("1000")
        self.assertEqual(transfers._snapshot_digest(first), transfers._snapshot_digest(second))

    def test_replace_creates_backup_verifies_and_can_restore(self) -> None:
        self.seed_database()
        db: Session = self.session_factory()

        backup_result = transfers.create_backup(db)
        original_backup = backup_result.backup_file
        self.assertTrue((Path(self.temp_dir.name) / original_backup).exists())

        replacement = self.sample_snapshot("새거래처")
        result = transfers.replace_snapshot(replacement, db)
        self.assertEqual((result.customers, result.items, result.records), (1, 1, 1))
        self.assertTrue((Path(self.temp_dir.name) / result.backup_file).exists())

        snapshot = transfers.read_snapshot(db)
        self.assertEqual(snapshot.customers[0].name, "새거래처")

        transfers.restore_backup(original_backup, db)
        restored = transfers.read_snapshot(db)
        self.assertEqual(restored.customers[0].name, "기존거래처")
        self.assertEqual(restored.records[0].pay_amt1, 0)
        db.close()

    def test_failed_replace_rolls_back_and_keeps_backup(self) -> None:
        self.seed_database()
        db: Session = self.session_factory()
        duplicate = self.sample_snapshot("중복거래처")
        duplicate.customers.append(TransferCustomer(name="중복거래처", balance=0))

        with self.assertRaises(HTTPException) as context:
            transfers.replace_snapshot(duplicate, db)
        self.assertEqual(context.exception.status_code, 409)
        self.assertIn("backup_file=", str(context.exception.detail))
        self.assertEqual(len(list(Path(self.temp_dir.name).glob("postgres_*.json"))), 1)

        unchanged = transfers.read_snapshot(db)
        self.assertEqual(unchanged.customers[0].name, "기존거래처")
        self.assertEqual(unchanged.records[0].total_val, 550)
        db.close()


if __name__ == "__main__":
    unittest.main()
