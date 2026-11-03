"""Compile actual OCAP rule/history methods with small host adapters, using C++03.
This does not build MFC, the resources, or hardware drivers.
Run: python3 OCAP_Review/tests/test_ocap.py (requires g++).
"""
from pathlib import Path
import re, subprocess, tempfile, os
ROOT=Path(__file__).resolve().parents[2]
def read(name):
 b=(ROOT/name).read_bytes();return b.decode('utf-16' if b[:2]==b'\xff\xfe' else 'cp949').replace('\r\n','\n')
def function(text,signature):
 a=text.index(signature);b=text.index('{',a);i=b+1;d=1
 while d:
  if text[i]=='{':d+=1
  if text[i]=='}':d-=1
  i+=1
 return text[a:i]
pre=r'''
#include <string>
#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdarg>
#include <cstdio>
typedef int BOOL;
#define TRUE 1
#define FALSE 0
class CString : public std::string {
public:
 CString(const char* s=""):std::string(s){}
 CString(const std::string& s):std::string(s){}
 bool IsEmpty()const{return empty();}
 int GetLength()const{return (int)size();}
 void Format(const char* fmt, ...) { assign(fmt); }
};
struct SYSTEMTIME {int wMonth,wDay,wHour,wMinute;};
void GetLocalTime(SYSTEMTIME* p){p->wMonth=11;p->wDay=2;p->wHour=12;p->wMinute=0;}
struct Log {void Save_HandlerLog(CString){} void Save_OCAPDataLog_FAI(int){} void Save_OCAPDataLog_Cosmetic(int){} } g_objLogFile;
'''
s=read('stdafx.h');a=s.index('typedef struct {',s.index('struct eAccessMode'));b=s.index('extern GLOVAL_OCAP_FAI',a);pre+=s[a:b]
pre+='\nGLOVAL_OCAP_FAI gCap_FAI; GLOVAL_OCAP_Cosmetic gCap_Cosmetic;\n'
h=read('OCAPManager.h');pre+=h[h.index('struct OCAP_LOT_DATA'):h.index('void OCAP_QueueLot')]
pre+='#include "OCAPRules.h"\n'
for k in ['FAI','Cosmetic']:
 pre+='class OCAP'+k+' {public: BOOL m_bError_'+k+';\n'
 for sig in ['void Set_AddLotData_'+k+'(const OCAP_LOT_DATA& data);','void AddTrayToLot_'+k+'(const OCAP_LOT_DATA& data,int nIdx);','void DelLotData_'+k+'(int nLotNo);','void AddLotOut_'+k+'(CString sLotid, const SYSTEMTIME& time);','void Check_DEFECT_'+k+'(int nNo);','void Check_DEFECTF_'+k+'(int nNo);']:
  pre+=sig+'\n'
 pre+='};\n'
 c=read('OCAP'+k+'.cpp')
 for f in ['Set_AddLotData','AddTrayToLot','DelLotData','AddLotOut','Check_DEFECT','Check_DEFECTF']:
  pre+=function(c,'void OCAP'+k+'::'+f+'_'+k+'(')+'\n'
