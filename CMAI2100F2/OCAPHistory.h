#pragma once
#include <vector>
#include <algorithm>

// Shared CSV reader for FAI / Cosmetic. Never modifies CSV files or alarm data.
static bool OCAPHistory_Split(const CString& line, std::vector<CString>& cells)
{
 cells.clear();
 if (cells.capacity()<410) cells.reserve(410);
 const int length=line.GetLength();
 int start=0;
 while (start<=length) {
  int end=start;
  CString cell;
  if (start<length && line[start]=='"') {
   ++end; int span=end; bool closed=false;
   while (end<length) {
    if (line[end]=='"') {
     cell+=line.Mid(span,end-span);
     if (end+1<length && line[end+1]=='"') { cell+='"'; end+=2; span=end; }
     else { ++end; closed=true; break; }
    } else ++end;
   }
   if (!closed || (end<length && line[end]!=',')) return false;
  } else {
   while (end<length && line[end]!=',') {
    if (line[end]=='"') return false;
    ++end;
   }
   cell=line.Mid(start,end-start);
  }
  cells.push_back(cell);
  if (cells.size()>410) return false;
  if (end==length) break;
  start=end+1;
 }
 return true;
}

// Bound allocations before parsing a physical CSV line. A truncated/oversized
// line invalidates the remainder of that file; never keep appending fragments.
static bool OCAPHistory_ReadLine(CStdioFile& file, CString& line)
{
 TCHAR buffer[32768];
 if (!file.ReadString(buffer, _countof(buffer))) return false;
 line = buffer;
 if (line.Find('\n') < 0) return false; // incomplete final record or oversized line
 line.TrimRight("\r\n");
 return true;
}

static bool OCAPHistory_Number(const CString& value, bool rate)
{
 if (value.IsEmpty() || value.GetLength() > 10) return false;
 bool dot = false; int digits = 0;
 for (int i=0; i<value.GetLength(); ++i) {
  TCHAR ch = value[i];
  if (rate && ch == '.' && !dot && digits > 0) { dot = true; continue; }
  if (ch < '0' || ch > '9') return false;
  ++digits;
 }
 if (!digits || value[value.GetLength()-1] == '.') return false;
 double number = _tstof(value);
 return number <= (rate ? 100.0 : 2147483647.0);
}

struct OCAPHistory_Row
{
	CString sKey;
	std::vector<CString> cells;
};
static bool OCAPHistory_Newer(const OCAPHistory_Row& a, const OCAPHistory_Row& b)
{
	return a.sKey.Compare(b.sKey) > 0;
}

// Sort pointers, then swap cell vectors once per record. Sorting rows directly
// in C++03 repeatedly deep-copies hundreds of CString cells per comparison/move.
static bool OCAPHistory_RowPointerNewer(const OCAPHistory_Row* a, const OCAPHistory_Row* b)
{ return OCAPHistory_Newer(*a,*b); }
static void OCAPHistory_Sort(std::vector<OCAPHistory_Row>& rows)
{
 std::vector<OCAPHistory_Row*> order;
 order.reserve(rows.size());
 for (size_t i=0;i<rows.size();++i) order.push_back(&rows[i]);
 std::stable_sort(order.begin(),order.end(),OCAPHistory_RowPointerNewer);
 std::vector<OCAPHistory_Row> sorted(rows.size());
 for (size_t i=0;i<order.size();++i) {
  sorted[i].sKey=order[i]->sKey;
  sorted[i].cells.swap(order[i]->cells);
 }
 rows.swap(sorted);
}

// Same rate highlighting as the original in-memory history display.
// These colors indicate threshold exceedance, not a saved alarm-event flag.
static COLORREF OCAPHistory_Color(int nTotal, int nCount, int nMinimum,
	double dIndividual, const double* pCommon, int nType)
{
	if (nTotal <= 0 || nTotal < nMinimum || nCount <= 0 || dIndividual == 0.0)
		return RGB(255, 255, 255);
	double dRate = nCount * 100.0 / nTotal;
	int nFirst = nType == 0 ? 2 : 3;
	int nLast = nType == 0 ? 0 : 3;
	for (int i=nFirst; i>=nLast; i--) {
		double dLimit = dIndividual == -1.0 ? pCommon[i] : dIndividual;
		if (dRate >= dLimit) {
			if (i >= 2) return RGB(255, 0, 0);
			if (i == 1) return RGB(0, 0, 255);
			return RGB(0, 255, 0);
		}
	}
	return RGB(255, 255, 255);
}

