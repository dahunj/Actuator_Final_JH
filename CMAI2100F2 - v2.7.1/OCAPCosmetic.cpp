// OCAPCosmetic.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "CMAI2100.h"
#include "OCAPCosmetic.h"
#include "Common.h"
#include "LogFile.h"
#include "DataManager.h"
#include "afxdialogex.h"
#include <math.h>
#include "OCAPRules.h"


// OCAPCosmetic 대화 상자입니다.
// 동적 생성하는 OCAP 전용 컨트롤 ID (다른 대화상자와 공유하지 않음)
#define IDC_OCAP_CODE_PREV  61000
#define IDC_OCAP_CODE_NEXT  61001
#define IDC_OCAP_CODE_VIEW  61002
#define IDC_OCAP_CODE_FIND  61003

static BOOL OCAP_ReadLotCount(CString str, int& nValue);
static BOOL OCAP_ReadDefectRate(CString str, double& dValue);

OCAPCosmetic g_dlgOCAP_Cosmetic;

IMPLEMENT_DYNAMIC(OCAPCosmetic, CDialogEx)

OCAPCosmetic::OCAPCosmetic(CWnd* pParent /*=NULL*/)
	: CDialogEx(OCAPCosmetic::IDD, pParent)
{
	m_bError_Cosmetic = FALSE;
	m_bSetting_Cosmetic = TRUE;
	m_nCodePage_Cosmetic = 0;
}

OCAPCosmetic::~OCAPCosmetic()
{
}

void OCAPCosmetic::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_btnSave_Cosmetic);
	DDX_Control(pDX, IDCANCEL, m_btnClose_Cosmetic);
	for (int i = 0; i < 2; i++) DDX_Control(pDX, IDC_GROUP_0 + i, m_Group_Cosmetic[i]);
	for (int i = 0; i < 7; i++) DDX_Control(pDX, IDC_LABEL_0 + i, m_Label_Cosmetic[i]);
	for (int i = 0; i < 9; i++) DDX_Control(pDX, IDC_STC_OCAP_VALUE_0 + i, m_stcOption_Cosmetic[i]);
	DDX_Control(pDX, IDC_GRD_OCAP_DATA, m_grdData_Cosmetic);
}


BEGIN_MESSAGE_MAP(OCAPCosmetic, CDialogEx)
	ON_WM_SHOWWINDOW()
	ON_WM_CLOSE()
	ON_CONTROL_RANGE(BN_CLICKED, IDC_OCAP_CODE_PREV, IDC_OCAP_CODE_FIND, OnCodeButton)
	ON_BN_CLICKED(IDCANCEL, &OCAPCosmetic::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK,		&OCAPCosmetic::OnBnClickedOk)
	ON_CONTROL_RANGE(STN_CLICKED, IDC_STC_OCAP_VALUE_0, IDC_STC_OCAP_VALUE_8, OnStcOptionClick)
END_MESSAGE_MAP()


BOOL OCAPCosmetic::OnInitDialog() 
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	SetWindowPos(this, 0, 75, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	Initial_Controls_Cosmetic();
	Initial_CodeControls_Cosmetic();

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

BOOL OCAPCosmetic::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE))
		return TRUE;

	return CDialogEx::PreTranslateMessage(pMsg);
}

void OCAPCosmetic::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	if (bShow) {
		Display_Option_Cosmetic();
		Initial_NameGrid_Cosmetic(&m_grdData_Cosmetic, 51, 416);
		Load_CodeData_Cosmetic();
		Display_CodePage_Cosmetic();
		Show_CodeControls_Cosmetic();
		Display_Status_Cosmetic();
	} else {
	}
}

// OCAPCosmetic 메시지 처리기입니다.

void OCAPCosmetic::OnBnClickedCancel()
{
	BOOL bChanged = FALSE;
	CString str;
	for (int i=0; i<100; i++) {
		if (m_sCode_Cosmetic[i] != gCap_Cosmetic.sCosmeticCode_Cosmetic[i] || m_sName_Cosmetic[i] != gCap_Cosmetic.sCosmeticName_Cosmetic[i] || m_nLotCnt_Cosmetic[i] != gCap_Cosmetic.nCodeLotCnt_Cosmetic[i] || m_dDefect_Cosmetic[i] != gCap_Cosmetic.dCodeDefect_Cosmetic[i]) { bChanged = TRUE; break; }
	}
	for (int i=0; i<20 && !bChanged; i++) {
		int nIx = m_nCodePage_Cosmetic * 20 + i;
		m_edtCode_Cosmetic[i].GetWindowText(str); if (str != m_sCode_Cosmetic[nIx]) bChanged = TRUE;
		m_edtName_Cosmetic[i].GetWindowText(str); if (str != m_sName_Cosmetic[nIx]) bChanged = TRUE;
		CString strCount; strCount.Format("%d", m_nLotCnt_Cosmetic[nIx]);
		m_edtLotCnt_Cosmetic[i].GetWindowText(str); if (str != strCount) bChanged = TRUE;
		strCount.Format("%0.3lf", m_dDefect_Cosmetic[nIx]);
		m_edtDefect_Cosmetic[i].GetWindowText(str); if (str != strCount) bChanged = TRUE;
	}
	int nOptionIndex[4] = { 0, 3, 5, 7 };
	int nRateIndex[4] = { 1, 4, 6, 8 };
	for (int i=0; i<4 && !bChanged; i++) {
		m_stcOption_Cosmetic[nOptionIndex[i]].GetWindowText(str);
		if (atoi(str) != gCap_Cosmetic.nGiLotCnt_Cosmetic[i]) bChanged = TRUE;
		m_stcOption_Cosmetic[nRateIndex[i]].GetWindowText(str);
		if (fabs(atof(str) - gCap_Cosmetic.dGiDefect_Cosmetic[i]) > 0.0000001) bChanged = TRUE;
	}
	m_stcOption_Cosmetic[2].GetWindowText(str);
	if (atoi(str) != gCap_Cosmetic.nGiMinCnt_Cosmetic) bChanged = TRUE;

	if (bChanged && AfxMessageBox("저장하지 않은 코드 설정을 취소하고 닫습니까?", MB_YESNO) != IDYES) return;
	ShowWindow(SW_HIDE);
}


