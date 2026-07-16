// MesAgent.cpp : 구현 파일입니다.
//
#include "stdafx.h"
#include "Inspector.h"
#include "LogFile.h"
#include "Common.h"
#include "SequenceMain.h"
//#include "WorkDlg.h"
//#include "OperatorDlg.h"
#include "MesAgent.h"

#define	MES_AGENT_IP	"127.0.0.1"
#define MES_AGENT_PORT	10000		// MesAgent Handler Port

IMPLEMENT_DYNAMIC(CMesAgent, CWnd)

CMesAgent g_objMesAgent;

// CMesAgent

CMesAgent::CMesAgent()
{
	m_bConnected = FALSE;
	m_bHostOnline = FALSE;
	m_strRecvCmd = "";
	for(int i=0; i<100; i++) m_sSizeNG[i] = "";
}

CMesAgent::~CMesAgent()
{
}

BEGIN_MESSAGE_MAP(CMesAgent, CWnd)
	ON_WM_TIMER()
	ON_MESSAGE(UM_CLIENT_CONNECT, OnClientConnect)
	ON_MESSAGE(UM_CLIENT_RECEIVE, OnClientReceive)
	ON_MESSAGE(UM_CLIENT_CLOSE, OnClientClose)
END_MESSAGE_MAP()

// CMesAgent 메시지 처리기입니다.

void CMesAgent::Initialize()
{
	if (m_bConnected) return;

	m_Client.Open_Socket(MES_AGENT_IP, MES_AGENT_PORT, this);	Sleep(3000);

	CString sLog, sKey;
	CIniFileCS INI(gsCurrentDir + "\\System\\SizeNGOrder.ini");
	if (!INI.Check_File()) {
		sLog.Format("MesAgent-Initialize => SizeNGOrder.ini File Not Found!!!!");
		g_objLogFile.Save_MesAgentLog(sLog);
		AfxMessageBox(sLog);
		return;
	}

	m_nSizeCount = INI.Get_Integer("SIZENG", "COUNT", 0);
	if (m_nSizeCount > 100) m_nSizeCount = 100;
	for(int i=0; i<m_nSizeCount; i++) {
		sKey.Format("%02d", i+1);
		m_sSizeNG[i] = INI.Get_String("SIZENG", sKey, "");
	}		

	sLog.Format("MesAgent Initialize. SizeNG Count=%d", m_nSizeCount);
	g_objLogFile.Save_MesAgentLog(sLog);
}

void CMesAgent::Terminate()
{
	m_bConnected = FALSE;
	m_bHostOnline = FALSE;
	m_Client.Close_Socket();

	g_objLogFile.Save_MesAgentLog("MesAgent Terminate.");	Sleep(500);
}

/////////////////////////////////////////////////////////////////////////////

LRESULT CMesAgent::OnClientConnect(WPARAM wConnect, LPARAM lParam)
{
	m_bConnected = (BOOL)wConnect;
	if (!m_bConnected) return 0;

	Set_OperUpdate(gData.sOperID);
	Set_EquipState(eEquipState::IDLE);	
	g_objLogFile.Save_MesAgentLog("MesAgent Connected");
	return 0;
}

LRESULT CMesAgent::OnClientClose(WPARAM wParam, LPARAM lParam)
{
	m_bConnected = FALSE;
	m_bHostOnline = FALSE;
	m_Client.Close_Socket();
	g_objLogFile.Save_MesAgentLog("MesAgent Disconnected");
	return 0;
}

