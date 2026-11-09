#pragma once
#include "OCAPManager.h"

// OCAPFAI 대화 상자입니다.

class OCAPFAI : public CDialogEx
{
	DECLARE_DYNAMIC(OCAPFAI)

public:
	OCAPFAI(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~OCAPFAI();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_OCAP_DLG };
	CGroupCS		m_Group_FAI[2];
	CLabelCS		m_Label_FAI[7];
	CStaticCS		m_stcOption_FAI[9];
	CGridCS			m_grdData_FAI;

	CButtonCS m_btnSave_FAI;
	CButtonCS m_btnClose_FAI;

	CLabelCS m_stcCodeTitle_FAI[10];
	CLabelCS m_stcCodeNo_FAI[20];
	CEditCS m_edtCode_FAI[20];
	CEditCS m_edtName_FAI[20];
	CEditCS m_edtLotCnt_FAI[20];
	CEditCS m_edtDefect_FAI[20];
	CButtonCS m_btnCode_FAI[4];
	CEditCS m_stcCodeInfo_FAI;
	CStatic m_stcCodeLine_FAI;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedCancel();
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	afx_msg void OnStcOptionClick(UINT nID);

private:
	
	BOOL m_bError_FAI;
	BOOL m_bSetting_FAI;
	int m_nCodePage_FAI;
	CString m_sCode_FAI[100];
	CString m_sName_FAI[100];
	int m_nLotCnt_FAI[100];
	double m_dDefect_FAI[100];
	

	void Initial_CodeControls_FAI();
	void Display_CodePage_FAI();
	void Show_CodeControls_FAI();
	BOOL Read_CodePage_FAI();
	BOOL Check_CodeData_FAI();
	void Load_CodeData_FAI();
	afx_msg void OnCodeButton(UINT nID);


	void Initial_NameGrid_FAI(CGridCS *pGrid, int nRows, int nCols);
	void Initial_DataGrid_FAI(CGridCS *pGrid, int nRows, int nCols);

	void Initial_Controls_FAI();
	void Display_Option_FAI();
	void Display_Grid_FAI(int nDp, int nIndex);

public:
	void Display_Status_FAI();
	void Set_AddLotData_FAI(const OCAP_LOT_DATA& data);
	void Set_AddDEFECT_FAI(CString sLotid, CString sCode);
	void Check_DEFECT_FAI(int nNo);
	void Check_DEFECTF_FAI(int nNo);
	void AddTrayToLot_FAI(const OCAP_LOT_DATA& data, int nIndex);
	void AddLotOut_FAI(CString sLotid, const SYSTEMTIME& time);
	void DelLotData_FAI(int nLotNo);
};

extern OCAPFAI g_dlgOCAP_FAI;