void OCAPCosmetic::OnBnClickedOk()
{
	//if (gLot.nJobStatus != 0) { AfxMessageBox("OCAP 설정은 Job 종료 후 저장하세요."); return; }
	OCAP_ProcessLots();
	if (!Read_CodePage_Cosmetic() || !Check_CodeData_Cosmetic()) { m_bSetting_Cosmetic = TRUE; Show_CodeControls_Cosmetic(); return; }

	int		nData1, nData2, nData3, nData4, nData5;
	double	dData1, dData2, dData3, dData4;
	CString strData, strLog;
	for (int i=0; i<9; i++) {
		m_stcOption_Cosmetic[i].GetWindowText(strData);
		double dValue = atof(strData);
		BOOL bCount = (i == 0 || i == 3 || i == 5 || i == 7);
		if ((bCount && (dValue < 0 || dValue > 40 || floor(dValue) != dValue)) ||
			(i == 2 && (dValue < 1 || dValue > 400 || floor(dValue) != dValue)) ||
			(!bCount && i != 2 && !(dValue > 0 && dValue <= 100))) {
			AfxMessageBox("공통 조건을 확인하세요. 연속 Lot 0~40 / 최소 수량 1~400 / 불량률 0초과~100%"); return;
		}
	}

	CIniFileCS INI(gsCurrentDir + "\\System\\OCAP_Data.ini");
	if (!INI.Check_File()) {
		AfxMessageBox("OCAP_Data.ini File Not Found!");
		return;
	}

	m_stcOption_Cosmetic[0].GetWindowText(strData); nData1 = atoi(strData); INI.Set_Integer("OPTION-COSMETIC", "LOT_COUNT_01", nData1);
	m_stcOption_Cosmetic[1].GetWindowText(strData); dData1 = atof(strData); INI.Set_Double("OPTION-COSMETIC", "DEFECTP_01", dData1, "%0.3lf");
	m_stcOption_Cosmetic[3].GetWindowText(strData); nData2 = atoi(strData); INI.Set_Integer("OPTION-COSMETIC", "LOT_COUNT_02", nData2);
	m_stcOption_Cosmetic[4].GetWindowText(strData); dData2 = atof(strData); INI.Set_Double("OPTION-COSMETIC", "DEFECTP_02", dData2, "%0.3lf");
	m_stcOption_Cosmetic[5].GetWindowText(strData); nData3 = atoi(strData); INI.Set_Integer("OPTION-COSMETIC", "LOT_COUNT_03", nData3);
	m_stcOption_Cosmetic[6].GetWindowText(strData); dData3 = atof(strData); INI.Set_Double("OPTION-COSMETIC", "DEFECTP_03", dData3, "%0.3lf");
	m_stcOption_Cosmetic[7].GetWindowText(strData); nData4 = atoi(strData); INI.Set_Integer("OPTION-COSMETIC", "LOT_COUNT_04", nData4);
	m_stcOption_Cosmetic[8].GetWindowText(strData); dData4 = atof(strData); INI.Set_Double("OPTION-COSMETIC", "DEFECTP_04", dData4, "%0.3lf");
	m_stcOption_Cosmetic[2].GetWindowText(strData); nData5 = atoi(strData); INI.Set_Integer("OPTION-COSMETIC", "LOT_MIN_COUNT", nData5);

	strLog.Format("[OCAP Cosmetic Option] OnBnClickedOk - Data1(%d,%0.3lf) Data2(%d,%0.3lf) Data3(%d,%0.3lf)  Data4(%d,%0.3lf) Skip(%d)", nData1, dData1, nData2, dData2, nData3, dData3, nData4, dData4, nData5);
	g_objLogFile.Save_HandlerLog(strLog);

	// 코드 변경 시 이전 코드의 수량을 새 코드에 사용하지 않는다.
	CString strKey;
	for (int i=0; i<100; i++) {
		strKey.Format("%02d", i + 1);
		INI.Set_String("COSMETIC-CODE", strKey, m_sCode_Cosmetic[i]);
		INI.Set_String("COSMETIC-NAME", strKey, m_sName_Cosmetic[i]);
		INI.Set_Integer("COSMETIC-LOT-COUNT", strKey, m_nLotCnt_Cosmetic[i]);
		INI.Set_Double("COSMETIC-DEFECTP", strKey, m_dDefect_Cosmetic[i], "%0.3lf");
		strLog.Format("[OCAP Cosmetic Code] No(%03d) Code(%s) Name(%s) Lot(%d) Defect(%0.3lf)", i + 1, m_sCode_Cosmetic[i], m_sName_Cosmetic[i], m_nLotCnt_Cosmetic[i], m_dDefect_Cosmetic[i]);
		g_objLogFile.Save_HandlerLog(strLog);
	}
	// 저장한 파일을 다시 읽어 전체 설정값이 일치하는지 확인한다.
	::WritePrivateProfileString(NULL, NULL, NULL, gsCurrentDir + "\\System\\OCAP_Data.ini");
	BOOL bSaved = TRUE;
	for (int i=0; i<100 && bSaved; i++) {
		strKey.Format("%02d", i + 1);
		if (INI.Get_String("COSMETIC-CODE", strKey, "#READ_ERROR#") != m_sCode_Cosmetic[i] ||
			INI.Get_String("COSMETIC-NAME", strKey, "#READ_ERROR#") != m_sName_Cosmetic[i] ||
			INI.Get_Integer("COSMETIC-LOT-COUNT", strKey, -9999) != m_nLotCnt_Cosmetic[i] ||
			fabs(INI.Get_Double("COSMETIC-DEFECTP", strKey, -9999.0) - m_dDefect_Cosmetic[i]) > 0.0000001) bSaved = FALSE;
	}
	int nLot[4] = { nData1, nData2, nData3, nData4 };
	double dDefect[4] = { dData1, dData2, dData3, dData4 };
	for (int i=0; i<4 && bSaved; i++) {
		strKey.Format("LOT_COUNT_%02d", i + 1);
		if (INI.Get_Integer("OPTION-COSMETIC", strKey, -9999) != nLot[i]) bSaved = FALSE;
		strKey.Format("DEFECTP_%02d", i + 1);
		if (fabs(INI.Get_Double("OPTION-COSMETIC", strKey, -9999.0) - dDefect[i]) > 0.0000001) bSaved = FALSE;
	}
	if (INI.Get_Integer("OPTION-COSMETIC", "LOT_MIN_COUNT", -9999) != nData5) bSaved = FALSE;
	if (!bSaved) { AfxMessageBox("OCAP_Data.ini 저장/읽기 검증에 실패했습니다. 파일 쓰기 권한을 확인하세요."); return; }
	for (int i=0; i<100; i++) {
		if (m_sCode_Cosmetic[i] != gCap_Cosmetic.sCosmeticCode_Cosmetic[i]) {
			for (int j=0; j<50; j++) gCap_Cosmetic.nCount_Cosmetic[j][i] = gCap_Cosmetic.nFCount_Cosmetic[j][i] = 0;
		}
	}
	if (!g_objDataManager.Read_OCAPData_Cosmetic()) return;
	Load_CodeData_Cosmetic();
	Display_CodePage_Cosmetic();

	Initial_NameGrid_Cosmetic(&m_grdData_Cosmetic, 51, 416);
	Display_Option_Cosmetic();
	Display_Status_Cosmetic();
	AfxMessageBox("OCAP_Data.ini 저장 및 읽기 확인이 완료되었습니다.");
}

void OCAPCosmetic::Initial_Controls_Cosmetic() 
{
	m_btnSave_Cosmetic.Init_Ctrl("바탕", 14, TRUE, RGB(0, 0, 255), COLOR_DEFAULT, 0, 0);
	m_btnClose_Cosmetic.Init_Ctrl("바탕", 14, TRUE, RGB(128, 0, 128), COLOR_DEFAULT, 0, 0);
	for (int i = 0; i < 2; i++) m_Group_Cosmetic[i].Init_Ctrl("바탕", 12, TRUE, COLOR_DEFAULT, COLOR_DEFAULT);
	for (int i = 0; i < 6; i++) m_Label_Cosmetic[i].Init_Ctrl("바탕", 12, FALSE, RGB(0xFF, 0xFF, 0xFF), RGB(0x80, 0x00, 0x80));
	m_Label_Cosmetic[0].Init_Ctrl("바탕", 12, TRUE, RGB(0x00, 0x00, 0x00), RGB(0x99, 0x99, 0x00));
	m_Label_Cosmetic[4].Init_Ctrl("바탕", 12, TRUE, RGB(0x00, 0x00, 0x00), RGB(0x99, 0x99, 0x00));
	m_Label_Cosmetic[5].Init_Ctrl("바탕", 12, TRUE, RGB(0x00, 0x00, 0x00), RGB(0x99, 0x99, 0x00));
	m_Label_Cosmetic[6].Init_Ctrl("바탕", 12, TRUE, RGB(0x00, 0x00, 0x00), RGB(0x99, 0x99, 0x00));
	for (int i = 0; i < 9; i++) m_stcOption_Cosmetic[i].Init_Ctrl("바탕", 11, TRUE, COLOR_DEFAULT, RGB(0xFF, 0xFF, 0xE0));
}

