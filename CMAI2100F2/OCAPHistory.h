#pragma once
#include <vector>
#include <algorithm>

// Shared CSV reader for FAI / Cosmetic. Never modifies CSV files or alarm data.
static bool OCAPHistory_Split(const CString& sLine, std::vector<CString>& cells)
{
	cells.clear();
	CString sCell;
	bool bQuoted = false;
	for (int i=0; i<sLine.GetLength(); i++) {
		TCHAR ch = sLine[i];
		if (ch == '"') {
			if (bQuoted && i+1<sLine.GetLength() && sLine[i+1] == '"') { sCell += '"'; i++; }
			else bQuoted = !bQuoted;
		} else if (ch == ',' && !bQuoted) { cells.push_back(sCell); sCell.Empty(); }
		else sCell += ch;
	}
	cells.push_back(sCell);
	return !bQuoted;
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

static void OCAPHistory_Display(CGridCS* pGrid, const CString& sFolder, CString* pCodes, const CString& sType, const double* pCodeDefect, const double* pCommon, int nMinimum)
{
	if (!pGrid || !pCodes || !pCodeDefect || !pCommon) return;
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

	std::vector<OCAPHistory_Row> rows;
	CFileFind finder;
	BOOL bMore = finder.FindFile(sFolder + "\\*.csv");
	while (bMore && rows.size() < 5000) {
		bMore = finder.FindNextFile();
		if (finder.IsDirectory()) continue;
		CString sName = finder.GetFileName();
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
		CStdioFile file;
		try 
		{
            if (!file.Open(finder.GetFilePath(), CFile::modeRead | CFile::typeText | CFile::shareDenyNone)) continue;
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
            BOOL bCodeOnly = TRUE;
			int nMap[400];
			for (int j=0; j<400; j++) nMap[j] = -1;
			for (int j=9; j<409; j+=4) {
				CString sTitle = header[j];
				int nSeparator = sTitle.Find('|');
				if (!bCodeOnly && (sTitle.GetLength() < 5 || sTitle[3] != '-' || nSeparator < 4)) continue;
				CString sRecorded = bCodeOnly ? sTitle : sTitle.Mid(4, nSeparator-4);
				for (int k=0; k<100; k++) {
					if (!pCodes[k].IsEmpty() && sRecorded == pCodes[k]) {
						nMap[k*2] = j; nMap[k*2+1] = j+1;
						nMap[200+k*2] = j+2; nMap[200+k*2+1] = j+3;
						break;
					}
				}
			}
			for (int nRead=0; nRead<100000 && rows.size()<5000 && OCAPHistory_ReadLine(file, sLine); ++nRead) 
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
				rows.push_back(row);
			}
			file.Close();
		} catch (CFileException* pEx) { pEx->Delete(); file.Abort(); }
	}
	finder.Close();
	std::stable_sort(rows.begin(), rows.end(), OCAPHistory_Newer);
	// WM_SETREDRAW(TRUE) adds WS_VISIBLE even to a hidden window.
	// Only suspend redraw when the grid is already intended to be visible.
	BOOL bVisible = (pGrid->GetStyle() & WS_VISIBLE) != 0;
	if (bVisible) pGrid->SetRedraw(FALSE);
	// Reset the previous display so expired rows cannot remain visible.
	pGrid->Set_RowCount(1);
	int nRows = (int)rows.size()+1;
	if (nRows < 51) nRows = 51;
	pGrid->Set_RowCount(nRows);
	pGrid->Set_ColWidth(0, 100);
	pGrid->Set_ColWidth(1, 105);
	for (int i=1; i<nRows; i++) {
		pGrid->Set_RowHeight(i, 26);
		for (int j=0; j<nBase+400; j++) {
			CString sText;
			if (i <= (int)rows.size()) sText = rows[i-1].cells[j];
			pGrid->Set_CellText(i, j, sText);
			if (j >= nBase) {
				COLORREF crBack = RGB(255, 255, 255);
				if (i <= (int)rows.size()) {
					const OCAPHistory_Row& row = rows[i-1];
					int nCode = ((j-nBase) % 200) / 2;
					int nCountCol = nBase + ((j-nBase) / 2) * 2;
					if (!row.cells[nCountCol].IsEmpty() && !pCodes[nCode].IsEmpty())
						crBack = OCAPHistory_Color(_ttoi(row.cells[3]), _ttoi(row.cells[nCountCol]),
							nMinimum, pCodeDefect[nCode], pCommon, (j-nBase) / 200);
				}
				pGrid->Set_CellBackClr(i, j, crBack);
				// White text remains legible on the original blue highlight.
				pGrid->Set_CellTextClr(i, j, crBack == RGB(0, 0, 255) ? RGB(255, 255, 255) : RGB(0, 0, 0));
			}
		}
	}
	if (bVisible) {
		pGrid->SetRedraw(TRUE);
		pGrid->Invalidate(FALSE);
	}
}
