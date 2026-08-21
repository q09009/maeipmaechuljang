# 📊 매입매출장 (maeipmaechuljang)

매입·매출 거래 기록을 효율적으로 관리하고 통계를 분석할 수 있는 
Qt/QML 기반 데스크톱 애플리케이션입니다.

## ✨ 주요 기능

### 📝 거래 기록 관리
- 매입/매출 거래 기록 입력 및 편집
- 거래처 및 상품 정보 관리
- 입금 내역 추적 (최대 3회차)
- 미수금/선수금 자동 계산

### 📈 통계 분석
- 월별 거래 통계
- 분기별 거래 통계
- 반기별 거래 통계
- 거래처별/상품별 필터링

### 💾 데이터 관리
- SQLite 데이터베이스 로컬 저장
- Excel 파일 양방향 동기화
- 자동 백업 (4시간 주기)
- 로그 기록 (7일 보관)

## 🛠️ 기술 스택

- C++ (Qt 6.x + QML)
- SQLite 데이터베이스
- FastAPI + PostgreSQL 서버 모드
- CMake 빌드 시스템
- QXlsx (엑셀 처리)

## 📦 설치 방법

### 시스템 요구사항
- Qt 6.0 이상
- CMake 3.16 이상
- C++17 호환 컴파일러

### 빌드
```bash
git clone https://github.com/q09009/maeipmaechuljang.git
cd maeipmaechuljang
mkdir build && cd build
cmake ..
cmake --build .
./maeipmaechuljang
```

## 🌐 FastAPI 서버 모드

서버 모드에서는 Qt 앱이 PostgreSQL에 직접 접속하지 않고 FastAPI에 HTTP 요청을 보냅니다. PostgreSQL 접속정보와 SQL 실행은 FastAPI 서버에서만 관리합니다.

설정 및 실행 방법은 [server/README.md](server/README.md)를 참고하세요. 기존 SQLite `data.db`는 서버 시작 과정에서 읽거나 변경하지 않습니다.

메인 화면의 `데이터 관리` 메뉴에서는 SQLite와 PostgreSQL 사이의 전체 데이터 이전을 실행할 수 있습니다. 이전 전 양쪽 데이터베이스를 백업하며, 거래처·품목·전표 건수와 내용 해시가 일치할 때만 성공으로 처리합니다. 대상 데이터베이스의 기존 내용은 병합되지 않고 교체됩니다.

같은 메뉴에서 SQLite 또는 PostgreSQL의 장부 데이터를 초기화할 수 있습니다. 초기화 직전에 자동 백업을 만들고, 거래처·품목·전표가 모두 0건인지 다시 검증합니다. 실행하려면 확인창에 `초기화`를 직접 입력해야 합니다.

레거시 SQLite의 빈 숫자 값은 `0`, 빈 규격(`spec`)은 빈 문자열로 정규화합니다. 거래처명·품목명 같은 핵심 문자값이나 날짜가 올바르지 않으면 해당 전표 ID를 알려주고 대상 데이터베이스를 변경하지 않습니다.