// A job owns immutable snapshots; worker threads never access dialogs or grids.
struct OCAPHistory_Job
{
 LONG refs;
 HANDLE done, cancel;
 CString folder, type, codes[100];
 double defect[100], common[4];
 int minimum;
 bool failed;
 std::vector<OCAPHistory_Row> rows;
 OCAPHistory_Job() : refs(1), done(CreateEvent(NULL, TRUE, FALSE, NULL)),
     cancel(CreateEvent(NULL, TRUE, FALSE, NULL)), minimum(0), failed(false) {}
 ~OCAPHistory_Job() { if (done) CloseHandle(done); if (cancel) CloseHandle(cancel); }
 bool Cancelled() const { return WaitForSingleObject(cancel, 0) == WAIT_OBJECT_0; }
 void Release() { if (InterlockedDecrement(&refs)==0) delete this; }
};
static bool OCAPHistory_FileNewer(const CString& a, const CString& b) { return a.Compare(b)>0; }

static void OCAPHistory_Load(OCAPHistory_Job* job)
{
	const CString& sFolder = job->folder;
	const CString& sType = job->type;
	CString* pCodes = job->codes;
	int nBase = sType == "Cosmetic" ? 13 : 16;
	// Three calendar months, inclusive of the cutoff date; clamp month-end days.
	SYSTEMTIME now;
	GetLocalTime(&now);
	int nYear = now.wYear, nMonth = now.wMonth - 3;
	if (nMonth <= 0) { nMonth += 12; nYear--; }
	int nDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (nYear%4 == 0 && (nYear%100 != 0 || nYear%400 == 0)) nDays[1] = 29;
	int nDay = now.wDay < nDays[nMonth-1] ? now.wDay : nDays[nMonth-1];
	CString sCutoff, sToday;
	sCutoff.Format("%04d%02d%02d", nYear, nMonth, nDay);
	sToday.Format("%04d%02d%02d", now.wYear, now.wMonth, now.wDay);

	std::vector<OCAPHistory_Row>& rows = job->rows;
	CFileFind finder;
    std::vector<CString> files;
    BOOL bMore = finder.FindFile(sFolder + "\\*.csv");
    while (bMore && !job->Cancelled()) {
        bMore = finder.FindNextFile();
        if (!finder.IsDirectory()) files.push_back(finder.GetFileName());
    }
    finder.Close();
    std::sort(files.begin(), files.end(), OCAPHistory_FileNewer);
    for (size_t f=0; f<files.size() && !job->Cancelled(); ++f) {
        CString sName = files[f];
        CString sPrefix = "_OCAP" + sType + "CodeV2_";
        int nHash = 8 + sPrefix.GetLength();
        if (sName.GetLength() != nHash+12 || sName.Mid(8, sPrefix.GetLength()) != sPrefix ||
            sName.Right(4).CompareNoCase(".csv") != 0) continue;
        bool bHash = true;
        for (int h=nHash; h<nHash+8; ++h)
            if (!((sName[h]>='0' && sName[h]<='9') || (sName[h]>='A' && sName[h]<='F'))) bHash = false;
        if (!bHash) continue;
        CString sDate = sName.Left(8);
		BOOL bDate = sDate.GetLength() == 8;
		for (int k=0; k<sDate.GetLength(); k++) if (sDate[k] < '0' || sDate[k] > '9') bDate = FALSE;
		if (!bDate) continue;
        int year = _ttoi(sDate.Left(4)), month = _ttoi(sDate.Mid(4,2)), day = _ttoi(sDate.Right(2));
        int days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if (year%4 == 0 && (year%100 != 0 || year%400 == 0)) days[1] = 29;
        if (month<1 || month>12 || day<1 || day>days[month-1]) continue;
		if (sDate.Compare(sCutoff) < 0 || sDate.Compare(sToday) > 0) continue;
        // Filenames are descending by date. Once full, older days cannot enter
        // the newest 5000; still merge ALL files belonging to the cutoff day.
        if (rows.size()>=5000 && sDate.Compare(rows.back().sKey.Left(8))<0) break;
		CStdioFile file;
		try 
		{
            if (!file.Open(sFolder + "\\" + sName, CFile::modeRead | CFile::typeText | CFile::shareDenyNone)) continue;
            // Avoid blocking the UI on unrelated or corrupted large files.
            if (file.GetLength() > 16*1024*1024) { file.Close(); continue; }
			CString sLine;
			std::vector<CString> header, data;
			if (!OCAPHistory_ReadLine(file, sLine)) { file.Close(); continue; }
            if (!OCAPHistory_Split(sLine, header) || header.size() != 410 || !header[409].IsEmpty()) { file.Close(); continue; }
            const TCHAR* names[] = { _T("Time"), _T("LotID"), _T("Total"), _T("Good"), _T("ROS_NG"), _T("ROS_Repair"), _T("BCR"), _T("MES"), _T("MC") };
            bool validHeader = true;
            for (int h=0; h<9; ++h) if (header[h] != names[h]) validHeader = false;
            for (int h=9; h<409; h+=4) {
                CString code = header[h];
                if (header[h+1] != code + " Rate (%)" || header[h+2] != "ERR " + code ||
                    header[h+3] != "ERR " + code + " Rate (%)") validHeader = false;
            }
            if (!validHeader) { file.Close(); continue; }
            			int nMap[400];
			for (int j=0; j<400; j++) nMap[j] = -1;
			for (int j=9; j<409; j+=4) {
				CString sTitle = header[j];
				CString sRecorded = sTitle;
				for (int k=0; k<100; k++) {
					if (!pCodes[k].IsEmpty() && sRecorded == pCodes[k]) {
						nMap[k*2] = j; nMap[k*2+1] = j+1;
						nMap[200+k*2] = j+2; nMap[200+k*2+1] = j+3;
						break;
					}
				}
			}
			std::vector<OCAPHistory_Row> fileRows;
			for (int nRead=0; nRead<100000 && !job->Cancelled() && OCAPHistory_ReadLine(file, sLine); ++nRead) 
			{
				if (!OCAPHistory_Split(sLine, data)) continue;
				// Ignore a partial final record while the logger is appending it.
				if (data.size() != header.size() || !data[409].IsEmpty()) continue;
                CString time = data[0];
                if (time.GetLength()!=8 || time[2]!=':' || time[5]!=':') continue;
                bool validRow = true;
                for (int t=0; t<8; ++t) if (t!=2 && t!=5 && (time[t]<'0' || time[t]>'9')) validRow = false;
                if (_ttoi(time.Left(2))>23 || _ttoi(time.Mid(3,2))>59 || _ttoi(time.Right(2))>59) validRow = false;
                if (data[1].IsEmpty()) validRow = false;
                for (int c=2; c<409; ++c)
                    if (!OCAPHistory_Number(data[c], c>=9 && ((c-9)%4==1 || (c-9)%4==3))) validRow = false;
                if (!validRow) continue;
				OCAPHistory_Row row;
				row.sKey = sDate + data[0];
				row.cells.resize(nBase+400);
				row.cells[0] = sDate.Left(4) + "/" + sDate.Mid(4,2) + "/" + sDate.Mid(6,2);
				row.cells[1] = data[0]; row.cells[2] = data[1];
				row.cells[3] = data[2];
				row.cells[4].Format("%I64d", (__int64)_ttoi(data[3]) + _ttoi(data[4]) + _ttoi(data[5]));
				row.cells[5] = data[3]; row.cells[6] = data[4]; row.cells[7] = data[5];
				row.cells[8].Format("%I64d", (__int64)_ttoi(data[6]) + _ttoi(data[7]) + _ttoi(data[8]));
				row.cells[9] = data[6]; row.cells[10] = data[7]; row.cells[11] = data[8];
				__int64 nTotal = 0, nGroup[3] = { 0, 0, 0 };
				for (int k=0; k<100; k++) {
					int nCount = _ttoi(data[9+k*4]);
					nTotal += nCount;
					if (k<7) nGroup[0] += nCount;
					else if (k<15) nGroup[1] += nCount;
					else if (k<20) nGroup[2] += nCount;
				}
				row.cells[12].Format("%I64d", nTotal);
				if (nBase == 16) for (int k=0; k<3; k++) row.cells[13+k].Format("%I64d", nGroup[k]);
				for (int j=0; j<400; j++) {
					if (nMap[j] >= 0 && nMap[j] < (int)data.size()) {
						row.cells[j+nBase] = data[nMap[j]];
						if (j%2 == 1) row.cells[j+nBase] += "%";
					}
				}
				fileRows.push_back(OCAPHistory_Row());
                fileRows.back().sKey = row.sKey;
                fileRows.back().cells.swap(row.cells);
                if (fileRows.size()>=5500) {
                    OCAPHistory_Sort(fileRows);
                    fileRows.resize(5000);
                }
			}
            OCAPHistory_Sort(fileRows);
            for (size_t r=0; r<fileRows.size(); ++r) {
                rows.push_back(OCAPHistory_Row());
                rows.back().sKey=fileRows[r].sKey;
                rows.back().cells.swap(fileRows[r].cells);
            }
            OCAPHistory_Sort(rows);
            if (rows.size()>5000) rows.resize(5000);
			file.Close();
		} catch (CFileException* pEx) { pEx->Delete(); file.Abort(); }
	}
	finder.Close();
	OCAPHistory_Sort(rows);
}