void OCAPCosmetic::Display_Option_Cosmetic()
{
	CString strData;
	strData.Format("%d", gCap_Cosmetic.nGiLotCnt_Cosmetic[0]);	     m_stcOption_Cosmetic[0].SetWindowText(strData);
	strData.Format("%0.3lf", gCap_Cosmetic.dGiDefect_Cosmetic[0]); m_stcOption_Cosmetic[1].SetWindowText(strData);
	strData.Format("%d", gCap_Cosmetic.nGiLotCnt_Cosmetic[1]);	     m_stcOption_Cosmetic[3].SetWindowText(strData);
	strData.Format("%0.3lf", gCap_Cosmetic.dGiDefect_Cosmetic[1]); m_stcOption_Cosmetic[4].SetWindowText(strData);
	strData.Format("%d", gCap_Cosmetic.nGiLotCnt_Cosmetic[2]);	     m_stcOption_Cosmetic[5].SetWindowText(strData);
	strData.Format("%0.3lf", gCap_Cosmetic.dGiDefect_Cosmetic[2]); m_stcOption_Cosmetic[6].SetWindowText(strData);
	strData.Format("%d", gCap_Cosmetic.nGiLotCnt_Cosmetic[3]);	     m_stcOption_Cosmetic[7].SetWindowText(strData);
	strData.Format("%0.3lf", gCap_Cosmetic.dGiDefect_Cosmetic[3]); m_stcOption_Cosmetic[8].SetWindowText(strData);
	strData.Format("%d", gCap_Cosmetic.nGiMinCnt_Cosmetic);	     m_stcOption_Cosmetic[2].SetWindowText(strData);
}

void OCAPCosmetic::OnStcOptionClick(UINT nID)
{
	int		nData = 0;
	double	dData = 0.0;
	int ID = nID - IDC_STC_OCAP_VALUE_0;

	CString strOld, strNew, strLog;
	m_stcOption_Cosmetic[ID].GetWindowText(strOld);
	if (g_objCommon.Show_NumPad(strOld, strNew) != IDOK) return;

	BOOL bCount = (ID == 0 || ID == 3 || ID == 5 || ID == 7);
	if (bCount && (!OCAP_ReadLotCount(strNew, nData) || nData < 0)) { AfxMessageBox("연속 Lot 수량은 0~40 정수로 입력하세요."); return; }
	if (!bCount && ID != 2 && !OCAP_ReadDefectRate(strNew, dData)) { AfxMessageBox("기준 불량률은 0초과~100%로 입력하세요."); return; }
	if (!bCount && ID != 2 && dData <= 0.0) { AfxMessageBox("기준 불량률은 0보다 커야 합니다."); return; }
	if (ID == 2) {
		if (strNew.IsEmpty() || strNew.GetLength() > 3) { AfxMessageBox("Lot 최소 수량은 1~400 정수로 입력하세요."); return; }
		for (int i=0; i<strNew.GetLength(); i++) if (strNew[i] < '0' || strNew[i] > '9') { AfxMessageBox("수량은 정수로 입력하세요."); return; }
		nData = atoi(strNew); if (nData < 1 || nData > 400) { AfxMessageBox("Lot 최소 수량은 1~400입니다."); return; }
	}

	m_stcOption_Cosmetic[ID].SetWindowText(strNew);
}

void OCAPCosmetic::Initial_NameGrid_Cosmetic(CGridCS *pGrid, int nRows, int nCols)
{
	pGrid->Set_RowCount(nRows);
	pGrid->Set_ColCount(nCols);
	pGrid->Set_FixRowCount(1);
	pGrid->Set_FixColCount(3);

	long lTotalW = pGrid->Get_GridWidth();
	long lTotalH = pGrid->Get_GridHeight();
	long lCellW = lTotalW / nCols;
	long lCellH = 26;
	long  lWidth[16]= { 60,    60,      100,          60,     60,      60,    50,   50,    50,    50,    50,    50,    50,     50,    50,     50};
	CString sTxt[16]= {"날짜","시간","Lot ID", "투입", "완공", "양품", "RN", "RR", "치명", "BCR", "MES", "MC", "외관", "001~007", "008~015", "016~020"};
	long  lWidthName[2]= { 105, 120 };

	for (int i=0; i<nRows; i++) {
		pGrid->Set_RowHeight(i, lCellH);
		for (int j=0; j<16; j++) {
			if(i==0) {
				pGrid->Set_ColWidth(j, lWidth[j]); pGrid->Set_CellText(i, j, sTxt[j]);
				pGrid->Set_CellBackClr(i, j, RGB(0xCC, 0xCC, 0xCC));
			}
		}

		int nW = 0, nName=0; 
		for (int j=16; j<216; j++) {
			if(i==0) {
				pGrid->Set_ColWidth(j, lWidthName[nW]);
				if (nW == 0) { CString str; str.Format("Cosmetic-%03d", nName + 1); pGrid->Set_CellText(i, j, str); }
				else		 pGrid->Set_CellText(i, j, gCap_Cosmetic.sCosmeticName_Cosmetic[nName]);
				nW++; if (nW == 2) { nW = 0; nName++; }
				if (j>=16 && j<=29) pGrid->Set_CellBackClr(i, j, RGB(0xFF, 0xFF, 0xCC));
				if (j>=30 && j<=45) pGrid->Set_CellBackClr(i, j, RGB(0x99, 0xFF, 0x99));
				if (j>=46 && j<=215) pGrid->Set_CellBackClr(i, j, RGB(0x33, 0xFF, 0x33));
			}
		}
		nW = 0; nName=0; 
		for (int j=216; j<416; j++) {
			if(i==0) {
				pGrid->Set_ColWidth(j, lWidthName[nW]);
				if (nW == 0) { CString str; str.Format("ERR-%03d", nName + 1); pGrid->Set_CellText(i, j, str); }
				else		 pGrid->Set_CellText(i, j, gCap_Cosmetic.sCosmeticName_Cosmetic[nName]);
				nW++; if (nW == 2) { nW = 0; nName++; }
				if (j>=216 && j<=229) pGrid->Set_CellBackClr(i, j, RGB(0xCC, 0xCC, 0xCC));
				if (j>=230 && j<=245) pGrid->Set_CellBackClr(i, j, RGB(0x99, 0x99, 0x99));
				if (j>=246 && j<=415) pGrid->Set_CellBackClr(i, j, RGB(0x66, 0x66, 0x66));
			}
		}
	}
}

