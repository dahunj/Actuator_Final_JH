#pragma once
#include <vector>
#include <algorithm>

// Shared CSV reader for FAI / Cosmetic. Never modifies CSV files or alarm data.
static void OCAPHistory_Split(const CString& sLine, std::vector<CString>& cells)
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
	(void)sType;
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
	while (bMore) {
		bMore = finder.FindNextFile();
		if (finder.IsDirectory()) continue;
		CString sDate = finder.GetFileName().Left(8);
		BOOL bDate = sDate.GetLength() == 8;
		for (int k=0; k<sDate.GetLength(); k++) if (sDate[k] < '0' || sDate[k] > '9') bDate = FALSE;
		if (!bDate) continue;
		if (sDate.Compare(sCutoff) < 0 || sDate.Compare(sToday) > 0) continue;
		CStdioFile file;
		if (!file.Open(finder.GetFilePath(), CFile::modeRead | CFile::typeText | CFile::shareDenyNone)) continue;
		try {
			CString sLine;
			std::vector<CString> header, data;
			if (!file.ReadString(sLine)) { file.Close(); continue; }
			OCAPHistory_Split(sLine, header);
			// F2 records one Lot per row, with Count/Rate/ERR Count/ERR Rate per code.
			if (header.size() < 409 || header[0] != "Time" || header[1] != "LotID" || header[2] != "Total") { file.Close(); continue; }
			BOOL bCodeOnly = finder.GetFileName().Find("CodeV2_") >= 0;
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
			while (file.ReadString(sLine)) {
				OCAPHistory_Split(sLine, data);
				// Ignore a partial final record while the logger is appending it.
				if (data.size() < header.size() || data[0].GetLength() < 8) continue;
				OCAPHistory_Row row;
				row.sKey = sDate + data[0];
				row.cells.resize(nBase+400);
				row.cells[0] = sDate.Left(4) + "/" + sDate.Mid(4,2) + "/" + sDate.Mid(6,2);
				row.cells[1] = data[0]; row.cells[2] = data[1];
				row.cells[3] = data[2];
				row.cells[4].Format("%d", _ttoi(data[3]) + _ttoi(data[4]) + _ttoi(data[5]));
				row.cells[5] = data[3]; row.cells[6] = data[4]; row.cells[7] = data[5];
				row.cells[8].Format("%d", _ttoi(data[6]) + _ttoi(data[7]) + _ttoi(data[8]));
				row.cells[9] = data[6]; row.cells[10] = data[7]; row.cells[11] = data[8];
				int nTotal = 0, nGroup[3] = { 0, 0, 0 };
				for (int k=0; k<100; k++) {
					int nCount = _ttoi(data[9+k*4]);
					nTotal += nCount;
					if (k<7) nGroup[0] += nCount;
					else if (k<15) nGroup[1] += nCount;
					else if (k<20) nGroup[2] += nCount;
				}
				row.cells[12].Format("%d", nTotal);
				if (nBase == 16) for (int k=0; k<3; k++) row.cells[13+k].Format("%d", nGroup[k]);
				for (int j=0; j<400; j++) {
					if (nMap[j] >= 0 && nMap[j] < (int)data.size()) {
						row.cells[j+nBase] = data[nMap[j]];
						if (j%2 == 1) row.cells[j+nBase] += "%";
					}
				}
				rows.push_back(row);
			}
			file.Close();
		} catch (CFileException* pEx) { pEx->Delete(); }
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
