#include "stdafx.h"
#include "CMAI2100.h"
#include "OCAPManager.h"
#include "OCAPFAI.h"
#include "OCAPCosmetic.h"
#include "Common.h"
#include "LogFile.h"
#include "ErrorDlg.h"
#include "AlarmDlg.h"
#include <deque>
#include <afxmt.h>

namespace {
	CCriticalSection csLot;
	std::deque<OCAP_LOT_DATA> pendingLots;
	struct OCAP_ALARM_DATA { int nError; CString sLotID; CString sMessage; };
	std::deque<OCAP_ALARM_DATA> pendingAlarms;
	OCAP_ALARM_DATA activeAlarm;

	void QueueAlarm(int nError, CString sLotID, CString sName, double dRate,
		double dThreshold, int nCount, BOOL bFAI)
	{
		if (nError == 0) return;
		OCAP_ALARM_DATA alarm; alarm.nError = nError; alarm.sLotID = sLotID;
		BOOL bMeasureError = nError == 9184 || nError == 9188;
		CString sAction;
		if (bMeasureError) sAction = "설비 문제여부를 확인해야 합니다.";
		else {
			sAction = bFAI ? "DFA공정에 내용전달 해주세요." : "외관 공정에 내용전달 해주세요.";
			if (nError != 9181 && nError != 9185)
				sAction += " AVI Master Sample 투입하여 Correlation 이상여부를 확인해 주세요.";
		}
		alarm.sMessage.Format(" #==> [%s] %s [%0.3lf%%] 발생했습니다. (설정: %0.3lf%%). 연속 %d Lot 발생되었습니다.#      %s#      [Lot ID: %s]",
			sName, bMeasureError ? "측정 Error" : "불량률이", dRate, dThreshold, nCount, sAction, sLotID);
		pendingAlarms.push_back(alarm);
		CString sLog; sLog.Format("[OCAP Alarm Queued] Error(%d) Lot(%s) Name(%s) Rate(%0.3lf) Threshold(%0.3lf) ConsecutiveLot(%d)",
			nError, sLotID, sName, dRate, dThreshold, nCount); g_objLogFile.Save_HandlerLog(sLog);
	}
}

void OCAP_QueueLot(int nPortNo)
{
	if (nPortNo < 1 || nPortNo > 6) return;
	int nNo = nPortNo - 1;
	CSingleLock lock(&csLot, TRUE);
	if (gLot.bOcapFinalized[nNo] || gLot.sLotID[nNo].IsEmpty()) return;
	OCAP_LOT_DATA data;
	data.nPortNo = nPortNo; data.sLotID = gLot.sLotID[nNo];
	GetLocalTime(&data.timeCompleted);
	data.nTotal = gLot.nCmCount[nNo]; data.nGood = gLot.nGoodCount[nNo];
	for (int i=0; i<11; i++) data.nRosCount[i] = gLot.nRosJugCount[nNo][i];
	for (int j=0; j<10; j++) for (int k=0; k<40; k++) {
		data.nRosJudge[j][k] = gLot.nJudge_R[nNo][j][k];
		data.nSpecialNG[j][k] = gLot.nSpecialNG[nNo][j][k];
		for (int v=0; v<6; v++) {
			data.nJudge[j][k][v] = gLot.nJudge_I[nNo][j][k][v];
			data.sCode[j][k][v] = gLot.sNGCode_I[nNo][j][k][v];
		}
	}
	pendingLots.push_back(data);
	gLot.bOcapFinalized[nNo] = TRUE;
}

void OCAP_ProcessLots()
{
	for (;;) {
		OCAP_LOT_DATA data;
		{
			CSingleLock lock(&csLot, TRUE);
			if (pendingLots.empty()) break;
			data = pendingLots.front(); pendingLots.pop_front();
		}
		g_dlgOCAP_FAI.Set_AddLotData_FAI(data);
		int n = gCap_FAI.nAlmIdx_FAI;
		if (n >= 0 && n < 100) QueueAlarm(gCap_FAI.nPendingAlarm_FAI, data.sLotID,
			gCap_FAI.sAlmFAIName_FAI[n], gCap_FAI.dAlmDefect_FAI[n], gCap_FAI.dAlmThreshold_FAI[n], gCap_FAI.nAlmCount_FAI[n], TRUE);
		gCap_FAI.nPendingAlarm_FAI = 0;
		g_dlgOCAP_Cosmetic.Set_AddLotData_Cosmetic(data);
		n = gCap_Cosmetic.nAlmIdx_Cosmetic;
		if (n >= 0 && n < 100) QueueAlarm(gCap_Cosmetic.nPendingAlarm_Cosmetic, data.sLotID,
			gCap_Cosmetic.sAlmCosmeticName_Cosmetic[n], gCap_Cosmetic.dAlmDefect_Cosmetic[n], gCap_Cosmetic.dAlmThreshold_Cosmetic[n], gCap_Cosmetic.nAlmCount_Cosmetic[n], FALSE);
		gCap_Cosmetic.nPendingAlarm_Cosmetic = 0;
		CString sLog; sLog.Format("[OCAP Lot Complete] Port(%d) Lot(%s) Total(%d) Good(%d)", data.nPortNo, data.sLotID, data.nTotal, data.nGood);
		g_objLogFile.Save_HandlerLog(sLog);
	}
}

void OCAP_ShowNextAlarm()
{
	if (pendingAlarms.empty() || gData.bAlarmShow || g_dlgError.IsWindowVisible() || g_dlgAlarm.IsWindowVisible()) return;
	activeAlarm = pendingAlarms.front(); pendingAlarms.pop_front();
	// Show synchronously on the owning UI thread; avoid Show_Error's worker-thread race.
	g_dlgError.Set_ErrorNumber(activeAlarm.nError);
	g_dlgError.ShowWindow(SW_SHOW);
}

CString OCAP_GetAlarmMessage() { return activeAlarm.sMessage; }
CString OCAP_GetAlarmLotID() { return activeAlarm.sLotID; }