void OCAPCosmetic::Initial_DataGrid_Cosmetic(CGridCS *pGrid, int nRows, int nCols)
{
	pGrid->Set_RowCount(nRows);
	pGrid->Set_ColCount(nCols);
	pGrid->Set_FixRowCount(0);
	pGrid->Set_FixColCount(0);

	long lTotalW = pGrid->Get_GridWidth();
	long lTotalH = pGrid->Get_GridHeight();
	long lCellW = lTotalW / nCols;
	long lCellH = (lTotalH / nRows);
	long  lWidth[416]= { 35, 105, 50, 33, 50};
	for (int i=5; i<nCols; i=i+2) {
		lWidth[i] = lWidth[3];
	}
	for (int i=6; i<nCols; i=i+2) {
		lWidth[i] = lWidth[4];
	}

	for (int i=0; i<nRows; i++) {
		pGrid->Set_RowHeight(i, lCellH);
		for (int j=0; j<nCols; j++) {
			if(i==0) {
				pGrid->Set_ColWidth(j, lWidth[j]);
				pGrid->Set_CellBackClr(i, j, RGB(0xF0, 0xF0, 0xC0));
			}
		}
	}
}

void OCAPCosmetic::Display_Status_Cosmetic()
{
	if (gCap_Cosmetic.nLotCycle_Cosmetic < 0 || gCap_Cosmetic.nLotCycle_Cosmetic > 49) return;
	int nIx = (gCap_Cosmetic.nLotCycle_Cosmetic + 49) % 50;
	for (int i=0; i<50; i++) {
		if (gCap_Cosmetic.sDate_Cosmetic[nIx].GetLength() < 1) break;
		Display_Grid_Cosmetic(i + 1, nIx);
		nIx = (nIx + 49) % 50;
	}
}

void OCAPCosmetic::Display_Grid_Cosmetic(int nDp, int nIx)
{
	if (nDp < 1 || nDp > 50 || nIx < 0 || nIx > 49) return;
	CString str;

	str.Format(_T("%s"), gCap_Cosmetic.sDate_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 0, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 0, str);

	str.Format(_T("%s"), gCap_Cosmetic.sTime_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 1, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 1, str);

	str.Format(_T("%s"), gCap_Cosmetic.sLotID_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 2, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 2, str);

	str.Format(_T("%d"), gCap_Cosmetic.nTotCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 3, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 3, str);

	str.Format(_T("%d"), gCap_Cosmetic.nGoodCount_Cosmetic[nIx] + gCap_Cosmetic.nROSNGCount_Cosmetic[nIx] + gCap_Cosmetic.nROSRfCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 4, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 4, str);

	str.Format(_T("%d"), gCap_Cosmetic.nGoodCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 5, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 5, str);

	str.Format(_T("%d"), gCap_Cosmetic.nROSNGCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 6, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 6, str);

	str.Format(_T("%d"), gCap_Cosmetic.nROSRfCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 7, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 7, str);

	str.Format(_T("%d"), gCap_Cosmetic.nBCRCount_Cosmetic[nIx] + gCap_Cosmetic.nMESCount_Cosmetic[nIx] + gCap_Cosmetic.nMCCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 8, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 8, str);

	str.Format(_T("%d"), gCap_Cosmetic.nBCRCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 9, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 9, str);

	str.Format(_T("%d"), gCap_Cosmetic.nMESCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 10, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 10, str);

	str.Format(_T("%d"), gCap_Cosmetic.nMCCount_Cosmetic[nIx]);
	m_grdData_Cosmetic.Set_CellFont(nDp, 11, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 11, str);

	int nA1 = gCap_Cosmetic.nCount_Cosmetic[nIx][0] + gCap_Cosmetic.nCount_Cosmetic[nIx][1] + gCap_Cosmetic.nCount_Cosmetic[nIx][2] + gCap_Cosmetic.nCount_Cosmetic[nIx][3] + gCap_Cosmetic.nCount_Cosmetic[nIx][4] + gCap_Cosmetic.nCount_Cosmetic[nIx][5] + gCap_Cosmetic.nCount_Cosmetic[nIx][6];
	int nA2 = gCap_Cosmetic.nCount_Cosmetic[nIx][7] + gCap_Cosmetic.nCount_Cosmetic[nIx][8] + gCap_Cosmetic.nCount_Cosmetic[nIx][9] + gCap_Cosmetic.nCount_Cosmetic[nIx][10] + gCap_Cosmetic.nCount_Cosmetic[nIx][11] + gCap_Cosmetic.nCount_Cosmetic[nIx][12] + gCap_Cosmetic.nCount_Cosmetic[nIx][13] + gCap_Cosmetic.nCount_Cosmetic[nIx][14];
	int nA3 = gCap_Cosmetic.nCount_Cosmetic[nIx][15] + gCap_Cosmetic.nCount_Cosmetic[nIx][16] + gCap_Cosmetic.nCount_Cosmetic[nIx][17] + gCap_Cosmetic.nCount_Cosmetic[nIx][18] + gCap_Cosmetic.nCount_Cosmetic[nIx][19];
	int nTotal = nA1 + nA2 + nA3;
	for (int i=20; i<100; i++) nTotal += gCap_Cosmetic.nCount_Cosmetic[nIx][i];
	str.Format(_T("%d"), nTotal);
	m_grdData_Cosmetic.Set_CellFont(nDp, 12, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 12, str);

	str.Format(_T("%d"), nA1);
	m_grdData_Cosmetic.Set_CellFont(nDp, 13, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 13, str);

	str.Format(_T("%d"), nA2);
	m_grdData_Cosmetic.Set_CellFont(nDp, 14, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 14, str);

	str.Format(_T("%d"), nA3);
	m_grdData_Cosmetic.Set_CellFont(nDp, 15, str, 10, FALSE);
	m_grdData_Cosmetic.Set_CellText(nDp, 15, str);

	double dValue;
	int	   nNo = 16;
	for(int i=0; i<100; i++) {
		str.Format(_T("%d"), gCap_Cosmetic.nCount_Cosmetic[nIx][i]);
		m_grdData_Cosmetic.Set_CellFont(nDp, nNo, str, 10, FALSE);
		m_grdData_Cosmetic.Set_CellText(nDp, nNo, str);
		nNo++;

		dValue = gCap_Cosmetic.nTotCount_Cosmetic[nIx] > 0 ? (gCap_Cosmetic.nCount_Cosmetic[nIx][i] * 100.0) / gCap_Cosmetic.nTotCount_Cosmetic[nIx] : 0.0;
		str.Format(_T("%0.3f%%"), dValue);
		m_grdData_Cosmetic.Set_CellFont(nDp, nNo, str, 10, FALSE);
		m_grdData_Cosmetic.Set_CellText(nDp, nNo, str);
		if		((gCap_Cosmetic.dCodeDefect_Cosmetic[i] == -1.0 ? gCap_Cosmetic.dGiDefect_Cosmetic[2] : gCap_Cosmetic.dCodeDefect_Cosmetic[i]) <= dValue)	m_grdData_Cosmetic.Set_CellBackClr(nDp, nNo, RGB(0xFF, 0x00, 0x00));	// 3 NG
		else if	((gCap_Cosmetic.dCodeDefect_Cosmetic[i] == -1.0 ? gCap_Cosmetic.dGiDefect_Cosmetic[1] : gCap_Cosmetic.dCodeDefect_Cosmetic[i]) <= dValue)	m_grdData_Cosmetic.Set_CellBackClr(nDp, nNo, RGB(0x00, 0x00, 0xFF));	// 2 NG
		else if	((gCap_Cosmetic.dCodeDefect_Cosmetic[i] == -1.0 ? gCap_Cosmetic.dGiDefect_Cosmetic[0] : gCap_Cosmetic.dCodeDefect_Cosmetic[i]) <= dValue)	m_grdData_Cosmetic.Set_CellBackClr(nDp, nNo, RGB(0x00, 0xFF, 0x00));	// 1 NG
		else									m_grdData_Cosmetic.Set_CellBackClr(nDp, nNo, RGB(0xFF, 0xFF, 0xFF));	// 0 Empty

		if (gCap_Cosmetic.nTotCount_Cosmetic[nIx] < gCap_Cosmetic.nGiMinCnt_Cosmetic) m_grdData_Cosmetic.Set_CellBackClr(nDp, nNo, RGB(0xFF, 0xFF, 0xFF));	// 0 Empty
		nNo++;
	}
	nNo = 216;
	for(int i=0; i<100; i++) {
		str.Format(_T("%d"), gCap_Cosmetic.nFCount_Cosmetic[nIx][i]);
		m_grdData_Cosmetic.Set_CellFont(nDp, nNo, str, 10, FALSE);
		m_grdData_Cosmetic.Set_CellText(nDp, nNo, str);
		nNo++;

		dValue = gCap_Cosmetic.nTotCount_Cosmetic[nIx] > 0 ? (gCap_Cosmetic.nFCount_Cosmetic[nIx][i] * 100.0) / gCap_Cosmetic.nTotCount_Cosmetic[nIx] : 0.0;
		str.Format(_T("%0.3f%%"), dValue);
		m_grdData_Cosmetic.Set_CellFont(nDp, nNo, str, 10, FALSE);
		m_grdData_Cosmetic.Set_CellText(nDp, nNo, str);
		if		((gCap_Cosmetic.dCodeDefect_Cosmetic[i] == -1.0 ? gCap_Cosmetic.dGiDefect_Cosmetic[3] : gCap_Cosmetic.dCodeDefect_Cosmetic[i]) <= dValue)	m_grdData_Cosmetic.Set_CellBackClr(nDp, nNo, RGB(0xFF, 0x00, 0x00));	// 3 NG
		else									m_grdData_Cosmetic.Set_CellBackClr(nDp, nNo, RGB(0xFF, 0xFF, 0xFF));	// 0 Empty
		if (gCap_Cosmetic.nTotCount_Cosmetic[nIx] < gCap_Cosmetic.nGiMinCnt_Cosmetic) m_grdData_Cosmetic.Set_CellBackClr(nDp, nNo, RGB(0xFF, 0xFF, 0xFF));	// 0 Empty
		nNo++;
	}
}

