# CMAI2100F2 OCAP 이식 검토 안내

기준: 첨부 CMAI2100F2 - v2.7.1.zip / 참조: CMAI2100Dj(5).zip

## 적용 내용

DJ의 OCAPCosmetic을 공통 형태로 삼아 FAI와 Cosmetic을 구성했습니다. 클래스 파일명은 OCAPFAI / OCAPCosmetic을 유지하고, 멤버/함수는 각각 _FAI / _Cosmetic 접미사로 구분했습니다. F2에는 매거진이 없으므로 MZ 관련 상태와 함수는 Lot 기준으로 변경했습니다.

- 메인 하단 Door Lock과 PM 사이에 OCAP 버튼 추가 → FAI / Cosmetic 선택.
- 두 설정 화면은 DJ Cosmetic과 같은 100개 코드, 20개씩 5페이지, 좌우 10개 배치.
- 코드별 이름, 연속 Lot 수, 기준 불량률, 최근 완료 Lot 50개 이력 제공.
- 기본 공통 조건 3개 및 F+코드 측정 Error 조건 1개.
- Main의 모드별 버튼 표시 규칙에 맞춰 Operator 모드에서는 OCAP 버튼을 숨김.
- 원본 C++ 소스 CP949, 리소스 UTF-16, 프로젝트 XML UTF-8 및 CRLF 유지.

## 실제 검사 판정과 집계 기준

F2 Inspector.cpp의 Get_InspectComplete 기준입니다. DJ 숫자 판정을 그대로 사용하지 않았습니다.

| 비전 수신 판정 | 카메라별 저장값 | OCAP 구분 |
|---|---:|---|
| G | 2 | 양품 |
| S / T / W | 7 / 8 / 9 | FAI 치수불량 |
| N / R / X / M | 3 / 4 / 5 / 6 | Cosmetic 외관불량 |

카메라별 결과와 최종 모듈 결과의 숫자 의미는 다릅니다. 최종 결과 [0]의 5는 Barcode/MES/MC 등으로 사용되므로 외관 코드로 취급하지 않습니다.

1. FAI는 최종 모듈 판정이 7~9인 모듈에서 카메라 판정 7~9의 코드를 확인합니다.
2. Cosmetic은 최종 모듈 판정이 3 또는 4인 모듈에서 카메라 판정 3~6의 코드를 확인합니다. ROS 최종 양품(2), 특수 NG는 제외합니다.
3. 동일 모듈·동일 등록 코드는 카메라 여러 개에 나타나도 1회만 집계합니다. 서로 다른 등록 코드가 검출되면 각 코드에 1회씩 집계하므로 코드별 수량의 합계는 NG 모듈 수와 다를 수 있습니다.
4. F2가 보관하는 카메라별 sNGCode_I를 사용합니다. DJ의 gNG 추가 20개 코드 배열은 F2에 없으므로 이식하지 않았습니다.
5. 전체 Lot의 10개 Tray × 40개 Pocket을 순회합니다. 미사용/빈 Pocket은 최종 판정이 없으므로 집계되지 않습니다. 이는 기존 F2 배열 한도이며 Tray 수를 10개보다 늘리는 변경은 하지 않았습니다.
6. 분모는 DJ와 동일한 취지로 해당 Lot의 투입 수량 gLot.nCmCount입니다. 불량률(%) = 해당 코드 불량 모듈 수 × 100 / Lot 투입 수량입니다. 예: 320개 Lot에서 코드 A 모듈 8개 → 2.5%입니다.
7. FAI/Cosmetic은 기존 최종 판정 우선순위를 따릅니다. 한 모듈에 치수·외관 코드가 함께 있어도 최종 치수 NG이면 FAI 대상으로 집계합니다. Barcode/MES/MC 등으로 먼저 확정된 모듈을 치수·외관 NG로 다시 분류하지 않습니다.

## F+코드와 측정 Error

