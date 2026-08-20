# Maeip Maechuljang API

PostgreSQL은 실제 데이터를 보관하고 FastAPI만 PostgreSQL에 접속합니다. Qt, 웹, AI 클라이언트는 `X-API-Key`가 포함된 HTTP/JSON 요청만 보냅니다.

이 서버는 기존 Qt 빌드 폴더의 `data.db`를 열거나 수정하지 않습니다. SQLite 데이터 이전은 별도 기능으로 구현합니다.

## Docker Compose 실행

1. `.env.example`을 `.env`로 복사합니다.
2. `POSTGRES_PASSWORD`와 `MAEIP_API_KEY`를 충분히 긴 임의 값으로 변경합니다.
3. 웹 클라이언트를 사용할 경우 `MAEIP_CORS_ORIGINS`에 허용할 웹 주소만 적습니다.
4. 아래 명령을 실행합니다.

```powershell
docker compose up --build -d
```

FastAPI는 `http://127.0.0.1:8000`에서 열립니다. PostgreSQL의 5432 포트는 호스트에 공개하지 않고 Compose 내부에서만 사용합니다. Compose 프로젝트명은 `maeipmaechuljang`, 데이터 볼륨명은 `maeipmaechuljang_postgres_data`로 고정됩니다.

## 직접 실행

PostgreSQL이 이미 설치돼 있다면 `.env.example`을 `.env`로 복사하고 `DATABASE_URL` 줄의 주석을 푼 다음 `DATABASE_URL`과 `MAEIP_API_KEY`를 실제 값으로 변경해 실행할 수 있습니다.

```powershell
python -m venv .venv
.\.venv\Scripts\pip install -r requirements.txt
.\.venv\Scripts\alembic upgrade head
.\.venv\Scripts\uvicorn app.main:app --host 127.0.0.1 --port 8000
```

Qt 설정 화면에는 API 서버 주소와 동일한 API 키를 입력합니다.

## 주요 API

- `GET /health`
- `GET/POST/PUT/DELETE /customers`
- `GET/POST/PUT/DELETE /items`
- `GET/POST/DELETE /records`
- `GET /records/search`
- `GET /records/monthly`
- `GET /records/monthly-detail`
- `PUT /records/{id}/payment`
- `POST /records/bulk-payment`
- `POST /imports/records`
- `POST /imports/reference-data`

모든 엔드포인트는 `X-API-Key` 헤더가 필요합니다.