LRESULT CMesAgent::OnClientReceive(WPARAM wParam, LPARAM lParam)
{
	BYTE byRecv[1025] = { 0 };	// Buffer 1024, Last 0x00
	int nLen = m_Client.Read_Socket(byRecv);

	CString strRecvSocket, strLog;
	strRecvSocket.Format("%s", byRecv);
	m_strRecvCmd += strRecvSocket;

	while (!m_strRecvCmd.IsEmpty()) {
		int nStart = m_strRecvCmd.Find("@");
		int nEnd = m_strRecvCmd.Find("\n");

		if (nEnd < 0) break;	// 버퍼에 들어오는 중...

		if (nStart < 0 || nStart > nEnd) {
			strLog.Format("[<-] : <<Error>> %s : Start(%d), End(%d)", m_strRecvCmd, nStart, nEnd);
			g_objLogFile.Save_MesAgentLog(strLog);
			m_strRecvCmd.Delete(0, nEnd + 1);	// 쓰레기값이 채워져 있어서...
			continue;
		}

		CString strRecv = m_strRecvCmd.Mid(nStart + 1, nEnd - nStart - 1);
		m_strRecvCmd.Delete(0, nEnd + 1);

		// Inspector Log ////////////////////////////////////////////////////////////
		strLog.Format("[<-] : %s", strRecv);
		g_objLogFile.Save_MesAgentLog(strLog);
		/////////////////////////////////////////////////////////////////////////////

		char chSep = ',';
		CString strCmd, strOp;

		AfxExtractSubString(strCmd, strRecv, 0, chSep);
		AfxExtractSubString(strOp, strRecv, 1, chSep);

		CString strArg[5];
		for (int i = 0; i < 5; i++) AfxExtractSubString(strArg[i], strRecv, i + 2, chSep);

		if (strCmd == "CONTROL") 
		{
			if (strOp == "STATE") Get_ControlState(strArg[0]);

		} else if (strCmd == "LOT") 
		{
			if (strOp == "START")  Get_LotStart(strArg[0], strArg[1], strArg[2]);
			if (strOp == "CANCEL") Get_LotCancel(strArg[0], strArg[1],  strArg[2]);

		}
		else if (strCmd == "TIME") {
			if (strOp == "UPDATE") Get_TimeSync();

		} else if (strCmd == "RECIPE") {
			if (strOp == "REQUEST") Get_RecipeList(strArg[0]);

		} else if (strCmd == "CM") {
			if (strOp == "RESULT") Get_CmResult(strArg[0], strArg[1],  strArg[2], strArg[3], strArg[4]);
			if (strOp == "FAIL")   Get_CmFail(strArg[0], strArg[1],  strArg[2], strArg[3]);

		} else if (strCmd == "HOST") {
			if (strOp == "MESSAGE") Get_HostMessage(strArg[0]);

		} else if (strCmd == "NGLOT") {
			if (strOp == "START")  Get_NGLotStart(strArg[0], strArg[1]);
			if (strOp == "CANCEL") Get_NGLotCancel(strArg[0], strArg[1],  strArg[2]);

		}
	}

	return 0;
}

void CMesAgent::OnTimer(UINT_PTR nIDEvent)
{
	KillTimer(0);
	if (GetTickCount() - gMes.dwMesTime > MES_WAITTIME) {
		g_objCommon.Show_Error(9000);
		KillTimer(0);
		return;
	}
	SetTimer(0, 100, NULL);
	CWnd::OnTimer(nIDEvent);
}

///////////////////////////////////////////////////////////////////////////////
// Get Command

void CMesAgent::Get_ControlState(CString sFlag)
{
	int nOnline = atoi(sFlag);	// 1:Online, 2:Offline
// 	if (m_bHostOnline && nOnline == 2) g_objCommon.Show_Error(9006);	// Agent 에서 Offline 변경
	if (nOnline == 1 && m_bHostOnline == FALSE) Set_OperUpdate(gData.sOperID);
	m_bHostOnline = (nOnline == 1 ? TRUE : FALSE);
}

void CMesAgent::Get_LotStart(CString sLotId, CString sRecipe, CString sCmCount)
{
	int nPortNo = 99;
	int nCmCount = atoi(sCmCount);

	gMes.nHostRcvCmCount = nCmCount;
	gMes.sHostCancelLotId = sLotId;
	gMes.sHostCancelCode = sRecipe;
	gMes.sHostCancelText = sCmCount;

	for(int i=0; i<6; i++) {
		if (gLot.sLotID[i] == sLotId) { nPortNo = i; break; }
	}
	if (nPortNo > 90) { g_objCommon.Show_Error(9009); return; }

	gMes.sHostRecipe[nPortNo]  = sRecipe;
	gMes.nHostCmCount[nPortNo] = nCmCount;

	if (sRecipe.GetLength() < 1) { g_objCommon.Show_Error(9001); return; }
	EQUIP_DATA *pEquipData = g_objDataManager.Get_pEquipData();

	if(!pEquipData->bUseCntAutoSet)
	{
		if (gLot.nCmCount[nPortNo] != nCmCount) { g_objCommon.Show_Error(9010); return; }
	}

	if (Exist_Recipe(sRecipe) == FALSE) {
		g_objCommon.Show_Error(9007);	return;
	}
	
	if (sRecipe != pEquipData->sModelName) {
		if (pEquipData->bUseMESRcpCheck) {
			g_objCommon.Show_Error(9008);	return;
		}
		g_objCommon.Display_MESRecipe(sRecipe);
	}

	gMes.nLotStatus[nPortNo] = 2;
	if(pEquipData->bUseCntAutoSet) g_objCommon.Set_LotCount(nPortNo+1, sLotId, nCmCount);
}

