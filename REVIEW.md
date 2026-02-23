# 매입매출장 전체 동작 리뷰

## ✅ 수정·보완한 항목

### 1. 총 입금액 / 검색 결과 안 나오는 문제 (C++)
- **원인**: `readRecordRange`와 `calcSearchedSum`에서 거래처/상품 필터 추가 시 SQL에 공백 누락 (`'매입'AND customer` → 잘못된 쿼리).
- **조치**: `sqlDatahandler.cpp`에서 `" AND customer = ..."`, `" AND item = ..."` 처럼 앞에 공백을 넣어 수정함.  
  → 거래처·상품 조건 사용 시에도 검색·합계(총 입금액 등)가 정상 동작함.

### 2. 검색 결과 컬럼 밸런스 (QML)
- **조치**: 구분/일자/거래처/품명/규격 등 모든 컬럼에 `Layout.preferredWidth`를 지정해, 품명이 거래처를 침범하지 않도록 함.  
  거래처·품명 각 118px, 나머지 컬럼도 고정 폭으로 맞춤.

### 3. 상품 목록이 비어 있을 때 입력 영역 (QML)
- **위험**: `productList`가 빈 배열일 때 `productList[productComboBox.currentIndex].spec` 접근 시 런타임 오류 가능.
- **조치**: `textSize`, `textPrice` 바인딩에 `productList.length > 0 && productList[productComboBox.currentIndex]` 조건을 넣어, 비어 있으면 빈 문자열을 쓰도록 방어 코드 추가함.

---

## ✅ 점검된 동작 (문제 없음)

| 구분 | 내용 |
|------|------|
| **C++ ↔ QML 연동** | `main.cpp`에서 `sqlData`, `excelData`, `sync` 컨텍스트 등록됨. |
| **검색 결과 구조** | `getSearchedResult()`가 `id, gb, date, supplier, product, size, price, quantity, gongga, buga, hapgye, ipD1~3, ipA1~3, miji, misu` 키를 반환하며, 리스트 delegate의 `modelData.*`와 일치함. |
| **상품 데이터** | `getDataProduct()`가 `id, name, spec, price` 객체 배열을 반환. ComboBox `textRole: "name"`, 입력란 `.spec`/`.price` 사용과 일치함. |
| **월별 통계** | `MonthTotal`이 `mainWindow.supplierSearchList`, `mainWindow.productSearchList`를 참조. 메인 창 자식으로 생성되므로 스코프 문제 없음. |
| **캘린더** | `MyCalendar`가 `calendarPopup`, `scalendarPopup1/2`, `calendarButton`, `searchCalendarFirst/Second` 등 메인 쪽 id를 참조. 팝업 contentItem으로 사용되므로 스코프 내에 있음. |
| **입금 수정** | `writeRecordIp(..., searchResultList.selectedRow)`로 선택 행 id 전달. C++ `writeRecordIp`는 해당 id로 `records` 업데이트. |
| **삭제** | `deleteAskPopup.row = modelData.id`, `deleteRecord(deleteAskPopup.row)`로 일치함. |

---

## ⚠️ 참고·권장 사항

1. **날짜 형식**  
   검색/등록 시 날짜가 QML `Date` → C++ `QVariant`로 넘어갈 때, `toString()`/`toDateTime()`/`toDate()` 결과가 환경에 따라 다를 수 있음.  
   가능하면 C++에서 `QDate::fromString(..., "yyyy-MM-dd")` 등으로 형식을 고정해 쓰는 것이 안전함.

2. **상품 추가 후 push**  
   `productAddPopup`에서 `mainWindow.productList.push(productAddName.text)`로 문자열만 넣고 있으나, 직후 `sqlData.refreshData()`와 `productList = sqlData.getDataProduct()`로 덮어쓰므로 실제 동작에는 문제 없음.  
   의도를 명확히 하려면 해당 `push` 한 줄은 제거해도 됨.

3. **빌드**  
   C++ 수정(`sqlDatahandler.cpp`) 반영을 위해 프로젝트를 한 번 다시 빌드해야 함.

---

## 요약

- **총 입금액·검색 결과**: SQL 공백 수정으로 정상 동작할 것으로 예상됨.
- **검색 결과 테이블**: 컬럼 폭 고정으로 거래처/품명 밸런스 확보됨.
- **데이터 흐름·연동**: 점검한 범위 내에서 일관되게 맞춰져 있음.
- **예외 방지**: 상품 목록이 비어 있을 때 입력란 접근만 방어 코드로 보완함.

위 사항 반영 후, 한 번 빌드하고 실제로 등록·검색·입금수정·삭제·월별통계·캘린더를 실행해 보면 전체 동작 확인에 도움이 됨.
