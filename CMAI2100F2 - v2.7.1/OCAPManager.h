#pragma once

// Captured before UM_LOT_END_MSG clears/reuses the source port.
struct OCAP_LOT_DATA {
	CString sLotID;
	int nPortNo;
	SYSTEMTIME timeCompleted;
	int nTotal;
	int nGood;
	int nRosCount[11];
	int nJudge[10][40][6];
	int nRosJudge[10][40];
	int nSpecialNG[10][40];
	CString sCode[10][40][6];
};

void OCAP_QueueLot(int nPortNo); // sequence thread: copy only, no UI access
void OCAP_ProcessLots();        // main UI timer: aggregate/history/log/rules
void OCAP_ShowNextAlarm();      // main UI timer: one alarm at a time
CString OCAP_GetAlarmMessage();
CString OCAP_GetAlarmLotID();