void CMesAgent::Get_LotCancel(CString sLotId, CString sCode, CString sText)
{
	gMes.sHostCancelLotId = sLotId;
	gMes.sHostCancelCode = sCode;
	gMes.sHostCancelText = sText;

	int nPortNo = 99;
	for(int i=0; i<6; i++) {
		if (gLot.sLotID[i] == sLotId) { nPortNo = i; break; }
	}
	if (nPortNo > 10) {
		g_objCommon.Show_Error(9002);
		return;
	}

	gMes.nLotStatus[nPortNo] = 3;
	g_objCommon.Show_Error(9002);
}

void CMesAgent::Get_TimeSync()
{
	g_objInspector.Set_TimeUpdate(INSPECTOR_ALL);
}

void CMesAgent::Get_RecipeList(CString sFlag)		// 0:All, 1:Current Recipe
{
	int nType = atoi(sFlag);
	Set_RecipeList(nType);
}

void CMesAgent::Get_CmResult(CString sLotId, CString sCmId, CString sJudge, CString sNgCode, CString sNgText)
{
	CString	strLog;

	int nPortNo = 99;
	for(int i=0; i<6; i++) {
		if (gLot.sLotID[i] == sLotId) { nPortNo = i; break; }
	}
	if (nPortNo > 10) {
		strLog.Format("MESAgent ReciveData LotID error => (%s) (%s) (%s) (%s) (%s)", sLotId, sCmId, sJudge, sNgCode, sNgText);
		g_objLogFile.Save_MesAgentLog(strLog);
		return;
	}
	if (sCmId.GetLength() < 10) {
		strLog.Format("MESAgent ReciveData ModuleID error => (%s) (%s) (%s) (%s) (%s)", sLotId, sCmId, sJudge, sNgCode, sNgText);
		g_objLogFile.Save_MesAgentLog(strLog);
		return;
	}
	
	int nTray = -1, nCmNo = -1;
	for(int i = 0; i < 10; i++) {
		for (int j = 0; j < 40; j++) {
			if (gLot.sBarCode[nPortNo][i][j] == sCmId) {
				nTray = i, nCmNo = j;	break;
			}
		}
		if (nTray > -1) break;
	}
	if (nTray == -1 || nCmNo == -1) {
		strLog.Format("MESAgent ReciveData ModuleID Not Found => (%s) (%s) (%s) (%s) (%s)", sLotId, sCmId, sJudge, sNgCode, sNgText);
		g_objLogFile.Save_MesAgentLog(strLog);
		return;
	}

	if (sJudge == "NG") {
		if (sNgCode == "08") gMes.nCmResult[nPortNo][nTray][nCmNo] = 4;	//Marginal
		else				 gMes.nCmResult[nPortNo][nTray][nCmNo] = 2;	//Mes_NG
	} else {
		if (gMes.nCmResult[nPortNo][nTray][nCmNo] != 2) gMes.nCmResult[nPortNo][nTray][nCmNo] = 1;
	}

	DWORD dwTerm = GetTickCount() - m_dwReqStart[nPortNo][nTray][nCmNo];
	strLog.Format("[Get_CmResult] MES CM Result(%s) Judge(%s) Code(%s) Text(%s) PortNo(%d) TrayNo(%d) CmNo(%d) Time,%d", sCmId, sJudge, sNgCode, sNgText, nPortNo+1, nTray+1, nCmNo+1, dwTerm);
	g_objLogFile.Save_MesAgentLog(strLog);
}

