import hashlib
import json
import os
from datetime import datetime, timezone
from decimal import Decimal
from pathlib import Path
from typing import Annotated, Any
from uuid import uuid4

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy import delete, select, text
from sqlalchemy.exc import IntegrityError, SQLAlchemyError
from sqlalchemy.orm import Session

from ..config import get_settings
from ..database import get_db
from ..models import Customer, Item, Record
from ..schemas import (
    ItemCreate,
    RecordImportItem,
    TransferCustomer,
    TransferResult,
    TransferSnapshot,
)


router = APIRouter(prefix="/transfers", tags=["transfers"])


def _lock_tables(db: Session, *, exclusive: bool) -> None:
    bind = db.get_bind()
    if bind.dialect.name != "postgresql":
        return
    mode = "ACCESS EXCLUSIVE" if exclusive else "SHARE"
    db.execute(text(f"LOCK TABLE records, customer, item IN {mode} MODE"))


def _build_snapshot(db: Session) -> TransferSnapshot:
    customers = list(db.scalars(select(Customer).order_by(Customer.name, Customer.id)))
    items = list(db.scalars(select(Item).order_by(Item.item_name, Item.spec, Item.id)))
    records = list(db.scalars(select(Record).order_by(Record.tr_date, Record.id)))
    return TransferSnapshot(
        customers=[
            TransferCustomer(name=customer.name, balance=customer.balance)
            for customer in customers
        ],
        items=[
            ItemCreate(item_name=item.item_name, spec=item.spec, price=item.price)
            for item in items
        ],
        records=[
            RecordImportItem(
                gubun=record.gubun,
                tr_date=record.tr_date,
                customer=record.customer,
                item=record.item,
                spec=record.spec,
                price=record.price,
                amount=record.amount,
                supply_val=record.supply_val,
                tax_val=record.tax_val,
                total_val=record.total_val,
                pay_date1=record.pay_date1,
                pay_amt1=record.pay_amt1,
                pay_date2=record.pay_date2,
                pay_amt2=record.pay_amt2,
                pay_date3=record.pay_date3,
                pay_amt3=record.pay_amt3,
            )
            for record in records
        ],
    )


def _decimal_text(value: Decimal) -> str:
    result = format(value, "f")
    if "." in result:
        result = result.rstrip("0").rstrip(".")
    return result or "0"


def _canonical_payload(snapshot: TransferSnapshot) -> dict[str, Any]:
    customers = sorted(
        ({"name": row.name, "balance": row.balance} for row in snapshot.customers),
        key=lambda row: (row["name"], row["balance"]),
    )
    items = sorted(
        (
            {
                "item_name": row.item_name,
                "spec": row.spec,
                "price": _decimal_text(row.price),
            }
            for row in snapshot.items
        ),
        key=lambda row: (row["item_name"], row["spec"], row["price"]),
    )
    records = sorted(
        (row.model_dump(mode="json") for row in snapshot.records),
        key=lambda row: json.dumps(
            row, ensure_ascii=False, sort_keys=True, separators=(",", ":")
        ),
    )
    return {"version": 1, "customers": customers, "items": items, "records": records}