등록 코드 A에 대해 수신 코드 FA는 A의 불량 수량과 별도의 측정 Error 수량에 모두 포함합니다. 동일 모듈에 A와 FA가 함께 있어도 A 불량 1개 / Error 1개입니다.

단, FA 자체가 별도 등록되어 있다면 FA의 정확한 코드 매칭을 우선하고 A의 Error로 중복 해석하지 않습니다. FAI와 Cosmetic의 등록 목록은 독립적입니다.

## Lot 종료 연결과 중복 방지

기존 종료 경로를 따라 Elevator2 → Check_LotEndTray / bEmptyNGTray → Set_LotEnd로 들어옵니다. Set_LotEnd에서 포트/동일 Lot/이미 종료 여부를 확인하고, 결과 복사본을 OCAP 대기열에 넣습니다. 이후 기존 UM_LOT_END_MSG에서 WorkDlg::OnShowLotEndMsg → Common::Set_LotDataClear가 실행되어 원본 포트 데이터가 지워져도 복사본이 남습니다.

- bOcapFinalized[6]: 포트별 동일 종료 결과의 중복 등록 방지. Lot 시작 및 포트 초기화에서 재설정.
- OCAP_LOT_DATA: Lot ID, 수량, 모든 Tray/Pocket 판정·코드·ROS 결과, 종료 시각 복사.
- 시퀀스 스레드는 결과 복사만 수행합니다. 메인 UI의 500ms 타이머에서 집계·이력·CSV·알람 조건을 처리합니다.
- Inspector는 다른 Lot의 지연 응답, 종료 후 응답, 이미 최종 완료된 모듈, 동일 카메라 중복 COMPLETE를 로그와 함께 제외합니다.
- 수동 Abort/Delete는 정상 완료 Lot로 추가하지 않습니다.
- 프로그램을 종료하면 메모리의 이력·대기열은 사라집니다. DJ처럼 재실행 시 연속 이력을 CSV에서 복원하지 않으며, CSV 로그는 별도로 남습니다.

## 연속 Lot 알람

최근 완료 Lot부터 역순으로 각 Lot이 기준 불량률 이상인지 검사합니다. N Lot의 불량 수량을 모두 합산해 가중 평균을 내는 방식이 아닙니다. 한 Lot이라도 기준 미만, 최소 수량 미만, 투입 0개, 미완료 이력이면 연속 조건을 끊습니다. 기준과 같은 값은 이상으로 판정합니다.

- 조건 3 → 조건 2 → 조건 1 → 측정 Error 순서이며, 같은 구분에서는 등록 코드 앞순위가 우선입니다.
- DJ와 같이 한 완료 Lot당 FAI 1건 / Cosmetic 1건까지 선택합니다. 모든 불량 코드·모든 조건의 알람을 한꺼번에 표시하는 기능은 아닙니다.
- 각 구분의 알람은 별도 대기열에 보관해 앞선 알람 확인 후 다음 알람을 보여줍니다. 다른 Lot의 집계가 진행되어도 표시 중인 알람 문구와 Lot ID가 바뀌지 않습니다.
- FAI 9181~9183, FAI 측정 Error 9184 / Cosmetic 9185~9187, Cosmetic 측정 Error 9188.
- DFA/외관 공정 전달, Master Sample/Correlation 확인, 설비 확인 문구는 DJ를 따르고 매거진 단위는 연속 Lot으로 명시했습니다. 실제값·설정값은 소수 3자리로 표시합니다.
- OCAP 알람은 메인 UI에서 기존 ErrorDlg를 동기 표시해 기존 정지/부저/램프 동작을 사용합니다. 기존 Show_Error의 새 스레드 표시 방식은 OCAP 경로에서 사용하지 않습니다.

## 최초 설정과 파일

실행 폴더 기준 System/OCAP_Data.ini를 사용합니다. 파일이 없으면 최초 실행 때 생성합니다. 실제 장비 코드나 알람 기준을 임의로 정하지 않았으므로, 최초에는 코드가 비어 있고 공통 연속 Lot 수량은 0(비활성)입니다. OCAP 화면에서 실제 비전 코드와 운용 기준을 설정한 후 저장해야 알람이 동작합니다.