static void OCAPHistory_Render(CGridCS* pGrid, OCAPHistory_Job* job, int first)
{
 const std::vector<OCAPHistory_Row>& rows = job->rows;
 CString* pCodes = job->codes;
 const double* pCodeDefect = job->defect;
 const double* pCommon = job->common;
 int nMinimum = job->minimum;
 int nBase = job->type == "Cosmetic" ? 13 : 16;
	// WM_SETREDRAW(TRUE) adds WS_VISIBLE even to a hidden window.
	// Only suspend redraw when the grid is already intended to be visible.
	BOOL bVisible = (pGrid->GetStyle() & WS_VISIBLE) != 0;
	if (bVisible) pGrid->SetRedraw(FALSE);
	// Reset the previous display so expired rows cannot remain visible.

	int nCount = (int)rows.size() - first;
	if (nCount>100) nCount=100;
	if (nCount<0) nCount=0;
	int nRows = nCount+1;
	if (nRows < 51) nRows = 51;
	pGrid->Set_RowCount(nRows);
	pGrid->Set_ColWidth(0, 100);
	pGrid->Set_ColWidth(1, 105);
	for (int i=1; i<nRows; i++) {
		pGrid->Set_RowHeight(i, 26);
		for (int j=0; j<nBase+400; j++) {
			CString sText;
			if (i <= nCount) sText = rows[first+i-1].cells[j];
			if (pGrid->Get_CellText(i,j) != sText) pGrid->Set_CellText(i, j, sText);
			if (j >= nBase) {
				COLORREF crBack = RGB(255, 255, 255);
				if (i <= nCount) {
					const OCAPHistory_Row& row = rows[first+i-1];
					int nCode = ((j-nBase) % 200) / 2;
					int nCountCol = nBase + ((j-nBase) / 2) * 2;
					if (!row.cells[nCountCol].IsEmpty() && !pCodes[nCode].IsEmpty())
						crBack = OCAPHistory_Color(_ttoi(row.cells[3]), _ttoi(row.cells[nCountCol]),
							nMinimum, pCodeDefect[nCode], pCommon, (j-nBase) / 200);
				}
				if (pGrid->Get_CellBackClr(i,j) != crBack) pGrid->Set_CellBackClr(i, j, crBack);
				// White text remains legible on the original blue highlight.
				COLORREF textColor = crBack == RGB(0, 0, 255) ? RGB(255, 255, 255) : RGB(0, 0, 0);
                if (pGrid->Get_CellTextClr(i,j) != textColor) pGrid->Set_CellTextClr(i, j, textColor);
			}
		}
	}
	if (bVisible) {
		pGrid->SetRedraw(TRUE);
		pGrid->Invalidate(FALSE);
	}
}
static UINT OCAPHistory_Worker(LPVOID param)
{
 OCAPHistory_Job* job = (OCAPHistory_Job*)param;
 try { OCAPHistory_Load(job); }
 catch (CException* ex) { ex->Delete(); job->failed=true; job->rows.clear(); }
 catch (...) { job->failed=true; job->rows.clear(); }
 SetEvent(job->done);
 job->Release();
 return 0;
}


