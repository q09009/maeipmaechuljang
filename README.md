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