설정은 Job 종료 후 저장할 수 있습니다. 저장 직전 이미 완료된 결과를 이전 설정으로 먼저 처리합니다. 실행 중 코드가 바뀌어 같은 Lot에 서로 다른 설정이 적용되는 것을 방지하기 위한 제한입니다.

| 설정 | 의미 |
|---|---|
| 공통 연속 Lot 수 0 | 해당 공통 조건 비활성 |
| 공통 연속 Lot 수 1~40 | 조건 적용 대상 연속 완료 Lot 수 |
| Lot 최소 모듈 수 1~400 | 미만인 Lot은 연속 조건 제외 |
| 코드별 연속 Lot 수 -1 | 각 공통 조건의 연속 Lot 수 사용 |
| 코드별 연속 Lot 수 0 | 그 코드 알람 제외 |
| 코드별 연속 Lot 수 1~40 | 모든 조건에서 그 코드의 개별 연속 수 사용 |
| 코드별 불량률 -1 | 각 공통 조건의 기준 사용 |
| 코드별 불량률 0초과~100 | 모든 조건에서 그 코드의 개별 기준 사용 |

개별 연속 수/기준을 지정하면 각 조건에서 공통값을 대체합니다. 예를 들어 코드별 1 Lot / 2%를 지정하면 조건 3도 해당 값으로 평가되어 가장 먼저 선택될 수 있습니다. DJ Cosmetic의 독립 대체 방식을 유지했습니다.

INI 섹션: OPTION-FAI / OPTION-COSMETIC, FAI-CODE / FAI-NAME / FAI-LOT-COUNT / FAI-DEFECTP, COSMETIC-CODE / COSMETIC-NAME / COSMETIC-LOT-COUNT / COSMETIC-DEFECTP. 공통 키는 LOT_COUNT_01~04, DEFECTP_01~04, LOT_MIN_COUNT입니다. DJ의 MZ 기준 INI를 그대로 복사해 적용하지 않습니다.

코드와 이름은 중복·금지 문자·입력 범위를 검증합니다. 저장 후 파일을 재확인합니다. 코드 문자열이 변경된 열은 과거 다른 코드 수량을 이어 쓰지 않도록 해당 열을 초기화합니다. 공통 설정을 포함해 저장하지 않은 변경은 닫을 때 확인합니다.

CSV: LOG/OCAPData_FAI 및 LOG/OCAPData_Cosmetic. Lot별 100개 코드의 수량·불량률·Error 수량·Error 불량률을 기록하며, 코드/이름 구성 해시별로 파일을 구분합니다.

## 파일별 변경 위치

| 파일 | 변경 |
|---|---|
| OCAPFAI.h/.cpp, OCAPCosmetic.h/.cpp | 대화상자, 100개 코드 설정, Lot 이력, 연속 조건 |
| OCAPSelect.h/.cpp | FAI/Cosmetic 선택 |
| OCAPRules.h | 판정 분류, 정확한 코드/F+ 매칭, 모듈별 중복 제거, 기준 비교 |
| OCAPManager.h/.cpp | 완료 복사본, 스레드 간 대기열, 알람 문구 고정·순차 표시 |
| stdafx.h/.cpp | 두 OCAP 전역 상태 및 포트별 중복 방지 플래그 |
| SequenceMain.cpp | Lot 시작/종료/초기화 연결, 종료 시 0 수량 나눗셈 방어 |
| Inspector.cpp | 늦은/중복 완료 패킷의 재반영 방지 |
| Common.cpp | 포트 초기화 시 중복 방지 플래그 초기화 |
| DataManager.h/.cpp | 두 종류 설정 초기화·INI 읽기 |
| LogFile.h/.cpp | 두 종류 완료 Lot CSV |
| CMAI2100Dlg.h/.cpp | 버튼, 선택, 대화상자 생명주기, 타이머 집계 |
| ErrorDlg.cpp | OCAP 상세 문구 및 종료 Lot ID 로그 연결 |
| CMAI2100.rc, resource.h | 세 대화상자와 메인 버튼 리소스 |
| CMAI2100.vcxproj/.filters | 소스/헤더 프로젝트 등록 |

