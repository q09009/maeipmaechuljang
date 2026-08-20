from datetime import date
from decimal import Decimal
from typing import Literal

from pydantic import BaseModel, ConfigDict, Field


class CustomerCreate(BaseModel):
    name: str = Field(min_length=1, max_length=200)


class CustomerUpdate(CustomerCreate):
    pass


class CustomerOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    name: str
    balance: int


class ItemCreate(BaseModel):
    item_name: str = Field(min_length=1, max_length=300)
    spec: str = Field(default="", max_length=300)
    price: Decimal = Field(default=0)


class ItemUpdate(ItemCreate):
    pass


class ItemOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    item_name: str
    spec: str
    price: Decimal


class RecordCreate(BaseModel):
    gubun: Literal["매입", "매출"]
    tr_date: date
    customer: str = Field(min_length=1, max_length=200)
    item: str = Field(min_length=1, max_length=300)
    spec: str = Field(default="", max_length=300)
    price: int
    amount: int
    tax: bool = False


class RecordImportItem(BaseModel):
    gubun: Literal["매입", "매출"]
    tr_date: date
    customer: str
    item: str
    spec: str = ""
    price: int
    amount: int
    supply_val: int
    tax_val: int
    total_val: int
    pay_date1: date | None = None
    pay_amt1: int = 0
    pay_date2: date | None = None
    pay_amt2: int = 0
    pay_date3: date | None = None
    pay_amt3: int = 0


class RecordImportRequest(BaseModel):
    records: list[RecordImportItem]


class ReferenceDataImportRequest(BaseModel):
    customers: list[str]
    items: list[ItemCreate]


class RecordOut(BaseModel):
    model_config = ConfigDict(from_attributes=True)

    id: int
    gubun: str
    tr_date: date
    customer: str
    item: str
    spec: str
    price: int
    amount: int
    supply_val: int
    tax_val: int
    total_val: int
    pay_date1: date | None
    pay_amt1: int
    pay_date2: date | None
    pay_amt2: int
    pay_date3: date | None
    pay_amt3: int
    unpaid_amt: int
    receivable_amt: int


class RecordSearchResponse(BaseModel):
    records: list[RecordOut]
    amount_sum: int
    supply_sum: int
    tax_sum: int
    total_sum: int
    ip_sum: int
    misu_sum: int
    miji_sum: int


class PaymentUpdate(BaseModel):
    tr_date: date | None = None
    pay_date1: date | None = None
    pay_amt1: int = 0
    pay_date2: date | None = None
    pay_amt2: int = 0
    pay_date3: date | None = None
    pay_amt3: int = 0


class BulkPaymentRequest(BaseModel):
    date: date
    amount: int = Field(gt=0)
    record_ids: list[int] = Field(min_length=1)


class ImportResult(BaseModel):
    imported: int