// 정확히 등록된 코드 우선. F+등록코드는 FAI와 같은 측정 Error 방식이다.


void OCAPCosmetic::Set_AddLotData_Cosmetic(const OCAP_LOT_DATA& data)
{
	if (data.sLotID.IsEmpty()) return;
	if (gCap_Cosmetic.nLotCycle_Cosmetic < 0 || gCap_Cosmetic.nLotCycle_Cosmetic >= 50) gCap_Cosmetic.nLotCycle_Cosmetic = 0;
	int nIdx = gCap_Cosmetic.nLotCycle_Cosmetic;
	DelLotData_Cosmetic(nIdx);
	AddTrayToLot_Cosmetic(data, nIdx);
	// 1 Lot 안의 모든 Tray/Pocket을 순회한다. 동일 모듈/코드는 카메라가 달라도 1회 집계.
	for (int i=0; i<100; i++) {
		if (gCap_Cosmetic.sCosmeticCode_Cosmetic[i].IsEmpty()) continue;
		OCAP_CountCode(false, data, gCap_Cosmetic.sCosmeticCode_Cosmetic, i, CString("F"),
			gCap_Cosmetic.nCount_Cosmetic[nIdx][i], gCap_Cosmetic.nFCount_Cosmetic[nIdx][i]);
	}
	AddLotOut_Cosmetic(data.sLotID, data.timeCompleted);
}

void OCAPCosmetic::AddTrayToLot_Cosmetic(const OCAP_LOT_DATA& data, int nIdx)
{
	gCap_Cosmetic.sLotID_Cosmetic[nIdx] = data.sLotID;
	gCap_Cosmetic.nTotCount_Cosmetic[nIdx] = data.nTotal;
	gCap_Cosmetic.nGoodCount_Cosmetic[nIdx] = data.nGood;
	gCap_Cosmetic.nROSNGCount_Cosmetic[nIdx] = data.nRosCount[3];
	gCap_Cosmetic.nROSRfCount_Cosmetic[nIdx] = data.nRosCount[4];
	gCap_Cosmetic.nBCRCount_Cosmetic[nIdx] = data.nRosCount[7];
	gCap_Cosmetic.nMESCount_Cosmetic[nIdx] = data.nRosCount[8];
	gCap_Cosmetic.nMCCount_Cosmetic[nIdx] = data.nRosCount[9];
}

void OCAPCosmetic::DelLotData_Cosmetic(int nLotNo)
{
	if (nLotNo < 0 || nLotNo > 49) return;
	gCap_Cosmetic.sDate_Cosmetic[nLotNo] = gCap_Cosmetic.sTime_Cosmetic[nLotNo] = gCap_Cosmetic.sLotID_Cosmetic[nLotNo] = "";
	gCap_Cosmetic.nTotCount_Cosmetic[nLotNo] = gCap_Cosmetic.nGoodCount_Cosmetic[nLotNo] = gCap_Cosmetic.nROSNGCount_Cosmetic[nLotNo] = gCap_Cosmetic.nROSRfCount_Cosmetic[nLotNo] = gCap_Cosmetic.nBCRCount_Cosmetic[nLotNo] = gCap_Cosmetic.nMESCount_Cosmetic[nLotNo] = gCap_Cosmetic.nMCCount_Cosmetic[nLotNo] = 0;
	for(int j=0; j<100; j ++) {
		gCap_Cosmetic.nCount_Cosmetic[nLotNo][j] = gCap_Cosmetic.nFCount_Cosmetic[nLotNo][j] = 0;
	}
}

void OCAPCosmetic::AddLotOut_Cosmetic(CString sLotid, const SYSTEMTIME& time)
{
	if (gCap_Cosmetic.nLotCycle_Cosmetic < 0 || gCap_Cosmetic.nLotCycle_Cosmetic >= 50) return;
	if (gCap_Cosmetic.sLotID_Cosmetic[gCap_Cosmetic.nLotCycle_Cosmetic].GetLength() < 1) return;


	gCap_Cosmetic.sDate_Cosmetic[gCap_Cosmetic.nLotCycle_Cosmetic].Format("%02d/%02d", time.wMonth, time.wDay);
	gCap_Cosmetic.sTime_Cosmetic[gCap_Cosmetic.nLotCycle_Cosmetic].Format("%02d:%02d", time.wHour,  time.wMinute);

	CString strLog;
	if (gCap_Cosmetic.sLotID_Cosmetic[gCap_Cosmetic.nLotCycle_Cosmetic] != sLotid) {
		strLog.Format("[OCAP Cosmetic Option] Lot End - Cycle[%d] Data-Lot[%s] Read-Lot[%s]", gCap_Cosmetic.nLotCycle_Cosmetic, gCap_Cosmetic.sLotID_Cosmetic[gCap_Cosmetic.nLotCycle_Cosmetic], sLotid);
		g_objLogFile.Save_HandlerLog(strLog);
	}
	g_objLogFile.Save_OCAPDataLog_Cosmetic(gCap_Cosmetic.nLotCycle_Cosmetic);

	gCap_Cosmetic.nLotCycle_Cosmetic++;
	if (gCap_Cosmetic.nLotCycle_Cosmetic >= 50) gCap_Cosmetic.nLotCycle_Cosmetic = 0;

	if (gCap_Cosmetic.nPendingAlarm_Cosmetic != 0) return;
	m_bError_Cosmetic = FALSE;
	Check_DEFECT_Cosmetic(3); if (m_bError_Cosmetic) return;
	Check_DEFECT_Cosmetic(2); if (m_bError_Cosmetic) return;
	Check_DEFECT_Cosmetic(1); if (m_bError_Cosmetic) return;
	Check_DEFECTF_Cosmetic(4);
}