void CMesAgent::Get_CmFail(CString sLotId, CString sCmId, CString sCode, CString sText)
{
	CString	strLog;
	gMes.sHostCancelLotId = sLotId;
	gMes.sHostCancelModule = sCmId;
	gMes.sHostCancelCode = sCode;
	gMes.sHostCancelText = sText;

	if (sCmId.GetLength() < 2) {
		strLog.Format("MESAgent CmFail ReciveData ModuleID error => (%s) (%s) (%s) (%s)", sLotId, sCmId, sCode, sText);
		g_objLogFile.Save_MesAgentLog(strLog);
		g_objCommon.Show_Error(9005);
		return;
	}

	int nPort = -1, nTray = -1, nCmNo = -1;
	for(int i=0; i<6; i++) {
		for(int j=0; j<10; j++) {
			for(int k=0; k<40; k++) {
				if (gLot.sBarCode[i][j][k] == sCmId) {
					nPort = i; nTray = j, nCmNo = k; break;
				}
			}
			if (nTray > -1) break;
		}
		if (nTray > -1) break;
	}
	if (nTray == -1 || nCmNo == -1) {
		strLog.Format("MESAgent CmFail ReciveData ModuleID Not Found => (%s) (%s) (%s) (%s)", sLotId, sCmId, sCode, sText);
		g_objLogFile.Save_MesAgentLog(strLog);
		g_objCommon.Show_Error(9005);
		return;
	}

	gMes.sHostCancelLotId = gLot.sLotID[nPort];
	gMes.nCmResult[nPort][nTray][nCmNo] = 2;

	g_objCommon.Show_Error(9005);

	strLog.Format("[Get_CmFail] MES CM Fail(%s) LotID(%s) Code(%s) Text(%s) PortNo(%d) TrayNo(%d) CmNo(%d)", sCmId, sLotId, sCode, sText, nPort+1, nTray+1, nCmNo+1);
	g_objLogFile.Save_MesAgentLog(strLog);
}

void CMesAgent::Get_HostMessage(CString sMsg)
{
	g_objCommon.Show_Alarm(sMsg, 1);
}

void CMesAgent::Get_NGLotStart(CString sLotId, CString sRecipe)
{
	gMes.sHostNGLotId  = sLotId;
	gMes.sHostNGRecipe = sRecipe;

	if (sLotId.GetLength() < 1) { g_objCommon.Show_Error(9021); return; }
	gMes.nMarStatus = 2;	//Start

	CString sLog;
	sLog.Format("NGLot => H<=M: NG_Lot_Start NGLot(%s) Recipe(%s)", gMes.sHostNGLotId, gMes.sHostNGRecipe);
	g_objLogFile.Save_MesAgentLog(sLog);
}

void CMesAgent::Get_NGLotCancel(CString sLotId, CString sCode, CString sText)
{
	gMes.sHostCancelNGLotId = sLotId;
	gMes.sHostCancelNGCode = sCode;
	gMes.sHostCancelNGText = sText;
	gMes.nMarStatus = 3;

	CString sLog;
	sLog.Format("NGLot => H<=M: NG_Lot_Fail NGLot(%s) Code(%s) Text(%s)", gMes.sHostCancelNGLotId, gMes.sHostCancelNGCode, gMes.sHostCancelNGText);
	g_objLogFile.Save_MesAgentLog(sLog);
}

///////////////////////////////////////////////////////////////////////////////
// Set Command

void CMesAgent::Set_EquipState(int nFlag)
{
	// MES : Init, idle, Setup, Ready, Executing(=Run), Paused(=Down)
	CString strSend;
	strSend.Format("EQUIP,STATE,%d", nFlag);	// 1:Init, 2:Idle, 3:Setup, 4:Ready, 5:Run(=Executing), 6;Pause(=Down)
	Send_Command(strSend);
}

void CMesAgent::Set_UnitState(int nState)
{
	// MES : Init, idle, Setup, Ready, Executing(=Run), Paused(=Down)
	CString strSend;
	strSend.Format("UNIT,STATE,%d", nState);	// 1:Init, 2:Idle, 3:Setup, 4:Ready, 5:Run(=Executing), 6;Pause(=Down)
	Send_Command(strSend);
}

