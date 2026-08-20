from typing import Annotated

from fastapi import APIRouter, Depends
from sqlalchemy import text
from sqlalchemy.orm import Session

from ..database import get_db
from ..models import Customer, Item, Record
from ..schemas import (
    ImportResult,
    RecordImportRequest,
    ReferenceDataImportRequest,
)


router = APIRouter(prefix="/imports", tags=["imports"])


@router.post("/records", response_model=ImportResult)
def replace_records(
    payload: RecordImportRequest,
    db: Annotated[Session, Depends(get_db)],
) -> ImportResult:
    db.execute(text("TRUNCATE TABLE records RESTART IDENTITY"))
    db.add_all(
        [
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
            for row in payload.records
        ]
    )
    db.commit()
    return ImportResult(imported=len(payload.records))


@router.post("/reference-data")
def replace_reference_data(
    payload: ReferenceDataImportRequest,
    db: Annotated[Session, Depends(get_db)],
) -> dict[str, int]:
    db.execute(text("TRUNCATE TABLE customer, item RESTART IDENTITY"))
    db.add_all(Customer(name=name.strip()) for name in payload.customers if name.strip())
    db.add_all(
        Item(item_name=item.item_name.strip(), spec=item.spec.strip(), price=item.price)
        for item in payload.items
    )
    db.commit()
    return {"customers": len(payload.customers), "items": len(payload.items)}