def _snapshot_digest(snapshot: TransferSnapshot) -> str:
    canonical = json.dumps(
        _canonical_payload(snapshot),
        ensure_ascii=False,
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return hashlib.sha256(canonical).hexdigest()


def _write_backup(snapshot: TransferSnapshot) -> str:
    backup_dir = get_settings().backup_dir
    backup_dir.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    filename = f"postgres_{timestamp}_{uuid4().hex[:8]}.json"
    destination = backup_dir / filename
    temporary = destination.with_suffix(".tmp")
    payload = {
        "format": "maeipmaechuljang-transfer-backup-v1",
        "created_at": datetime.now(timezone.utc).isoformat(),
        "digest": _snapshot_digest(snapshot),
        "snapshot": snapshot.model_dump(mode="json"),
    }
    try:
        temporary.write_text(
            json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        os.replace(temporary, destination)
    finally:
        temporary.unlink(missing_ok=True)
    return filename


def _replace_snapshot(db: Session, snapshot: TransferSnapshot) -> TransferSnapshot:
    expected_digest = _snapshot_digest(snapshot)
    db.execute(delete(Record))
    db.execute(delete(Customer))
    db.execute(delete(Item))
    db.add_all(
        Customer(name=row.name, balance=row.balance) for row in snapshot.customers
    )
    db.add_all(
        Item(item_name=row.item_name, spec=row.spec, price=row.price)
        for row in snapshot.items
    )
    db.add_all(
        Record(
            gubun=row.gubun,
            tr_date=row.tr_date,
            customer=row.customer,
            item=row.item,
            spec=row.spec,
            price=row.price,
            amount=row.amount,
            supply_val=row.supply_val,
            tax_val=row.tax_val,
            total_val=row.total_val,
            pay_date1=row.pay_date1,
            pay_amt1=row.pay_amt1,
            pay_date2=row.pay_date2,
            pay_amt2=row.pay_amt2,
            pay_date3=row.pay_date3,
            pay_amt3=row.pay_amt3,
        )
        for row in snapshot.records
    )
    db.flush()
    actual = _build_snapshot(db)
    if _snapshot_digest(actual) != expected_digest:
        raise ValueError("Transferred PostgreSQL data did not match the source snapshot")
    return actual


def _result(snapshot: TransferSnapshot, backup_file: str) -> TransferResult:
    return TransferResult(
        backup_file=backup_file,
        digest=_snapshot_digest(snapshot),
        customers=len(snapshot.customers),
        items=len(snapshot.items),
        records=len(snapshot.records),
    )


@router.get("/snapshot", response_model=TransferSnapshot)
def read_snapshot(db: Annotated[Session, Depends(get_db)]) -> TransferSnapshot:
    _lock_tables(db, exclusive=False)
    return _build_snapshot(db)


@router.post("/backups", response_model=TransferResult)
def create_backup(db: Annotated[Session, Depends(get_db)]) -> TransferResult:
    _lock_tables(db, exclusive=False)
    snapshot = _build_snapshot(db)
    try:
        backup_file = _write_backup(snapshot)
    except OSError as exc:
        raise HTTPException(status_code=500, detail=f"Backup creation failed: {exc}") from exc
    return _result(snapshot, backup_file)


@router.post("/replace", response_model=TransferResult)
def replace_snapshot(
    payload: TransferSnapshot,
    db: Annotated[Session, Depends(get_db)],
) -> TransferResult:
    _lock_tables(db, exclusive=True)
    previous = _build_snapshot(db)
    try:
        backup_file = _write_backup(previous)
    except OSError as exc:
        raise HTTPException(status_code=500, detail=f"Backup creation failed: {exc}") from exc
    try:
        actual = _replace_snapshot(db, payload)
        db.commit()
    except (IntegrityError, SQLAlchemyError, ValueError) as exc:
        db.rollback()
        raise HTTPException(
            status_code=409,
            detail=f"Database replacement failed (backup_file={backup_file}): {exc}",
        ) from exc
    return _result(actual, backup_file)


@router.post("/restore/{backup_file}", response_model=TransferResult)
def restore_backup(
    backup_file: str,
    db: Annotated[Session, Depends(get_db)],
) -> TransferResult:
    if Path(backup_file).name != backup_file or not backup_file.endswith(".json"):
        raise HTTPException(status_code=400, detail="Invalid backup file name")
    source = get_settings().backup_dir / backup_file
    try:
        document = json.loads(source.read_text(encoding="utf-8"))
        snapshot = TransferSnapshot.model_validate(document["snapshot"])
    except (OSError, KeyError, json.JSONDecodeError, ValueError) as exc:
        raise HTTPException(status_code=400, detail=f"Invalid backup file: {exc}") from exc
    if document.get("digest") != _snapshot_digest(snapshot):
        raise HTTPException(status_code=400, detail="Backup digest verification failed")

    _lock_tables(db, exclusive=True)
    previous = _build_snapshot(db)
    try:
        safety_backup = _write_backup(previous)
        actual = _replace_snapshot(db, snapshot)
        db.commit()
    except OSError as exc:
        db.rollback()
        raise HTTPException(status_code=500, detail=f"Safety backup failed: {exc}") from exc
    except (IntegrityError, SQLAlchemyError, ValueError) as exc:
        db.rollback()
        raise HTTPException(status_code=409, detail=f"Database restore failed: {exc}") from exc
    return _result(actual, safety_backup)