void CMesAgent::Set_ErrorUpdate(int nFlag, CString sErrNo)
{
	CString strSend;
	strSend.Format("ERROR,UPDATE,%d,%s", nFlag, sErrNo);
	Send_Command(strSend);
}

void CMesAgent::Set_ControlState(int nFlag, CString sOperId)
{
	CString strSend;
	strSend.Format("CONTROL,STATE,%d,%s", nFlag, sOperId);
	Send_Command(strSend);
	if (nFlag == 2) m_bHostOnline = FALSE;	// 사용자 Offline
}

void CMesAgent::Set_LotStart(int nType, int nPortNo, CString sLotId, CString sRecipe, int nCount)	//0:Request, 1:Started
{
	if (nType == 0) gMes.nLotStatus[nPortNo] = 1;
	else			gMes.nLotStatus[nPortNo] = 4;
	CString strSend;
	strSend.Format("LOT,START,%d,%s,%s,%d", nType, sLotId, sRecipe, nCount);
	Send_Command(strSend);
}

void CMesAgent::Set_LotEnd(int nPortNo, CString sLotId, CString sRecipe, int nHCount, int nOk, int nNg)
{
	gMes.nLotStatus[nPortNo] = 0;
	CString strSend;
	strSend.Format("LOT,END,%s,%s,%d,%d,%d", sLotId, sRecipe, nHCount, nOk, nNg);
	Send_Command(strSend);
}

void CMesAgent::Set_LotAbort(CString sLotId)
{
	CString strSend;
	strSend.Format("LOT,ABORT,%s", sLotId);
	Send_Command(strSend);
}

void CMesAgent::Set_OperUpdate(CString sOperId)
{
	if (sOperId.GetLength() < 4) return;
	CString strSend;
	strSend.Format("OPER,UPDATE,%s", sOperId);
	Send_Command(strSend);
}

void CMesAgent::Set_IdleReport(CString sOperId, CString sSTime, CString sETime, CString sCode, CString sType)
{
	CString strSend;
	strSend.Format("IDLE,REPORT,%s,%s,%s,%s,%s", sOperId, sSTime, sETime, sCode, sType);
	Send_Command(strSend);
}

void CMesAgent::Set_ModeChanged(int nMode)
{
	CString strSend;
	strSend.Format("ACCESS,CHANGED,%d", nMode);
	Send_Command(strSend);
}

void CMesAgent::Set_RecipeList(int nFlag)						// 0:All, 1:Current Recipe
{
	CString strSend;

	if (nFlag == 1) {	// 1:Current Recipe
		EQUIP_DATA *pEquipData = g_objDataManager.Get_pEquipData();
		strSend.Format("RECIPE,REQUEST,1,1,%s", pEquipData->sModelName);
		Send_Command(strSend);
	}
	if (nFlag == 0) {	// 0:All Recipe
		int		nCount = 0;
		CString sPathSource, sRcipeData = "";

		sPathSource = gsCurrentDir + "\\System\\Model";

		if (GetFileAttributes(sPathSource) == -1) {	// 디렉토리 없음
			strSend.Format("RECIPE,REQUEST,0,%d,%s", nCount, sRcipeData);
			Send_Command(strSend);
			return;
		}

		CFileFind ff;
		BOOL bFile = ff.FindFile(sPathSource + _T("\\*.*"));

		while(bFile)
		{
			bFile = ff.FindNextFile();

			CString str;	// = ff.GetFileName();
			if(ff.IsDots()) continue;

			if(ff.IsDirectory()){
				str = ff.GetFileName();	nCount++;
				if (nCount == 1) sRcipeData = str;
				else			 sRcipeData = sRcipeData + "," + str;
			}
		}
		ff.Close();

		strSend.Format("RECIPE,REQUEST,0,%d,%s", nCount, sRcipeData);
		Send_Command(strSend);
	}
}

void CMesAgent::Set_CmRequest(CString sLotId, CString sCmId, int nPortNo, int nTrayNo, int nCmNo)
{
	CString strSend;
	strSend.Format("CM,REQUEST,%s,%s", sLotId, sCmId);
	Send_Command(strSend);

	m_dwReqStart[nPortNo-1][nTrayNo-1][nCmNo-1] = GetTickCount();
}

