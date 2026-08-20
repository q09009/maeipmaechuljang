import os
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

from dotenv import load_dotenv
from sqlalchemy.engine import URL


load_dotenv(Path(__file__).resolve().parents[1] / ".env")


@dataclass(frozen=True)
class Settings:
    database_url: str
    api_key: str
    cors_origins: tuple[str, ...]
    app_name: str = "Maeip Maechuljang API"


@lru_cache
def get_settings() -> Settings:
    database_url = os.getenv("DATABASE_URL", "").strip()
    if not database_url:
        db_user = os.getenv("DB_USER", "").strip()
        db_password = os.getenv("DB_PASSWORD", "")
        db_host = os.getenv("DB_HOST", "").strip()
        db_port = int(os.getenv("DB_PORT", "5432"))
        db_name = os.getenv("DB_NAME", "").strip()
        if not all((db_user, db_password, db_host, db_name)):
            raise RuntimeError(
                "DATABASE_URL or DB_USER/DB_PASSWORD/DB_HOST/DB_NAME is required"
            )
        database_url = URL.create(
            "postgresql+psycopg",
            username=db_user,
            password=db_password,
            host=db_host,
            port=db_port,
            database=db_name,
        ).render_as_string(hide_password=False)
    api_key = os.getenv("MAEIP_API_KEY", "").strip()
    cors_origins = tuple(
        origin.strip()
        for origin in os.getenv("MAEIP_CORS_ORIGINS", "").split(",")
        if origin.strip()
    )
    if not api_key:
        raise RuntimeError("MAEIP_API_KEY environment variable is required")
    return Settings(database_url=database_url, api_key=api_key, cors_origins=cors_origins)
