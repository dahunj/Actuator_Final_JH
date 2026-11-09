#pragma once

// VS2010/C++03 compatible rules, independent of MFC and equipment I/O.
inline bool OCAP_IsVisionNG(bool bFAI, int nJudge)
{
	return bFAI ? (nJudge >= 7 && nJudge <= 9) : (nJudge >= 3 && nJudge <= 6);
}

inline bool OCAP_UseModule(bool bFAI, int nJudge, int nRosJudge, int nSpecialNG)
{
	if (bFAI) return nJudge >= 7 && nJudge <= 9;
	return (nJudge == 3 || nJudge == 4) && nRosJudge != 2 && nSpecialNG == 0;
}

inline bool OCAP_IsEligibleRate(int nTotal, int nCount, int nMinimum, double dThreshold)
{
	return nTotal > 0 && nTotal >= nMinimum && nCount > 0 &&
		dThreshold > 0.0 && dThreshold <= 100.0 && nCount * 100.0 / nTotal >= dThreshold;
}

// Exact registered code wins over the F+code measurement-error interpretation.
template<class TString>
bool OCAP_MatchRegisteredCode(const TString& sCode, const TString* pCodes,
	int nIndex, const TString& sPrefix, bool& bError)
{
	if (sCode == pCodes[nIndex]) return true;
	if (sCode != sPrefix + pCodes[nIndex]) return false;
	for (int i=0; i<100; i++) if (sCode == pCodes[i]) return false;
	bError = true;
	return true;
}

template<class TData, class TString>
void OCAP_CountCode(bool bFAI, const TData& data, const TString* pCodes, int nIndex,
	const TString& sPrefix, int& nCount, int& nErrorCount)
{
	nCount = nErrorCount = 0;
	for (int j=0; j<10; j++) for (int k=0; k<40; k++) 
	{
		if (!OCAP_UseModule(bFAI, data.nJudge[j][k][0], data.nRosJudge[j][k], data.nSpecialNG[j][k])) continue;
		bool bFound = false, bError = false;
		for (int v=1; v<=5; v++) {
			if (!OCAP_IsVisionNG(bFAI, data.nJudge[j][k][v])) continue;
			bool bCodeError = false;
			if (OCAP_MatchRegisteredCode(data.sCode[j][k][v], pCodes, nIndex, sPrefix, bCodeError)) {
				bFound = true;
				if (bCodeError) bError = true;
			}
		}
		if (bFound) nCount++;
		if (bError) nErrorCount++;
	}
}
