"""Create initial ledger tables.

Revision ID: 20260820_0001
Revises: None
"""
from collections.abc import Sequence

from alembic import op
import sqlalchemy as sa


revision: str = "20260820_0001"
down_revision: str | None = None
branch_labels: str | Sequence[str] | None = None
depends_on: str | Sequence[str] | None = None


def upgrade() -> None:
    op.create_table(
        "customer",
        sa.Column("id", sa.Integer(), sa.Identity(), primary_key=True),
        sa.Column("name", sa.Text(), nullable=False, unique=True),
        sa.Column("balance", sa.BigInteger(), server_default=sa.text("0"), nullable=False),
        sa.Column("created_at", sa.DateTime(timezone=True), server_default=sa.func.now(), nullable=False),
        sa.Column("updated_at", sa.DateTime(timezone=True), server_default=sa.func.now(), nullable=False),
    )
    op.create_table(
        "item",
        sa.Column("id", sa.Integer(), sa.Identity(), primary_key=True),
        sa.Column("item_name", sa.Text(), nullable=False),
        sa.Column("spec", sa.Text(), server_default=sa.text("''"), nullable=False),
        sa.Column("price", sa.Numeric(18, 2), nullable=False),
        sa.Column("created_at", sa.DateTime(timezone=True), server_default=sa.func.now(), nullable=False),
        sa.Column("updated_at", sa.DateTime(timezone=True), server_default=sa.func.now(), nullable=False),
        sa.UniqueConstraint("item_name", "spec", name="uq_item_name_spec"),
    )
    op.create_table(
        "records",
        sa.Column("id", sa.Integer(), sa.Identity(), primary_key=True),
        sa.Column("gubun", sa.String(length=2), nullable=False),
        sa.Column("tr_date", sa.Date(), nullable=False),
        sa.Column("customer", sa.Text(), nullable=False),
        sa.Column("item", sa.Text(), nullable=False),
        sa.Column("spec", sa.Text(), server_default=sa.text("''"), nullable=False),
        sa.Column("price", sa.BigInteger(), nullable=False),
        sa.Column("amount", sa.BigInteger(), nullable=False),
        sa.Column("supply_val", sa.BigInteger(), nullable=False),
        sa.Column("tax_val", sa.BigInteger(), server_default=sa.text("0"), nullable=False),
        sa.Column("total_val", sa.BigInteger(), nullable=False),
        sa.Column("pay_date1", sa.Date()),
        sa.Column("pay_amt1", sa.BigInteger(), server_default=sa.text("0"), nullable=False),
        sa.Column("pay_date2", sa.Date()),
        sa.Column("pay_amt2", sa.BigInteger(), server_default=sa.text("0"), nullable=False),
        sa.Column("pay_date3", sa.Date()),
        sa.Column("pay_amt3", sa.BigInteger(), server_default=sa.text("0"), nullable=False),
        sa.Column(
            "unpaid_amt",
            sa.BigInteger(),
            sa.Computed(
                "CASE WHEN gubun = '매입' THEN total_val - COALESCE(pay_amt1, 0) "
                "- COALESCE(pay_amt2, 0) - COALESCE(pay_amt3, 0) ELSE 0 END",
                persisted=True,
            ),
        ),
        sa.Column(
            "receivable_amt",
            sa.BigInteger(),
            sa.Computed(
                "CASE WHEN gubun = '매출' THEN total_val - COALESCE(pay_amt1, 0) "
                "- COALESCE(pay_amt2, 0) - COALESCE(pay_amt3, 0) ELSE 0 END",
                persisted=True,
            ),
        ),
        sa.Column("created_at", sa.DateTime(timezone=True), server_default=sa.func.now(), nullable=False),
        sa.Column("updated_at", sa.DateTime(timezone=True), server_default=sa.func.now(), nullable=False),
        sa.CheckConstraint("gubun IN ('매입', '매출')", name="ck_records_gubun"),
    )
    op.create_index("ix_records_date_gubun", "records", ["tr_date", "gubun"])
    op.create_index("ix_records_customer", "records", ["customer"])
    op.create_index("ix_records_item", "records", ["item"])


def downgrade() -> None:
    op.drop_table("records")
    op.drop_table("item")
    op.drop_table("customer")
