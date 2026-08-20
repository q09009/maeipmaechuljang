from datetime import date, datetime
from decimal import Decimal

from sqlalchemy import (
    BigInteger,
    CheckConstraint,
    Computed,
    Date,
    DateTime,
    Identity,
    Index,
    Integer,
    Numeric,
    String,
    Text,
    UniqueConstraint,
    func,
    text,
)
from sqlalchemy.orm import Mapped, mapped_column

from .database import Base


class Customer(Base):
    __tablename__ = "customer"

    id: Mapped[int] = mapped_column(Integer, Identity(), primary_key=True)
    name: Mapped[str] = mapped_column(Text, nullable=False, unique=True)
    balance: Mapped[int] = mapped_column(BigInteger, nullable=False, default=0, server_default=text("0"))
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False, server_default=func.now()
    )
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False, server_default=func.now(), onupdate=func.now()
    )


class Item(Base):
    __tablename__ = "item"
    __table_args__ = (UniqueConstraint("item_name", "spec", name="uq_item_name_spec"),)

    id: Mapped[int] = mapped_column(Integer, Identity(), primary_key=True)
    item_name: Mapped[str] = mapped_column(Text, nullable=False)
    spec: Mapped[str] = mapped_column(Text, nullable=False, default="", server_default=text("''"))
    price: Mapped[Decimal] = mapped_column(Numeric(18, 2), nullable=False, default=0)
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False, server_default=func.now()
    )
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False, server_default=func.now(), onupdate=func.now()
    )


class Record(Base):
    __tablename__ = "records"
    __table_args__ = (
        CheckConstraint("gubun IN ('매입', '매출')", name="ck_records_gubun"),
        Index("ix_records_date_gubun", "tr_date", "gubun"),
        Index("ix_records_customer", "customer"),
        Index("ix_records_item", "item"),
    )

    id: Mapped[int] = mapped_column(Integer, Identity(), primary_key=True)
    gubun: Mapped[str] = mapped_column(String(2), nullable=False)
    tr_date: Mapped[date] = mapped_column(Date, nullable=False)
    customer: Mapped[str] = mapped_column(Text, nullable=False)
    item: Mapped[str] = mapped_column(Text, nullable=False)
    spec: Mapped[str] = mapped_column(Text, nullable=False, default="", server_default=text("''"))
    price: Mapped[int] = mapped_column(BigInteger, nullable=False)
    amount: Mapped[int] = mapped_column(BigInteger, nullable=False)
    supply_val: Mapped[int] = mapped_column(BigInteger, nullable=False)
    tax_val: Mapped[int] = mapped_column(BigInteger, nullable=False, default=0, server_default=text("0"))
    total_val: Mapped[int] = mapped_column(BigInteger, nullable=False)

    pay_date1: Mapped[date | None] = mapped_column(Date)
    pay_amt1: Mapped[int] = mapped_column(BigInteger, nullable=False, default=0, server_default=text("0"))
    pay_date2: Mapped[date | None] = mapped_column(Date)
    pay_amt2: Mapped[int] = mapped_column(BigInteger, nullable=False, default=0, server_default=text("0"))
    pay_date3: Mapped[date | None] = mapped_column(Date)
    pay_amt3: Mapped[int] = mapped_column(BigInteger, nullable=False, default=0, server_default=text("0"))

    unpaid_amt: Mapped[int] = mapped_column(
        BigInteger,
        Computed(
            "CASE WHEN gubun = '매입' THEN total_val - COALESCE(pay_amt1, 0) "
            "- COALESCE(pay_amt2, 0) - COALESCE(pay_amt3, 0) ELSE 0 END",
            persisted=True,
        ),
    )
    receivable_amt: Mapped[int] = mapped_column(
        BigInteger,
        Computed(
            "CASE WHEN gubun = '매출' THEN total_val - COALESCE(pay_amt1, 0) "
            "- COALESCE(pay_amt2, 0) - COALESCE(pay_amt3, 0) ELSE 0 END",
            persisted=True,
        ),
    )
    created_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False, server_default=func.now()
    )
    updated_at: Mapped[datetime] = mapped_column(
        DateTime(timezone=True), nullable=False, server_default=func.now(), onupdate=func.now()
    )