void OCAPCosmetic::Set_AddDEFECT_Cosmetic(CString sLotid, CString sCode)
{
	if (gCap_Cosmetic.nLotCycle_Cosmetic < 0 || gCap_Cosmetic.nLotCycle_Cosmetic > 49) return;
	if (sLotid.GetLength() < 2 || sCode.GetLength() < 1 ) return;

	for (int i=0; i<50; i++) {
		if (sLotid == gCap_Cosmetic.sLotID_Cosmetic[i]) 
		{
			for(int j=0; j<100; j++)
			{
				if (sCode == gCap_Cosmetic.sCosmeticCode_Cosmetic[j]) 
				{
					gCap_Cosmetic.nCount_Cosmetic[i][j]++;
					return;
				}
			}
			return;
		}
	}
}

void OCAPCosmetic::Check_DEFECT_Cosmetic(int nNo)
{
	int nX = nNo - 1;
	if (nX < 0 || nX > 3) return;

	int nDefCount;
	double dPer = 0.0;
	CString sDefect = "";

	for (int i=0; i<100; i++) 
	{
		if (gCap_Cosmetic.sCosmeticCode_Cosmetic[i].GetLength() < 1) continue;
		int nLotCnt = gCap_Cosmetic.nCodeLotCnt_Cosmetic[i] < 0 ? gCap_Cosmetic.nGiLotCnt_Cosmetic[nX] : gCap_Cosmetic.nCodeLotCnt_Cosmetic[i];
		if (nLotCnt <= 0 || nLotCnt > 40) continue;
		double dDefect = gCap_Cosmetic.dCodeDefect_Cosmetic[i] == -1.0 ? gCap_Cosmetic.dGiDefect_Cosmetic[nX] : gCap_Cosmetic.dCodeDefect_Cosmetic[i];
		if (!(dDefect > 0.0 && dDefect <= 100.0)) continue;

		nDefCount = 0;
		int nLotno = gCap_Cosmetic.nLotCycle_Cosmetic - 1;	if (nLotno < 0) nLotno = 49;
		for (int j=0; j<nLotCnt; j++) 
		{
			if (gCap_Cosmetic.sDate_Cosmetic[nLotno].IsEmpty() || gCap_Cosmetic.nTotCount_Cosmetic[nLotno] <= 0 || gCap_Cosmetic.nTotCount_Cosmetic[nLotno] < gCap_Cosmetic.nGiMinCnt_Cosmetic) break;

			dPer = (gCap_Cosmetic.nCount_Cosmetic[nLotno][i] * 100.0) / gCap_Cosmetic.nTotCount_Cosmetic[nLotno];
			if (!OCAP_IsEligibleRate(gCap_Cosmetic.nTotCount_Cosmetic[nLotno], gCap_Cosmetic.nCount_Cosmetic[nLotno][i], gCap_Cosmetic.nGiMinCnt_Cosmetic, dDefect)) break;

			nDefCount++;
			if (nDefCount == 1) 
			{
				gCap_Cosmetic.sAlmLotID_Cosmetic[i] = gCap_Cosmetic.sLotID_Cosmetic[nLotno];
				gCap_Cosmetic.sAlmCosmeticName_Cosmetic[i].Format("Cosmetic-%s", gCap_Cosmetic.sCosmeticName_Cosmetic[i]);
				gCap_Cosmetic.dAlmDefect_Cosmetic[i]  = dPer;
				gCap_Cosmetic.dAlmThreshold_Cosmetic[i] = dDefect;
				gCap_Cosmetic.nAlmCount_Cosmetic[i]   = nLotCnt;
				gCap_Cosmetic.nAlmIdx_Cosmetic = i;
			}
			nLotno--;
			if (nLotno < 0) nLotno = 49;
		}
		if (nDefCount >= nLotCnt) 
		{
			sDefect = sDefect + gCap_Cosmetic.sAlmCosmeticName_Cosmetic[i] + ". ";
			break;
		}
	}

	if (sDefect.GetLength() > 1) 
	{
		m_bError_Cosmetic = TRUE;
		gCap_Cosmetic.sPendingDefect_Cosmetic = sDefect;
		gCap_Cosmetic.nPendingAlarm_Cosmetic = 9184 + nX+1;
		return;
	}
}

void OCAPCosmetic::Check_DEFECTF_Cosmetic(int nNo)
{
	int nX = nNo - 1;
	if (nX < 0 || nX > 3) return;

	int nDefCount;
	double dPer = 0.0;
	CString sDefect = "";

	for (int i=0; i<100; i++) {
		if (gCap_Cosmetic.sCosmeticCode_Cosmetic[i].GetLength() < 1) continue;
		int nLotCnt = gCap_Cosmetic.nCodeLotCnt_Cosmetic[i] < 0 ? gCap_Cosmetic.nGiLotCnt_Cosmetic[nX] : gCap_Cosmetic.nCodeLotCnt_Cosmetic[i];
		if (nLotCnt <= 0 || nLotCnt > 40) continue;
		double dDefect = gCap_Cosmetic.dCodeDefect_Cosmetic[i] == -1.0 ? gCap_Cosmetic.dGiDefect_Cosmetic[nX] : gCap_Cosmetic.dCodeDefect_Cosmetic[i];
		if (!(dDefect > 0.0 && dDefect <= 100.0)) continue;

		nDefCount = 0;
		int nLotno = gCap_Cosmetic.nLotCycle_Cosmetic - 1;	if (nLotno < 0) nLotno = 49;
		for (int j=0; j<nLotCnt; j++) {
			if (gCap_Cosmetic.sDate_Cosmetic[nLotno].IsEmpty() || gCap_Cosmetic.nTotCount_Cosmetic[nLotno] <= 0 || gCap_Cosmetic.nTotCount_Cosmetic[nLotno] < gCap_Cosmetic.nGiMinCnt_Cosmetic) break;

			dPer = (gCap_Cosmetic.nFCount_Cosmetic[nLotno][i] * 100.0) / gCap_Cosmetic.nTotCount_Cosmetic[nLotno];
			if (!OCAP_IsEligibleRate(gCap_Cosmetic.nTotCount_Cosmetic[nLotno], gCap_Cosmetic.nFCount_Cosmetic[nLotno][i], gCap_Cosmetic.nGiMinCnt_Cosmetic, dDefect)) break;

			nDefCount++;
			if (nDefCount == 1) {
				gCap_Cosmetic.sAlmLotID_Cosmetic[i] = gCap_Cosmetic.sLotID_Cosmetic[nLotno];
				gCap_Cosmetic.sAlmCosmeticName_Cosmetic[i].Format("ERR-%s", gCap_Cosmetic.sCosmeticName_Cosmetic[i]);
				gCap_Cosmetic.dAlmDefect_Cosmetic[i]  = dPer;
				gCap_Cosmetic.dAlmThreshold_Cosmetic[i] = dDefect;
				gCap_Cosmetic.nAlmCount_Cosmetic[i]   = nLotCnt;
				gCap_Cosmetic.nAlmIdx_Cosmetic = i;
			}
			nLotno--;
			if (nLotno < 0) nLotno = 49;
		}
		if (nDefCount >= nLotCnt) 
		{
			sDefect = sDefect + gCap_Cosmetic.sAlmCosmeticName_Cosmetic[i] + ". ";
			break;
		}
	}

	if (sDefect.GetLength() > 1) {
		m_bError_Cosmetic = TRUE;
		gCap_Cosmetic.sPendingDefect_Cosmetic = sDefect;
		gCap_Cosmetic.nPendingAlarm_Cosmetic = 9188;
		return;
	}
}