void CMesAgent::Set_CmEnd(int nType, int nPortNo, int nTrayNo, int nCmNo, int nOut, int nRosInfo, int nNGType )
{
	if (nPortNo < 1 || nPortNo > 6 || nCmNo < 1 || nCmNo > 40) return;

	CString	sLotID  = gLot.sLotID[nPortNo-1];
	CString	strCmId = gLot.sBarCode[nPortNo-1][nTrayNo-1][nCmNo-1];
	int nSpecialNG  = gLot.nSpecialNG[nPortNo-1][nTrayNo-1][nCmNo-1];

	CString strResult, strNgCode, strRosResult;
	if (nType == 1)
	{
//		if (nNGType == 4) { strResult = "MOK"; gMes.nNGCount[0]++; }
//		else			  { strResult = "NG";  gMes.nNGCount[1]++; }
//		strResult = "MOK"; gMes.nNGCount[0]++;
		if (nSpecialNG == 1) { strResult = "NG";  gMes.nNGCount[1]++; }
		else				 { strResult = "MOK"; gMes.nNGCount[0]++; }
		strNgCode = Set_NGSort(nPortNo, nTrayNo, nCmNo);
	}
	else
	{
		strResult = "OK";
		strNgCode = "00";
	}

	if(nRosInfo == 2)
	{
		strRosResult = "OK";
	}	
	else if(nRosInfo == 3 || nRosInfo == 4)
	{
		strRosResult = "NG";
	}	
	else
	{
		strRosResult = "Empty";
	}

	CString strSend;
	strSend.Format("CM,END,%s,%s,%s,%s,%s,%d", sLotID, strCmId, strResult, strNgCode, strRosResult, nOut);
	g_objLogFile.Save_TestLog(strSend);
	Send_Command(strSend);	
}

CString CMesAgent::Set_NGSort(int nPno, int nTNo, int CNo)
{
	CString sNGCode;

	sNGCode = gLot.sNGCode_I[nPno-1][nTNo-1][CNo-1][0];
	if (sNGCode == "BARCODE_NOREAD") return sNGCode;
	if (sNGCode == "MES_NG") return sNGCode;
	if (sNGCode == "MC") return sNGCode;
	if (sNGCode == "VISION-TIMEOVER") return sNGCode;
	if (sNGCode == "MARGINAL_NG") return sNGCode;

	int nSeq[10] = { 999, 999, 999, 999, 999, 999, 999, 999, 999, 999 };
	for (int i=1; i<6; i++) {
		if (gLot.sNGCode_I[nPno-1][nTNo-1][CNo-1][i] == "MC") {
			gLot.sNGCode_I[nPno-1][nTNo-1][CNo-1][0] = sNGCode = "MC"; return sNGCode;
		}

		for (int j=0; j<m_nSizeCount; j++) {
			if (m_sSizeNG[j].GetLength() > 0 && gLot.sNGCode_I[nPno-1][nTNo-1][CNo-1][i] == m_sSizeNG[j]) {
				nSeq[i-1] = j; break;
			}
		}
	}

	int nNo = 999, nI;
	for (int i=0; i<5; i++) {
		if (nNo > nSeq[i]) { nNo = nSeq[i]; nI = i; }
	}
	if (nNo < 100) { gLot.sNGCode_I[nPno-1][nTNo-1][CNo-1][0] = sNGCode = m_sSizeNG[nNo]; }
	if (sNGCode.GetLength() < 1) sNGCode = "NG";
	return sNGCode;
}

void CMesAgent::Set_TerminalOK()
{
	CString strSend;
	strSend.Format("TERMINAL,MSG");
	Send_Command(strSend);
}

void CMesAgent::Set_NGLotRequest()
{
	gMes.nMarStatus = 1;	//Request = 1;
	gMes.nNGCount[0] = gMes.nNGCount[1] = 0;

	CString strSend;
	strSend.Format("NGLOT,REQUEST");
	Send_Command(strSend);

	g_objLogFile.Save_MesAgentLog("NGLot => H=>M: NG_Lot_Request");
}

