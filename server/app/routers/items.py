from typing import Annotated

from fastapi import APIRouter, Depends, HTTPException, Response, status
from sqlalchemy import select
from sqlalchemy.exc import IntegrityError
from sqlalchemy.orm import Session

from ..database import get_db
from ..models import Item
from ..schemas import ItemCreate, ItemOut, ItemUpdate


router = APIRouter(prefix="/items", tags=["items"])


@router.get("", response_model=list[ItemOut])
def list_items(db: Annotated[Session, Depends(get_db)]) -> list[Item]:
    return list(db.scalars(select(Item).order_by(Item.item_name, Item.spec, Item.id)))


@router.post("", response_model=ItemOut, status_code=status.HTTP_201_CREATED)
def create_item(payload: ItemCreate, db: Annotated[Session, Depends(get_db)]) -> Item:
    item = Item(item_name=payload.item_name.strip(), spec=payload.spec.strip(), price=payload.price)
    db.add(item)
    try:
        db.commit()
    except IntegrityError as exc:
        db.rollback()
        raise HTTPException(status_code=409, detail="Item and spec combination already exists") from exc
    db.refresh(item)
    return item


@router.put("/{item_id}", response_model=ItemOut)
def update_item(
    item_id: int,
    payload: ItemUpdate,
    db: Annotated[Session, Depends(get_db)],
) -> Item:
    item = db.get(Item, item_id)
    if item is None:
        raise HTTPException(status_code=404, detail="Item not found")
    item.item_name = payload.item_name.strip()
    item.spec = payload.spec.strip()
    item.price = payload.price
    try:
        db.commit()
    except IntegrityError as exc:
        db.rollback()
        raise HTTPException(status_code=409, detail="Item and spec combination already exists") from exc
    db.refresh(item)
    return item


@router.delete("/{item_id}", status_code=status.HTTP_204_NO_CONTENT)
def delete_item(item_id: int, db: Annotated[Session, Depends(get_db)]) -> Response:
    item = db.get(Item, item_id)
    if item is None:
        raise HTTPException(status_code=404, detail="Item not found")
    db.delete(item)
    db.commit()
    return Response(status_code=status.HTTP_204_NO_CONTENT)