// 기존 OCAP 리소스 내부에 동적으로 생성하여 다른 화면 리소스는 변경하지 않는다.
static CRect OCAP_CodeRect(CWnd* pWnd, int nX, int nY, int nW, int nH)
{
	CRect rect(nX, nY, nX + nW, nY + nH);
	::MapDialogRect(pWnd->GetSafeHwnd(), &rect);
	return rect;
}

void OCAPCosmetic::Initial_CodeControls_Cosmetic()
{
	CString sTitle[5] = { "No.", "불량 Code", "불량 Name", "연속 Lot 횟수", "기준 불량율 (%)" };
	int nX[5] = { 25, 55, 200, 365, 455 };
	int nW[5] = { 25, 135, 155, 80, 85 };
	for (int j=0; j<2; j++) {
		int nOffset = j * 530;
		for (int i=0; i<5; i++) {
			int nIx = j * 5 + i;
			m_stcCodeTitle_Cosmetic[nIx].Create(sTitle[i], WS_CHILD | SS_CENTER | SS_CENTERIMAGE, OCAP_CodeRect(this, nX[i] + nOffset, 89, nW[i], 18), this);
			m_stcCodeTitle_Cosmetic[nIx].Init_Ctrl("바탕", 11, TRUE, RGB(255, 255, 255), RGB(128, 0, 128));
		}
	}
	m_stcCodeLine_Cosmetic.Create("", WS_CHILD | SS_ETCHEDVERT, OCAP_CodeRect(this, 547, 89, 1, 303), this);
	for (int i=0; i<20; i++) {
		int nY = 112 + (i % 10) * 28;
		int nOffset = (i / 10) * 530;
		m_stcCodeNo_Cosmetic[i].Create("", WS_CHILD | SS_CENTER | SS_CENTERIMAGE, OCAP_CodeRect(this, nX[0] + nOffset, nY, nW[0], 21), this);
		m_edtCode_Cosmetic[i].Create(WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, OCAP_CodeRect(this, nX[1] + nOffset, nY, nW[1], 21), this, 61100 + i * 4);
		m_edtName_Cosmetic[i].Create(WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, OCAP_CodeRect(this, nX[2] + nOffset, nY, nW[2], 21), this, 61101 + i * 4);
		m_edtLotCnt_Cosmetic[i].Create(WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, OCAP_CodeRect(this, nX[3] + nOffset, nY, nW[3], 21), this, 61102 + i * 4);
		m_edtDefect_Cosmetic[i].Create(WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, OCAP_CodeRect(this, nX[4] + nOffset, nY, nW[4], 21), this, 61103 + i * 4);
		m_stcCodeNo_Cosmetic[i].Init_Ctrl("바탕", 11, TRUE, RGB(0, 0, 0), RGB(235, 235, 210));
		m_edtCode_Cosmetic[i].Init_Ctrl("바탕", 11, FALSE, RGB(0, 0, 0), RGB(255, 255, 240)); m_edtCode_Cosmetic[i].LimitText(80);
		m_edtName_Cosmetic[i].Init_Ctrl("바탕", 11, FALSE, RGB(0, 0, 0), RGB(255, 255, 240)); m_edtName_Cosmetic[i].LimitText(80);
		m_edtLotCnt_Cosmetic[i].Init_Ctrl("바탕", 11, TRUE, RGB(0, 0, 128), RGB(255, 255, 224)); m_edtLotCnt_Cosmetic[i].LimitText(3);
		m_edtDefect_Cosmetic[i].Init_Ctrl("바탕", 11, TRUE, RGB(0, 0, 128), RGB(255, 255, 224)); m_edtDefect_Cosmetic[i].LimitText(7);
	}
	m_stcCodeInfo_Cosmetic.Create("", WS_CHILD, OCAP_CodeRect(this, 25, 400, 1040, 46), this);
	m_stcCodeInfo_Cosmetic.Init_Ctrl("바탕", 10, FALSE, RGB(0, 0, 0), COLOR_DEFAULT);
	CString sButton[4] = { "이전 20개", "다음 20개", "이력 보기", "빈 코드로 이동" };
	int nBtnX[4] = { 25, 135, 930, 700 };
	int nBtnW[4] = { 100, 100, 125, 180 };
	for (int i=0; i<4; i++) {
		m_btnCode_Cosmetic[i].Create(sButton[i], WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, OCAP_CodeRect(this, nBtnX[i], 452, nBtnW[i], 26), this, IDC_OCAP_CODE_PREV + i);
		m_btnCode_Cosmetic[i].Init_Ctrl("바탕", 12, TRUE, RGB(0, 0, 255), COLOR_DEFAULT, 0, 0);
	}
	m_grdData_Cosmetic.MoveWindow(OCAP_CodeRect(this, 13, 83, 1071, 360));
	m_Label_Cosmetic[3].SetWindowText("공통 연속 수량");
}

void OCAPCosmetic::Load_CodeData_Cosmetic()
{
	for (int i=0; i<100; i++) {
		m_sCode_Cosmetic[i] = gCap_Cosmetic.sCosmeticCode_Cosmetic[i];
		m_sName_Cosmetic[i] = gCap_Cosmetic.sCosmeticName_Cosmetic[i];
		m_nLotCnt_Cosmetic[i] = gCap_Cosmetic.nCodeLotCnt_Cosmetic[i];
		m_dDefect_Cosmetic[i] = gCap_Cosmetic.dCodeDefect_Cosmetic[i];
	}
}