pre += r'''
#include <deque>
struct CCriticalSection {};
struct CSingleLock { CSingleLock(CCriticalSection*, BOOL){} };
struct FakeData {bool bAlarmShow;} gData;
struct FakeError {bool visible;int number;bool IsWindowVisible(){return visible;}void Set_ErrorNumber(int n){number=n;}void ShowWindow(int){visible=true;}} g_dlgError;
struct FakeAlarm {bool IsWindowVisible(){return false;}} g_dlgAlarm;
#define SW_SHOW 1
struct FakeLot {
 bool bOcapFinalized[6];CString sLotID[6];int nCmCount[6];int nGoodCount[6];int nRosJugCount[6][11];
 int nJudge_I[6][10][40][6];int nJudge_R[6][10][40];int nSpecialNG[6][10][40];CString sNGCode_I[6][10][40][6];
} gLot;
OCAPFAI g_dlgOCAP_FAI; OCAPCosmetic g_dlgOCAP_Cosmetic;
'''
pre += re.sub(r'^#include.*$', '', read('OCAPManager.cpp'), flags=re.M)
main=r'''
int main(){
 OCAP_LOT_DATA d; memset(d.nJudge,0,sizeof(d.nJudge));memset(d.nRosJudge,0,sizeof(d.nRosJudge));memset(d.nSpecialNG,0,sizeof(d.nSpecialNG));memset(d.nRosCount,0,sizeof(d.nRosCount));
 d.sLotID="LOT";d.nTotal=100;d.nGood=90;GetLocalTime(&d.timeCompleted);
 // Every camera-local NG value is categorized using F2's actual wire mapping.
 for(int j=2;j<=9;j++){assert(OCAP_IsVisionNG(true,j)==(j>=7));assert(OCAP_IsVisionNG(false,j)==(j>=3&&j<=6));}
 CString codes[100];codes[0]="A";codes[1]="FA";bool e=false;
 assert(!OCAP_MatchRegisteredCode(CString("FA"),codes,0,CString("F"),e));assert(!e);
 assert(OCAP_MatchRegisteredCode(CString("FA"),codes,1,CString("F"),e));assert(!e);
 codes[1]="";assert(OCAP_MatchRegisteredCode(CString("FA"),codes,0,CString("F"),e)&&e);
 // First and tenth tray, first and last pocket; deduplicate across five cameras.
 d.nJudge[0][0][0]=3;d.nJudge[9][39][0]=4;
 for(int v=1;v<=5;v++){d.nJudge[0][0][v]=3;d.sCode[0][0][v]="A";d.nJudge[9][39][v]=6;d.sCode[9][39][v]="FA";}
 int n=0,ne=0;OCAP_CountCode(false,d,codes,0,CString("F"),n,ne);assert(n==2&&ne==1);
 d.nRosJudge[9][39]=2;OCAP_CountCode(false,d,codes,0,CString("F"),n,ne);assert(n==1&&ne==0);
 d.nSpecialNG[0][0]=1;OCAP_CountCode(false,d,codes,0,CString("F"),n,ne);assert(n==0);
 d.nSpecialNG[0][0]=0;
 // All dimension classes S/T/W and F-prefixed errors, across distinct trays.
 for(int j=1;j<=3;j++){d.nJudge[j][0][0]=6+j;d.nJudge[j][0][1]=6+j;d.sCode[j][0][1]=(j==3?"FA":"A");}
 OCAP_CountCode(true,d,codes,0,CString("F"),n,ne);assert(n==3&&ne==1);
 assert(OCAP_IsEligibleRate(100,1,100,1.0));assert(!OCAP_IsEligibleRate(100,1,101,1.0));
 assert(!OCAP_IsEligibleRate(0,0,0,1.0));assert(!OCAP_IsEligibleRate(100,0,1,1.0));
 assert(!OCAP_IsEligibleRate(100,1,1,1.001));assert(!OCAP_IsEligibleRate(100,1,1,0.0));
 OCAPFAI fai; OCAPCosmetic cos;
 // Run the actual class methods: per-code overrides, thresholds and severity order.
 gCap_FAI.sFAICode_FAI[0]="A";gCap_FAI.sFAIName_FAI[0]="Dimension";
 gCap_FAI.nCodeLotCnt_FAI[0]=-1;gCap_FAI.dCodeDefect_FAI[0]=-1;gCap_FAI.nGiMinCnt_FAI=1;
 for(int i=0;i<4;i++){gCap_FAI.nGiLotCnt_FAI[i]=2;gCap_FAI.dGiDefect_FAI[i]=3;}
 fai.Set_AddLotData_FAI(d);assert(gCap_FAI.nPendingAlarm_FAI==0);
 fai.Set_AddLotData_FAI(d);assert(gCap_FAI.nPendingAlarm_FAI==9183);assert(gCap_FAI.dAlmDefect_FAI[0]==3.0);
 // Clear pending as UI queue does; clean lot interrupts the consecutive sequence.
 gCap_FAI.nPendingAlarm_FAI=0;OCAP_LOT_DATA good=d;memset(good.nJudge,0,sizeof(good.nJudge));
 fai.Set_AddLotData_FAI(good);assert(gCap_FAI.nPendingAlarm_FAI==0);
 fai.Set_AddLotData_FAI(d);assert(gCap_FAI.nPendingAlarm_FAI==0);
 gCap_FAI.nCodeLotCnt_FAI[0]=0;fai.Set_AddLotData_FAI(d);assert(gCap_FAI.nPendingAlarm_FAI==0);
 gCap_FAI.nCodeLotCnt_FAI[0]=1;gCap_FAI.dCodeDefect_FAI[0]=3.001;fai.Set_AddLotData_FAI(d);assert(gCap_FAI.nPendingAlarm_FAI==0);
 // Error-only criteria when ordinary criteria disabled.
 gCap_FAI.nCodeLotCnt_FAI[0]=-1;gCap_FAI.dCodeDefect_FAI[0]=-1;
 for(int i=0;i<3;i++)gCap_FAI.nGiLotCnt_FAI[i]=0;
 gCap_FAI.nGiLotCnt_FAI[3]=1;gCap_FAI.dGiDefect_FAI[3]=1;
 fai.Set_AddLotData_FAI(d);assert(gCap_FAI.nPendingAlarm_FAI==9184);gCap_FAI.nPendingAlarm_FAI=0;
 // 50-row ring wraps repeatedly and stores the latest complete lot as one row.
 for(int i=0;i<105;i++){fai.Set_AddLotData_FAI(d);gCap_FAI.nPendingAlarm_FAI=0;}
 assert(gCap_FAI.nLotCycle_FAI>=0&&gCap_FAI.nLotCycle_FAI<50);
 int ix=(gCap_FAI.nLotCycle_FAI+49)%50;assert(gCap_FAI.nCount_FAI[ix][0]==3&&gCap_FAI.nTotCount_FAI[ix]==100);
 gCap_Cosmetic.sCosmeticCode_Cosmetic[0]="A";gCap_Cosmetic.sCosmeticName_Cosmetic[0]="Surface";
 gCap_Cosmetic.nCodeLotCnt_Cosmetic[0]=1;gCap_Cosmetic.dCodeDefect_Cosmetic[0]=1;gCap_Cosmetic.nGiMinCnt_Cosmetic=1;
 cos.Set_AddLotData_Cosmetic(d);assert(gCap_Cosmetic.nPendingAlarm_Cosmetic==9187);

 // Real manager methods: copy before source clear, no double queue, two independent alarms.
 gCap_FAI=GLOVAL_OCAP_FAI();gCap_Cosmetic=GLOVAL_OCAP_Cosmetic();
 gCap_FAI.sFAICode_FAI[0]="A";gCap_FAI.sFAIName_FAI[0]="Dimension";gCap_FAI.nCodeLotCnt_FAI[0]=1;gCap_FAI.dCodeDefect_FAI[0]=1;gCap_FAI.nGiMinCnt_FAI=1;
 gCap_Cosmetic.sCosmeticCode_Cosmetic[0]="B";gCap_Cosmetic.sCosmeticName_Cosmetic[0]="Surface";gCap_Cosmetic.nCodeLotCnt_Cosmetic[0]=1;gCap_Cosmetic.dCodeDefect_Cosmetic[0]=1;gCap_Cosmetic.nGiMinCnt_Cosmetic=1;
 gLot.sLotID[0]="SNAP-A";gLot.nCmCount[0]=100;gLot.nJudge_I[0][0][0][0]=7;gLot.nJudge_I[0][0][0][1]=7;gLot.sNGCode_I[0][0][0][1]="A";
 OCAP_QueueLot(1);OCAP_QueueLot(1);
 gLot.sLotID[0]="CLEARED";gLot.nCmCount[0]=0;gLot.sNGCode_I[0][0][0][1]="";
 gLot.sLotID[1]="SNAP-B";gLot.nCmCount[1]=100;gLot.nJudge_I[1][0][0][0]=3;gLot.nJudge_I[1][0][0][1]=3;gLot.sNGCode_I[1][0][0][1]="B";
 OCAP_QueueLot(2);OCAP_ProcessLots();
 assert(gCap_FAI.nLotCycle_FAI==2);assert(gCap_FAI.nTotCount_FAI[0]==100);assert(gCap_FAI.nCount_FAI[0][0]==1);assert(gCap_FAI.sLotID_FAI[0]=="SNAP-A");
 OCAP_ShowNextAlarm();assert(g_dlgError.number==9183);assert(OCAP_GetAlarmLotID()=="SNAP-A");
 OCAP_ShowNextAlarm();assert(OCAP_GetAlarmLotID()=="SNAP-A");
 g_dlgError.visible=false;gData.bAlarmShow=true;OCAP_ShowNextAlarm();assert(OCAP_GetAlarmLotID()=="SNAP-A");
 gData.bAlarmShow=false;OCAP_ShowNextAlarm();assert(g_dlgError.number==9187);assert(OCAP_GetAlarmLotID()=="SNAP-B");
 g_dlgError.visible=false;OCAP_ShowNextAlarm();assert(OCAP_GetAlarmLotID()=="SNAP-B");
 std::cout<<"PASS: F2 mapping, 10 trays, camera deduplication, exact/F+ code precedence, ROS Good exclusion, special NG exclusion, minimum sample, threshold equality, class aggregation, consecutive reset, overrides, measurement error, ring wrap, snapshot isolation, duplicate queue guard and alarm serialization\n";
}
'''
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'test.cpp';p.write_text(pre+main)
 subprocess.run(['g++','-std=c++03','-Wall','-Wextra','-Werror','-Wno-format','-fsanitize=address,undefined','-I',str(ROOT),str(p),'-o',str(Path(td)/'test')],check=True)
 subprocess.run([str(Path(td)/'test')],check=True,env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