void CMesAgent::Set_NGLotEnd(CString sLotId, int nOk, int nNg)
{
	gMes.sGUItNGLotID = gMes.sHostNGLotId;
	gMes.nGUICount[0] = gMes.nNGCount[0];
	gMes.nGUICount[1] = gMes.nNGCount[1];
	gMes.nGUICount[2] = gMes.nMarTrayCount;

	gMes.nMarStatus = gMes.nMarTrayCount = gMes.nNGCount[0] = gMes.nNGCount[1] = 0;
	CString strSend;
	strSend.Format("NGLOT,END,%s,%d,%d", sLotId, nOk, nNg);
	Send_Command(strSend);

	CString sLog;
	sLog.Format("[MESAget NG_Lot_End] NGLot(%s) Tray(%d) MOK(%d) NG(%d)", gMes.sGUItNGLotID, gMes.nGUICount[2], gMes.nGUICount[0], gMes.nGUICount[1]);
	g_objLogFile.Save_HandlerLog(sLog);

	sLog.Format("NGLot => H=>M: NG_Lot_End NGLot(%s) Tray(%d) MOK(%d) NG(%d)", gMes.sGUItNGLotID, gMes.nGUICount[2], gMes.nGUICount[0], gMes.nGUICount[1]);
	g_objLogFile.Save_MesAgentLog(sLog);

	sLog.Format("\t%s \t%d \t%d \t%d \t%d", gMes.sGUItNGLotID, gMes.nGUICount[2], (gMes.nGUICount[0]+gMes.nGUICount[1]), gMes.nGUICount[0], gMes.nGUICount[1]);
	g_objLogFile.Save_NGLotLog(sLog);
}

///////////////////////////////////////////////////////////////////////////////

void CMesAgent::Send_Command(CString sSend)
{
	EQUIP_DATA *pEquipData = g_objDataManager.Get_pEquipData();
	if (!pEquipData->bUseMES) return;

	CString strSendSocket, strLog;

	if (!m_bConnected) return;
	if (sSend.Left(7) != "CONTROL" && sSend.Left(9) != "LOT,ABORT") {
		if (!m_bHostOnline) return;
	}

	strSendSocket.Format("@%s\n", sSend);

	char chSend[1001] = { 0 };	// Buffer 1000, Last 0x00
	int nLength = strSendSocket.GetLength();
	memcpy(chSend, (LPSTR)(LPCSTR)strSendSocket, nLength);

	if (!m_Client.Write_Socket((BYTE*)chSend, nLength)) return;

	// Host Log ////////////////////////////////////////////
	strLog.Format("[->] : %s", sSend);
	g_objLogFile.Save_MesAgentLog(strLog);
	///////////////////////////////////////////////////////
}

///////////////////////////////////////////////////////////////////////////////
BOOL CMesAgent::Exist_Recipe(CString sRecipe)
{
	CString sPathSource;

	sPathSource = gsCurrentDir + "\\System\\Model";
	if (GetFileAttributes(sPathSource) == -1) return FALSE;

	CFileFind ff;
	BOOL bFile = ff.FindFile(sPathSource + _T("\\*.*"));

	while(bFile)
	{
		bFile = ff.FindNextFile();
		if(ff.IsDots()) continue;

		if(ff.IsDirectory()) {
			if (sRecipe == ff.GetFileName()) { ff.Close(); return TRUE; }
		}
	}
	ff.Close();

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
/*/
#pragma region No Use Methods
// 안 쓰는 메서드

void CMesAgent::Set_LotReady(CString sLotId, int nPortIdx)
{
	CString strSend;
	EQUIP_DATA *pEquipData = g_objDataManager.Get_pEquipData();

	strSend.Format("LOT,READY,%s,%d", sLotId, nPortIdx);
	Send_Command(strSend);
}

void CMesAgent::Set_LotIdReport(CString sLotId, int nPortNo, CString sRecipeId)
{
	CString strSend;
	strSend.Format("LOT,REPORT,%s,%d,%s", sLotId, nPortNo, sRecipeId);
	Send_Command(strSend);
}

void CMesAgent::Set_IdleRequest()
{
	CString strSend;
	strSend.Format("IDLE,REQUEST");
	Send_Command(strSend);
}

void CMesAgent::Cancel_Data()
{
	g_objDataManager.Read_EquipData();
	g_objDataManager.Read_MoveData();
}
#pragma endregion Not_Use_Method
/*/