void OCAPCosmetic::Display_CodePage_Cosmetic()
{
	CString str;
	for (int i=0; i<20; i++) {
		int nIx = m_nCodePage_Cosmetic * 20 + i;
		str.Format("%03d", nIx + 1); m_stcCodeNo_Cosmetic[i].SetWindowText(str);
		m_edtCode_Cosmetic[i].SetWindowText(m_sCode_Cosmetic[nIx]);
		m_edtName_Cosmetic[i].SetWindowText(m_sName_Cosmetic[nIx]);
		str.Format("%d", m_nLotCnt_Cosmetic[nIx]); m_edtLotCnt_Cosmetic[i].SetWindowText(str);
		str.Format("%0.3lf", m_dDefect_Cosmetic[nIx]); m_edtDefect_Cosmetic[i].SetWindowText(str);
	}
	str.Format("%d / 5 페이지  (%03d ~ %03d)    -1: 위의 공통 조건 사용 / 0: 알람 제외 / 1~40: 코드별 연속 Lot 횟수\r\n"
		"불량율: -1은 공통 조건 / 0초과~100.000은 개별 기준(소수 3자리). 횟수와 불량율은 각각 독립 적용하며 [저장]으로 100개를 저장합니다.\r\n"
		"OCAP 알람은 1회만 발생합니다. (코드 앞순위 우선)", m_nCodePage_Cosmetic + 1, m_nCodePage_Cosmetic * 20 + 1, m_nCodePage_Cosmetic * 20 + 20);
	m_stcCodeInfo_Cosmetic.SetWindowText(str);
	m_btnCode_Cosmetic[0].EnableWindow(m_nCodePage_Cosmetic > 0);
	m_btnCode_Cosmetic[1].EnableWindow(m_nCodePage_Cosmetic < 4);
}

static BOOL OCAP_ReadLotCount(CString str, int& nValue)
{
	str.TrimLeft(); str.TrimRight();
	if (str.IsEmpty()) return FALSE;
	int nStart = str[0] == '-' ? 1 : 0;
	if (str.GetLength() == nStart) return FALSE;
	for (int i=nStart; i<str.GetLength(); i++) if (str[i] < '0' || str[i] > '9') return FALSE;
	nValue = atoi(str);
	return nValue >= -1 && nValue <= 40;
}

static BOOL OCAP_ReadDefectRate(CString str, double& dValue)
{
	str.TrimLeft(); str.TrimRight();
	if (str == "-1" || str == "-1.000") { dValue = -1.0; return TRUE; }
	if (str.IsEmpty()) return FALSE;
	int nPoint = -1, nDigits = 0;
	for (int i=0; i<str.GetLength(); i++) {
		if (str[i] == '.' && nPoint < 0) { nPoint = i; continue; }
		if (str[i] < '0' || str[i] > '9') return FALSE;
		nDigits++;
	}
	if (nDigits == 0 || (nPoint >= 0 && str.GetLength() - nPoint - 1 > 3)) return FALSE;
	dValue = atof(str);
	return dValue > 0.0 && dValue <= 100.0;
}

BOOL OCAPCosmetic::Read_CodePage_Cosmetic()
{
	CString sCode[20], sName[20], str;
	int nCount[20];
	double dDefect[20];
	for (int i=0; i<20; i++) {
		m_edtCode_Cosmetic[i].GetWindowText(sCode[i]); sCode[i].TrimLeft(); sCode[i].TrimRight();
		m_edtName_Cosmetic[i].GetWindowText(sName[i]); sName[i].TrimLeft(); sName[i].TrimRight();
		m_edtLotCnt_Cosmetic[i].GetWindowText(str);
		if (!OCAP_ReadLotCount(str, nCount[i])) {
			AfxMessageBox("연속 Lot 횟수는 -1, 0 또는 1~40 정수로 입력하세요.");
			m_edtLotCnt_Cosmetic[i].SetFocus(); return FALSE;
		}
		m_edtDefect_Cosmetic[i].GetWindowText(str);
		if (!OCAP_ReadDefectRate(str, dDefect[i])) {
			AfxMessageBox("기준 불량율은 -1 또는 0초과~100.000으로 입력하세요. 소수 3자리까지 가능합니다.");
			m_edtDefect_Cosmetic[i].SetFocus(); return FALSE;
		}
		if (sCode[i].FindOneOf("=;\r\n\"\t,") >= 0 || sName[i].FindOneOf("=;\r\n\"\t,") >= 0) {
			AfxMessageBox("Code / Name에 = ; 쉼표 따옴표 탭 문자는 사용할 수 없습니다.");
			m_edtCode_Cosmetic[i].SetFocus(); return FALSE;
		}
	}
	for (int i=0; i<20; i++) {
		int nIx = m_nCodePage_Cosmetic * 20 + i;
		m_sCode_Cosmetic[nIx] = sCode[i]; m_sName_Cosmetic[nIx] = sName[i]; m_nLotCnt_Cosmetic[nIx] = nCount[i]; m_dDefect_Cosmetic[nIx] = dDefect[i];
	}
	return TRUE;
}

BOOL OCAPCosmetic::Check_CodeData_Cosmetic()
{
	CString str;
	for (int i=0; i<100; i++) {
		if (m_sCode_Cosmetic[i].IsEmpty()) continue;
		if (m_sName_Cosmetic[i].IsEmpty()) {
			str.Format("%03d번 불량 Name을 입력하세요.", i + 1);
			AfxMessageBox(str); m_nCodePage_Cosmetic = i / 20; Display_CodePage_Cosmetic(); return FALSE;
		}
		for (int j=i+1; j<100; j++) {
			if (m_sCode_Cosmetic[i] != m_sCode_Cosmetic[j]) continue;
			str.Format("%03d번과 %03d번 불량 Code가 중복됩니다.", i + 1, j + 1);
			AfxMessageBox(str); m_nCodePage_Cosmetic = j / 20; Display_CodePage_Cosmetic(); return FALSE;
		}
	}
	return TRUE;
}

void OCAPCosmetic::Show_CodeControls_Cosmetic()
{
	int nShow = m_bSetting_Cosmetic ? SW_SHOW : SW_HIDE;
	for (int i=0; i<10; i++) m_stcCodeTitle_Cosmetic[i].ShowWindow(nShow);
	m_stcCodeLine_Cosmetic.ShowWindow(nShow);
	for (int i=0; i<20; i++) {
		m_stcCodeNo_Cosmetic[i].ShowWindow(nShow);
		m_edtCode_Cosmetic[i].ShowWindow(nShow); m_edtName_Cosmetic[i].ShowWindow(nShow); m_edtLotCnt_Cosmetic[i].ShowWindow(nShow); m_edtDefect_Cosmetic[i].ShowWindow(nShow);
	}
	m_stcCodeInfo_Cosmetic.ShowWindow(nShow);
	for (int i=0; i<4; i++) if (i != 2) m_btnCode_Cosmetic[i].ShowWindow(nShow);
	m_btnCode_Cosmetic[2].SetWindowText(m_bSetting_Cosmetic ? "이력 보기" : "코드 설정");
	m_grdData_Cosmetic.ShowWindow(m_bSetting_Cosmetic ? SW_HIDE : SW_SHOW);
}

void OCAPCosmetic::OnCodeButton(UINT nID)
{
	if (!Read_CodePage_Cosmetic()) return;
	if (nID == IDC_OCAP_CODE_PREV && m_nCodePage_Cosmetic > 0) m_nCodePage_Cosmetic--;
	if (nID == IDC_OCAP_CODE_NEXT && m_nCodePage_Cosmetic < 4) m_nCodePage_Cosmetic++;
	if (nID == IDC_OCAP_CODE_FIND) {
		for (int i=0; i<100; i++) if (m_sCode_Cosmetic[i].IsEmpty()) { m_nCodePage_Cosmetic = i / 20; break; }
	}
	if (nID == IDC_OCAP_CODE_VIEW) {
		m_bSetting_Cosmetic = !m_bSetting_Cosmetic;
		if (!m_bSetting_Cosmetic) Display_Status_Cosmetic();
		Show_CodeControls_Cosmetic();
	}
	Display_CodePage_Cosmetic();
}

void OCAPCosmetic::OnClose()
{
	OnBnClickedCancel();
}
