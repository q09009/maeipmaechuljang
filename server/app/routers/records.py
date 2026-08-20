from datetime import date
from typing import Annotated, Any

from fastapi import APIRouter, Depends, HTTPException, Response, status
from sqlalchemy import case, extract, func, select
from sqlalchemy.orm import Session

from ..database import get_db
from ..models import Customer, Record
from ..schemas import (
    BulkPaymentRequest,
    PaymentUpdate,
    RecordCreate,
    RecordOut,
    RecordSearchResponse,
)


router = APIRouter(prefix="/records", tags=["records"])


def _record_filters(
    *,
    start: date | None = None,
    end: date | None = None,
    year: int | None = None,
    month: int | None = None,
    gubun: str | None = None,
    customer: str | None = None,
    item: str | None = None,
) -> list[Any]:
    filters: list[Any] = []
    if start is not None:
        filters.append(Record.tr_date >= start)
    if end is not None:
        filters.append(Record.tr_date <= end)
    if year is not None:
        filters.append(extract("year", Record.tr_date) == year)
    if month is not None:
        filters.append(extract("month", Record.tr_date) == month)
    if gubun:
        filters.append(Record.gubun == gubun)
    if customer and customer != "전체":
        filters.append(Record.customer == customer)
    if item and item != "전체":
        filters.append(Record.item == item)
    return filters


def _sum_values(db: Session, filters: list[Any]) -> dict[str, int]:
    row = db.execute(
        select(
            func.coalesce(func.sum(Record.amount), 0),
            func.coalesce(func.sum(Record.supply_val), 0),
            func.coalesce(func.sum(Record.tax_val), 0),
            func.coalesce(func.sum(Record.total_val), 0),
            func.coalesce(func.sum(Record.unpaid_amt), 0),
            func.coalesce(func.sum(Record.receivable_amt), 0),
        ).where(*filters)
    ).one()
    amount_sum, supply_sum, tax_sum, total_sum, miji_sum, misu_sum = map(int, row)
    return {
        "amount_sum": amount_sum,
        "supply_sum": supply_sum,
        "tax_sum": tax_sum,
        "total_sum": total_sum,
        "ip_sum": total_sum - miji_sum - misu_sum,
        "misu_sum": misu_sum,
        "miji_sum": miji_sum,
    }


def _aggregate_rows(
    db: Session,
    period_expression: Any,
    period_key: str,
    filters: list[Any],
) -> list[dict[str, int]]:
    rows = db.execute(
        select(
            period_expression.label("period"),
            func.coalesce(func.sum(Record.amount), 0),
            func.coalesce(func.sum(Record.supply_val), 0),
            func.coalesce(func.sum(Record.tax_val), 0),
            func.coalesce(func.sum(Record.total_val), 0),
            func.coalesce(func.sum(Record.unpaid_amt), 0),
            func.coalesce(func.sum(Record.receivable_amt), 0),
        )
        .where(*filters)
        .group_by(period_expression)
        .order_by(period_expression)
    ).all()
    return [
        {
            period_key: int(row[0]),
            "amount": int(row[1]),
            "gongga": int(row[2]),
            "buga": int(row[3]),
            "hapgye": int(row[4]),
            "miji": int(row[5]),
            "misu": int(row[6]),
        }
        for row in rows
    ]


@router.get("", response_model=list[RecordOut])
def list_records(db: Annotated[Session, Depends(get_db)]) -> list[Record]:
    return list(db.scalars(select(Record).order_by(Record.tr_date, Record.id)))


@router.post("", response_model=RecordOut, status_code=status.HTTP_201_CREATED)
def create_record(payload: RecordCreate, db: Annotated[Session, Depends(get_db)]) -> Record:
    supply_val = payload.price * payload.amount
    tax_val = supply_val // 10 if payload.tax else 0
    record = Record(
        gubun=payload.gubun,
        tr_date=payload.tr_date,
        customer=payload.customer.strip(),
        item=payload.item.strip(),
        spec=payload.spec.strip(),
        price=payload.price,
        amount=payload.amount,
        supply_val=supply_val,
        tax_val=tax_val,
        total_val=supply_val + tax_val,
    )
    db.add(record)
    db.commit()
    db.refresh(record)
    return record