// This small child toolbar owns jobs. Closing the dialog cancels without waiting
// for disk IO; worker-owned references keep job memory alive until it exits.
class OCAPHistory_View : public CWnd
{
 CGridCS* grid;
 HWND gridWindow;
 OCAPHistory_Job* pending;
 OCAPHistory_Job* current;
 int page;
 bool selfDelete;
 CButton previous, next, refresh;
 CStatic status;
 void Drop(OCAPHistory_Job*& job) {
  if (job) { SetEvent(job->cancel); job->Release(); job=NULL; }
 }
 void ShowPage() {
  if (!current) return;
  int pages=((int)current->rows.size()+99)/100;
  if (pages<1) pages=1;
  if (page>=pages) page=pages-1;
  OCAPHistory_Render(grid,current,page*100);
  CString text;
  if (current->failed) text="이력 읽기 실패 - 새로고침을 눌러주세요";
  else text.Format("%d / %d 페이지 | %d건%s",page+1,pages,(int)current->rows.size(),
      current->rows.size()>=5000 ? " (최대 5,000건)" : "");
  status.SetWindowText(text);
  previous.EnableWindow(page>0); next.EnableWindow(page+1<pages);
 }
protected:
 virtual void PostNcDestroy() { if (selfDelete) delete this; }
 virtual LRESULT WindowProc(UINT message, WPARAM w, LPARAM l) {
  if (message==WM_TIMER && w==1) {
   if (pending && WaitForSingleObject(pending->done,0)==WAIT_OBJECT_0) {
    KillTimer(1); Drop(current); current=pending; pending=NULL; page=0; ShowPage();
    refresh.EnableWindow(TRUE);
   }
   return 0;
  }
  if (message==WM_COMMAND && HIWORD(w)==BN_CLICKED) {
   if (LOWORD(w)==1 && current && page>0) { --page; ShowPage(); }
   if (LOWORD(w)==2 && current && (page+1)*100<(int)current->rows.size()) { ++page; ShowPage(); }
   if (LOWORD(w)==3 && current && !pending) {
    Start(current->folder,current->codes,current->type,current->defect,current->common,current->minimum);
   }
   return 0;
  }
  if (message==WM_DESTROY) {
   KillTimer(1); Drop(pending); Drop(current);
   if (::IsWindow(gridWindow)) ::RemoveProp(gridWindow,_T("OCAPHistoryView"));
  }
  return CWnd::WindowProc(message,w,l);
 }
public:
 OCAPHistory_View(CGridCS* value) : grid(value), gridWindow(value->GetSafeHwnd()),
    pending(NULL),current(NULL),page(0),selfDelete(false) {}
 bool Init() {
  CRect rect; grid->GetWindowRect(&rect); grid->GetParent()->ScreenToClient(&rect);
  CRect toolbar=rect; toolbar.top=toolbar.bottom-30;
  if (!CreateEx(0,AfxRegisterWndClass(0),_T(""),WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,
      toolbar,grid->GetParent(),0)) return false;
  selfDelete=true;
  rect.bottom-=32; grid->MoveWindow(rect);
  previous.Create(_T("< 이전"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,CRect(0,2,75,28),this,1);
  next.Create(_T("다음 >"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,CRect(80,2,155,28),this,2);
  refresh.Create(_T("새로고침"),WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,CRect(160,2,240,28),this,3);
  status.Create(_T(""),WS_CHILD|WS_VISIBLE|SS_CENTERIMAGE,CRect(250,2,toolbar.Width(),28),this,4);
  CFont* font=grid->GetParent()->GetFont();
  previous.SetFont(font); next.SetFont(font); refresh.SetFont(font); status.SetFont(font);
  previous.EnableWindow(FALSE); next.EnableWindow(FALSE);
  ::SetProp(gridWindow,_T("OCAPHistoryView"),(HANDLE)this);
  return true;
 }
 void Start(const CString& folder, CString* codes, const CString& type,
     const double* defect,const double* common,int minimum) {
  Drop(pending);
  OCAPHistory_Job* job=new OCAPHistory_Job;
  job->folder=folder; job->type=type; job->minimum=minimum;
  for (int i=0;i<100;++i) { job->codes[i]=codes[i]; job->defect[i]=defect[i]; }
  for (int i=0;i<4;++i) job->common[i]=common[i];
  if (!job->done || !job->cancel) { job->Release(); status.SetWindowText(_T("이력 읽기 실패")); return; }
  pending=job;
  status.SetWindowText(_T("이력을 불러오는 중...")); refresh.EnableWindow(FALSE);
  // Only a worker-owned reference is released by the worker.
  InterlockedIncrement(&job->refs);
  CWinThread* thread=NULL;
  try { thread=AfxBeginThread(OCAPHistory_Worker,job,THREAD_PRIORITY_BELOW_NORMAL); }
  catch (CException* ex) { ex->Delete(); }
  if (!thread) { job->Release(); Drop(pending); status.SetWindowText(_T("이력 읽기 실패")); refresh.EnableWindow(TRUE); return; }
  if (!SetTimer(1,100,NULL)) {
   Drop(pending); status.SetWindowText(_T("이력 읽기 실패")); refresh.EnableWindow(TRUE);
  }
 }
};

static void OCAPHistory_Display(CGridCS* grid, const CString& folder, CString* codes,
 const CString& type,const double* defect,const double* common,int minimum)
{
 if (!grid || !::IsWindow(grid->GetSafeHwnd()) || !codes || !defect || !common) return;
 OCAPHistory_View* view=(OCAPHistory_View*)::GetProp(grid->GetSafeHwnd(),_T("OCAPHistoryView"));
 if (!view) {
  view=new OCAPHistory_View(grid);
  if (!view->Init()) { delete view; return; }
 }
 view->Start(folder,codes,type,defect,common,minimum);
}