CHANGES_REVIEW.txt는 기존 파일 변경만 보여주는 UTF-8 검토용 비교본입니다. 실제 적용은 ZIP의 전체 프로젝트를 사용하세요.

## 수행한 검증과 남은 확인

통과한 자동 검증:

- 실제 OCAPRules.h와 두 클래스의 집계/연속 조건 함수를 추출해 C++03 모드로 컴파일·실행.
- 실제 OCAPManager.cpp를 작은 호스트 어댑터와 함께 컴파일·실행해 결과 복사, 포트 초기화 후 보존, 중복 대기열 등록 방지, 알람 순차 처리를 검증.
- S/T/W 및 N/R/X/M 분류, 첫/마지막 Tray·Pocket, 다중 카메라 중복 제거, F+코드 및 정확한 등록 코드 우선, ROS Good/특수 NG 제외.
- 최소 수량 경계, 0 투입, 기준 일치/미달, 정상 Lot이 중간에 들어오는 경우, 개별 대체/비활성, Error 조건, 이력 50개 초과 순환.
- AddressSanitizer와 UndefinedBehaviorSanitizer 사용. 실행 환경 제약으로 LeakSanitizer의 종료 시 프로세스 검사는 비활성화.
- 프로젝트 XML/필터, 파일 등록, 리소스 ID 중복, 연속 컨트롤 ID, 클래스 선언/정의, 전역 멤버 참조, 원본 초기화 전 복사 순서 검사.

Windows/MFC UI 및 장비 드라이버는 이 환경에서 빌드·실행하지 못했습니다. 위 테스트는 MFC·CString 서식·파일 I/O·실제 스레드 경합·부저/램프를 검증하는 전체 프로그램 테스트가 아닙니다.

현장 확인 순서:

1. 기존 프로젝트와 System 설정을 백업하고 별도 폴더에서 CMAI2100.sln을 VS2010으로 열어 Debug/Release Win32 빌드.
2. 실행 폴더에 기존 System 설정·장비 의존 DLL/라이브러리가 있는 상태로 실행. OCAP 버튼·선택 화면·두 설정 화면의 글꼴/배치/5페이지 확인.
3. 실제 S/T/W 및 N/R/X/M 코드와 일치하는 코드를 등록. 먼저 1 Lot / 낮은 기준으로 시험용 조건 설정.
4. 동일 코드가 여러 카메라와 여러 Tray에서 들어오는 시험 Lot으로 수량/CSV/비율 확인. FA와 A 및 별도 FA 등록도 구분 시험.
5. ROS NG/Repair/Good과 Barcode/MES/MC/특수 NG가 섞인 시험 Lot으로 제외 기준 확인.
6. 연속 N Lot, 사이에 정상 Lot, 최소 수량 미만 Lot, 기준과 정확히 같은 불량률을 각각 확인.
7. FAI와 Cosmetic이 같은 Lot에서 동시에 걸리는 경우 순차 확인, 앞선 일반 알람이 표시 중인 경우 OCAP 대기, 알람의 Lot ID·기준값·실제값 확인.
8. Lot 종료 직후 포트 데이터 초기화·다음 Lot 입력 뒤에도 이전 완료 Lot이 정확히 1행인지 확인. 정상 종료와 수동 Abort를 구분.
9. 장비의 기존 정지/재시작, 부저, 타워램프 동작과 알람 9181~9188의 외부 MES/로그 연동 확인.

자동 검증 재실행( Python 3 + g++가 있는 환경 ):

    python3 OCAP_Review/tests/test_ocap.py
    python3 OCAP_Review/tests/check_integration.py