@router.get("/search", response_model=RecordSearchResponse)
def search_records(
    start: date,
    end: date,
    db: Annotated[Session, Depends(get_db)],
    gubun: str | None = None,
    customer: str | None = None,
    item: str | None = None,
) -> dict[str, Any]:
    filters = _record_filters(
        start=start,
        end=end,
        gubun=gubun,
        customer=customer,
        item=item,
    )
    records = list(db.scalars(select(Record).where(*filters).order_by(Record.tr_date, Record.id)))
    return {"records": records, **_sum_values(db, filters)}


@router.get("/monthly")
def monthly_statistics(
    year: int,
    gubun: str,
    db: Annotated[Session, Depends(get_db)],
    customer: str | None = None,
    item: str | None = None,
) -> dict[str, list[dict[str, int]]]:
    filters = _record_filters(year=year, gubun=gubun, customer=customer, item=item)
    month_expression = extract("month", Record.tr_date)
    quarter_expression = extract("quarter", Record.tr_date)
    half_expression = case((extract("month", Record.tr_date) <= 6, 1), else_=2)
    return {
        "monthly": _aggregate_rows(db, month_expression, "month", filters),
        "quarterly": _aggregate_rows(db, quarter_expression, "num", filters),
        "halfyear": _aggregate_rows(db, half_expression, "num", filters),
    }


@router.get("/monthly-detail", response_model=list[RecordOut])
def monthly_detail(
    year: int,
    month: int,
    db: Annotated[Session, Depends(get_db)],
) -> list[Record]:
    filters = _record_filters(year=year, month=month)
    return list(db.scalars(select(Record).where(*filters).order_by(Record.tr_date, Record.id)))


@router.put("/{record_id}/payment", response_model=RecordOut)
def update_payment(
    record_id: int,
    payload: PaymentUpdate,
    db: Annotated[Session, Depends(get_db)],
) -> Record:
    record = db.get(Record, record_id)
    if record is None:
        raise HTTPException(status_code=404, detail="Record not found")
    if payload.tr_date is not None:
        record.tr_date = payload.tr_date
    record.pay_date1 = payload.pay_date1
    record.pay_amt1 = payload.pay_amt1
    record.pay_date2 = payload.pay_date2
    record.pay_amt2 = payload.pay_amt2
    record.pay_date3 = payload.pay_date3
    record.pay_amt3 = payload.pay_amt3
    db.commit()
    db.refresh(record)
    return record


@router.post("/bulk-payment")
def bulk_payment(
    payload: BulkPaymentRequest,
    db: Annotated[Session, Depends(get_db)],
) -> dict[str, int]:
    unique_ids = list(dict.fromkeys(payload.record_ids))
    records = list(
        db.scalars(
            select(Record)
            .where(Record.id.in_(unique_ids))
            .order_by(Record.tr_date, Record.id)
            .with_for_update()
        )
    )
    if len(records) != len(unique_ids):
        raise HTTPException(status_code=404, detail="One or more records were not found")

    customer_name = records[0].customer
    gubun = records[0].gubun
    if any(record.customer != customer_name or record.gubun != gubun for record in records):
        raise HTTPException(status_code=400, detail="Bulk payment records must share customer and type")

    customer = db.scalar(
        select(Customer).where(Customer.name == customer_name).with_for_update()
    )
    if customer is None:
        raise HTTPException(status_code=404, detail="Customer not found")

    remaining = payload.amount + customer.balance
    customer.balance = 0
    applied = 0

    for record in records:
        outstanding = record.unpaid_amt if gubun == "매입" else record.receivable_amt
        if outstanding <= 0 or remaining <= 0:
            continue
        payment = min(outstanding, remaining)
        remaining -= payment
        applied += payment

        if record.pay_amt1 == 0:
            record.pay_date1 = payload.date
            record.pay_amt1 = payment
        elif record.pay_amt2 == 0:
            record.pay_date2 = payload.date
            record.pay_amt2 = payment
        else:
            record.pay_date3 = payload.date
            record.pay_amt3 = (record.pay_amt3 or 0) + payment

    customer.balance = remaining if gubun == "매입" else -remaining
    db.commit()
    return {"applied": applied, "balance": customer.balance}


@router.delete("/{record_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_record(record_id: int, db: Annotated[Session, Depends(get_db)]) -> Response:
    record = db.get(Record, record_id)
    if record is None:
        raise HTTPException(status_code=404, detail="Record not found")
    db.delete(record)
    db.commit()
    return Response(status_code=status.HTTP_204_NO_CONTENT)
