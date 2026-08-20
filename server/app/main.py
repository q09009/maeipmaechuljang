from fastapi import Depends, FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .config import get_settings
from .routers import customers, health, imports, items, records, transfers
from .security import require_api_key


settings = get_settings()
app = FastAPI(
    title=settings.app_name,
    version="0.1.0",
    dependencies=[Depends(require_api_key)],
)

if settings.cors_origins:
    app.add_middleware(
        CORSMiddleware,
        allow_origins=list(settings.cors_origins),
        allow_credentials=False,
        allow_methods=["GET", "POST", "PUT", "DELETE", "OPTIONS"],
        allow_headers=["Content-Type", "X-API-Key"],
    )

app.include_router(health.router)
app.include_router(customers.router)
app.include_router(items.router)
app.include_router(records.router)
app.include_router(imports.router)
app.include_router(transfers.router)
