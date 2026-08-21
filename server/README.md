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

PostgreSQL 애플리케이션 백업은 홈서버의 `server/backups` 디렉터리에 JSON으로 저장됩니다. 이 디렉터리는 컨테이너에 바인드 마운트되므로 컨테이너를 다시 만들더라도 유지됩니다.

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
- `GET /transfers/snapshot`
- `POST /transfers/backups`
- `POST /transfers/replace`
- `POST /transfers/restore/{backup_file}`

모든 엔드포인트는 `X-API-Key` 헤더가 필요합니다.

## SQLite ↔ PostgreSQL 이전 안전장치

Qt 메인 화면의 `데이터 관리` 메뉴에 있는 데이터 이전 기능은 대상 데이터베이스를 병합하지 않고 전체 교체합니다.

1. 원본 데이터베이스 스냅샷 백업
2. 대상 데이터베이스의 기존 데이터 백업
3. 트랜잭션 안에서 거래처, 품목, 전표 전체 교체
4. 거래처 잔액과 입금 내역을 포함한 전체 데이터 해시 검증
5. 검증 성공 시에만 커밋
6. Qt에서 대상을 다시 읽어 원본과 건수 및 해시를 한 번 더 비교

예전 SQLite 파일에서 비어 있는 숫자 값은 기존 프로그램의 계산 방식과 동일하게 `0`, 빈 규격(`spec`)은 빈 문자열로 정규화합니다. 날짜 형식이 손상된 전표는 임의로 고치지 않고 전표 ID를 포함한 오류로 이전을 중단합니다.

서버 백업을 복구해야 하는 경우 API 키와 백업 파일명을 사용합니다.

```bash
curl -X POST \
  -H "X-API-Key: 실제_API키" \
  "http://127.0.0.1:8000/transfers/restore/postgres_백업파일.json"
```

복구 직전의 PostgreSQL 상태도 다시 백업한 후 복구가 실행됩니다.

## 테스트용 PostgreSQL 초기화

가장 안전한 방법은 Qt 메인 화면의 `데이터 관리` → `PostgreSQL 데이터 초기화...`를 사용하는 것입니다. 서버 백업을 먼저 만들고 거래처·품목·전표를 비운 뒤, 세 항목이 모두 0건인지 검증합니다. Compose 볼륨과 서버 설정은 그대로 유지됩니다.

홈서버에서 PostgreSQL 볼륨 자체를 완전히 새로 만들려면 `server` 디렉터리에서 아래 순서로 실행합니다. 이 방법은 PostgreSQL 볼륨의 모든 데이터를 복구하기 어렵게 삭제하므로 테스트 DB에만 사용하세요.

```bash
docker compose down -v
docker compose up -d --build
docker compose ps
```

`server/backups`는 호스트 디렉터리로 바인드 마운트되어 있어 `down -v`로 PostgreSQL 볼륨을 지워도 남습니다. 새 컨테이너가 시작될 때 Alembic이 빈 스키마를 다시 생성합니다.
