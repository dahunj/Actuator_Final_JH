// MesAgent.h : 헤더 파일
//
#pragma once

// CMesAgent

class CMesAgent : public CWnd
{
	DECLARE_DYNAMIC(CMesAgent)

public:
	CMesAgent();
	virtual ~CMesAgent();

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnClientConnect(WPARAM wConnect, LPARAM lParam);
	afx_msg LRESULT OnClientReceive(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnClientClose(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
private:
	CClientSocketCS m_Client;
	
	BOOL	m_bConnected;
	BOOL	m_bHostOnline;
	CString m_strRecvCmd;
	int		m_nSizeCount;
	CString m_sSizeNG[100];

private:
	DWORD	m_dwReqStart[6][10][40];		// 요청시작

	void Get_ReciveData();
	void Get_ControlState(CString sFlag);	// 1:Online, 2:Offline
	void Get_TimeSync();

	void Get_LotStart(CString sLotId, CString sRecipe, CString sCmCount);
	void Get_LotCancel(CString sLotId, CString sCode, CString sText);
	
	void Get_RecipeList(CString sFlag);		// 0:All, 1:Current Recipe
	void Get_CmResult(CString sLotId, CString sCmId, CString sJudge, CString sNgCode, CString sNgText);
	void Get_CmFail(CString sLotId, CString sCmId, CString sCode, CString sText);
	void Get_HostMessage(CString sMsg);

	void Get_NGLotStart(CString sLotId, CString sRecipe);
	void Get_NGLotCancel(CString sLotId, CString sCode, CString sText);

	void Get_IdleReasonCode(CString sData);
	void Get_DownActionCode(CString sData);

	void Send_Command(CString sSend);

public:
	void Initialize();
	void Terminate();

	BOOL Is_Connected() { return m_bConnected; }
	BOOL Is_HostOnline() { return m_bHostOnline; }
	BOOL Exist_Recipe(CString sRecipe);

	void Set_OperUpdate(CString sOperId);				// Operator ID 변경시 보고
	void Set_ControlState(int nFlag, CString sOperId);	// 1:Onine, 2:Offline
	void Set_EquipState(int nFlag);						
	void Set_ErrorUpdate(int nFlag, CString sErrNo);	// 0: 해제, 1:발생
	void Set_RecipeList(int nFlag);						// 0: All, 1:Current Recipe
	void Set_IdleReport(CString sOperId, CString sSTime, CString sETime, CString sCode, CString sText, CString sType);	//1:Start, 2:End, 3: Auto 
	void Set_ModeChanged(int nMode);
	void Set_UnitState(int nState);

	void Set_CmRequest(CString sLotId, CString sCmId, int nPortNo, int nTrayNo, int nCmNo);
	void Set_CmEnd(int nType, int nPortNo, int nTrayNo, int nCmNo, int nOut, int nRosInfo, int nNGType= 0);

	void Set_UnitMaterialCount(int nMDCount, int nPortNo, int nInputCnt, int nOk, int nNG);

	void Set_LotStart(int nType, int nPortNo, CString sLotId, CString sRecipe, int nCount);	//0:Request, 1:Started
	void Set_LotAbort(CString sLotId);
	void Set_LotEnd(int nPortNo, CString sLotId, CString sRecipe, int nHCount, int nOk, int nNg);
	void Set_TerminalOK();
	CString Set_NGSort(int nPno, int nTNo, int CNo);

	void Set_NGLotRequest();
	void Set_NGLotEnd(CString sLotId, int nOk, int nNg);

	void Set_DownActionReport(CString sActionCode, CString sActionDetail, CString sStartTime, CString sEndTime, int nErrNo, int nErrCat, CString sErrMsg);
	void Set_UnitProcessingTimeReport(CString sLotID, CString sProcessID, CString sModelID, CString sRecipe, CString sTactTime, CString sCycleTime);


};

extern CMesAgent g_objMesAgent;

///////////////////////////////////////////////////////////////////////////////
