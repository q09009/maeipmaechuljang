from typing import Annotated

from fastapi import APIRouter, Depends, HTTPException, Response, status
from sqlalchemy import select
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session

from ..database import get_db
from ..models import Customer
from ..schemas import CustomerCreate, CustomerOut, CustomerUpdate


router = APIRouter(prefix="/customers", tags=["customers"])


@router.get("", response_model=list[CustomerOut])
def list_customers(db: Annotated[Session, Depends(get_db)]) -> list[Customer]:
    return list(db.scalars(select(Customer).order_by(Customer.name, Customer.id)))


@router.post("", response_model=CustomerOut, status_code=status.HTTP_201_CREATED)
def create_customer(payload: CustomerCreate, db: Annotated[Session, Depends(get_db)]) -> Customer:
    customer = Customer(name=payload.name.strip())
    db.add(customer)
    try:
        db.commit()
    except IntegrityError as exc:
        db.rollback()
        raise HTTPException(status_code=409, detail="Customer name already exists") from exc
    db.refresh(customer)
    return customer


@router.put("/{customer_id}", response_model=CustomerOut)
def update_customer(
    customer_id: int,
    payload: CustomerUpdate,
    db: Annotated[Session, Depends(get_db)],
) -> Customer:
    customer = db.get(Customer, customer_id)
    if customer is None:
        raise HTTPException(status_code=404, detail="Customer not found")
    customer.name = payload.name.strip()
    try:
        db.commit()
    except IntegrityError as exc:
        db.rollback()
        raise HTTPException(status_code=409, detail="Customer name already exists") from exc
    db.refresh(customer)
    return customer


@router.delete("/{customer_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_customer(customer_id: int, db: Annotated[Session, Depends(get_db)]) -> Response:
    customer = db.get(Customer, customer_id)
    if customer is None:
        raise HTTPException(status_code=404, detail="Customer not found")
    db.delete(customer)
    db.commit()
    return Response(status_code=status.HTTP_204_NO_CONTENT)
