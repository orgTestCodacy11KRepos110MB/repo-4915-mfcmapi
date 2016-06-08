#include "stdafx.h"
#include "ParseProperty.h"
#include "Property.h"
#include "MAPIFunctions.h"
#include "ExtraPropTags.h"
#include "String.h"
#include "InterpretProp.h"
#include "InterpretProp2.h"
#include <algorithm>
#include <cctype>

wstring BuildErrorPropString(_In_ LPSPropValue lpProp)
{
	if (PROP_TYPE(lpProp->ulPropTag) != PT_ERROR) return L"";
	switch (PROP_ID(lpProp->ulPropTag))
	{
	case PROP_ID(PR_BODY):
	case PROP_ID(PR_BODY_HTML):
	case PROP_ID(PR_RTF_COMPRESSED):
		if (MAPI_E_NOT_ENOUGH_MEMORY == lpProp->Value.err ||
			MAPI_E_NOT_FOUND == lpProp->Value.err)
		{
			return loadstring(IDS_OPENBODY);
		}

		break;
	default:
		if (MAPI_E_NOT_ENOUGH_MEMORY == lpProp->Value.err)
		{
			return loadstring(IDS_OPENSTREAM);
		}
	}

	return L"";
}

wstring BinToTextString(_In_ LPSBinary lpBin, bool bMultiLine)
{
	if (!lpBin || !lpBin->cb || !lpBin->lpb) return L"";

	wstring szBin;

	ULONG i;
	for (i = 0; i < lpBin->cb; i++)
	{
		// Any printable extended ASCII character gets mapped directly
		if (lpBin->lpb[i] >= 0x20 &&
			lpBin->lpb[i] <= 0xFE)
		{
			szBin += lpBin->lpb[i];
		}
		// If we allow multiple lines, we accept tab, LF and CR
		else if (bMultiLine &&
			(lpBin->lpb[i] == 9 || // Tab
				lpBin->lpb[i] == 10 || // Line Feed
				lpBin->lpb[i] == 13))  // Carriage Return
		{
			szBin += lpBin->lpb[i];
		}
		// Everything else is a dot
		else
		{
			szBin += L'.';
		}
	}

	return szBin;
}

wstring BinToHexString(_In_opt_count_(cb) LPBYTE lpb, size_t cb, bool bPrependCB)
{
	wstring lpsz;

	if (bPrependCB)
	{
		lpsz = format(L"cb: %u lpb: ", (UINT)cb); // STRING_OK
	}

	if (!cb || !lpb)
	{
		lpsz += L"NULL";
	}
	else
	{
		ULONG i = 0;
		for (i = 0; i < cb; i++)
		{
			BYTE bLow = (BYTE)((lpb[i]) & 0xf);
			BYTE bHigh = (BYTE)((lpb[i] >> 4) & 0xf);
			wchar_t szLow = (wchar_t)((bLow <= 0x9) ? L'0' + bLow : L'A' + bLow - 0xa);
			wchar_t szHigh = (wchar_t)((bHigh <= 0x9) ? L'0' + bHigh : L'A' + bHigh - 0xa);

			lpsz += szHigh;
			lpsz += szLow;
		}
	}

	return lpsz;
}

wstring BinToHexString(_In_opt_ LPSBinary lpBin, bool bPrependCB)
{
	if (!lpBin) return L"";

	return BinToHexString(
		lpBin->lpb,
		lpBin->cb,
		bPrependCB);
}

bool stripPrefix(wstring& str, wstring prefix)
{
	size_t length = prefix.length();
	if (str.compare(0, length, prefix) == 0)
	{
		str.erase(0, length);
		return true;
	}

	return false;
}

vector<BYTE> HexStringToBin(_In_ wstring lpsz)
{
	// remove junk
	WCHAR szJunk[] = L"\r\n\t -.,\\/'{}`\""; // STRING_OK
	for (unsigned int i = 0; i < _countof(szJunk); ++i)
	{
		lpsz.erase(std::remove(lpsz.begin(), lpsz.end(), szJunk[i]), lpsz.end());
	}

	// strip one (and only one) prefix
	stripPrefix(lpsz, L"0x") ||
		stripPrefix(lpsz, L"0X") ||
		stripPrefix(lpsz, L"x") ||
		stripPrefix(lpsz, L"X");

	size_t iCur = 0;
	WCHAR szTmp[3] = { 0 };
	size_t cchStrLen = lpsz.length();
	vector<BYTE> lpb;

	// We have a clean string now. If it's of odd length, we're done.
	if (cchStrLen % 2 != 0) return vector<BYTE>();

	// convert two characters at a time
	int iTmp = 0;
	while (iCur < cchStrLen)
	{
		// Check for valid hex characters
		if (!isxdigit(lpsz[iCur]) || !isxdigit(lpsz[iCur + 1]))
		{
			return vector<BYTE>();
		}

		szTmp[0] = lpsz[iCur];
		szTmp[1] = lpsz[iCur + 1];
		lpb.push_back((BYTE)wcstol(szTmp, NULL, 16));
		iCur += 2;
	}

	return lpb;
}

Property ParseMVProperty(_In_ LPSPropValue lpProp, ULONG ulMVRow)
{
	if (!lpProp || ulMVRow > lpProp->Value.MVi.cValues) return Property();

	// We'll let ParseProperty do all the work
	SPropValue sProp = { 0 };
	sProp.ulPropTag = CHANGE_PROP_TYPE(lpProp->ulPropTag, PROP_TYPE(lpProp->ulPropTag) & ~MV_FLAG);

	// Only attempt to dereference our array if it's non-NULL
	if (PROP_TYPE(lpProp->ulPropTag) & MV_FLAG &&
		lpProp->Value.MVi.lpi)
	{
		switch (PROP_TYPE(lpProp->ulPropTag))
		{
		case PT_MV_I2:
			sProp.Value.i = lpProp->Value.MVi.lpi[ulMVRow];
			break;
		case PT_MV_LONG:
			sProp.Value.l = lpProp->Value.MVl.lpl[ulMVRow];
			break;
		case PT_MV_DOUBLE:
			sProp.Value.dbl = lpProp->Value.MVdbl.lpdbl[ulMVRow];
			break;
		case PT_MV_CURRENCY:
			sProp.Value.cur = lpProp->Value.MVcur.lpcur[ulMVRow];
			break;
		case PT_MV_APPTIME:
			sProp.Value.at = lpProp->Value.MVat.lpat[ulMVRow];
			break;
		case PT_MV_SYSTIME:
			sProp.Value.ft = lpProp->Value.MVft.lpft[ulMVRow];
			break;
		case PT_MV_I8:
			sProp.Value.li = lpProp->Value.MVli.lpli[ulMVRow];
			break;
		case PT_MV_R4:
			sProp.Value.flt = lpProp->Value.MVflt.lpflt[ulMVRow];
			break;
		case PT_MV_STRING8:
			sProp.Value.lpszA = lpProp->Value.MVszA.lppszA[ulMVRow];
			break;
		case PT_MV_UNICODE:
			sProp.Value.lpszW = lpProp->Value.MVszW.lppszW[ulMVRow];
			break;
		case PT_MV_BINARY:
			sProp.Value.bin = lpProp->Value.MVbin.lpbin[ulMVRow];
			break;
		case PT_MV_CLSID:
			sProp.Value.lpguid = &lpProp->Value.MVguid.lpguid[ulMVRow];
			break;
		default:
			break;
		}
	}

	return ParseProperty(&sProp);
}

Property ParseProperty(_In_ LPSPropValue lpProp)
{
	Property properties;
	if (!lpProp) return properties;

	if (MV_FLAG & PROP_TYPE(lpProp->ulPropTag))
	{
		// MV property
		properties.AddAttribute(L"mv", L"true"); // STRING_OK
		// All the MV structures are basically the same, so we can cheat when we pull the count
		properties.AddAttribute(L"count", to_wstring(lpProp->Value.MVi.cValues)); // STRING_OK

		// Don't bother with the loop if we don't have data
		if (lpProp->Value.MVi.lpi)
		{
			ULONG iMVCount = 0;
			for (iMVCount = 0; iMVCount < lpProp->Value.MVi.cValues; iMVCount++)
			{
				properties.AddMVParsing(ParseMVProperty(lpProp, iMVCount));
			}
		}
	}
	else
	{
		wstring szTmp;
		bool bPropXMLSafe = true;
		Attributes attributes;

		wstring szAltTmp;
		bool bAltPropXMLSafe = true;
		Attributes altAttributes;

		switch (PROP_TYPE(lpProp->ulPropTag))
		{
		case PT_I2:
			szTmp = to_wstring(lpProp->Value.i);
			szAltTmp = format(L"0x%X", lpProp->Value.i); // STRING_OK
			break;
		case PT_LONG:
			szTmp = to_wstring(lpProp->Value.l);
			szAltTmp = format(L"0x%X", lpProp->Value.l); // STRING_OK
			break;
		case PT_R4:
			szTmp = to_wstring(lpProp->Value.flt); // STRING_OK
			break;
		case PT_DOUBLE:
			szTmp = to_wstring(lpProp->Value.dbl); // STRING_OK
			break;
		case PT_CURRENCY:
			szTmp = format(L"%05I64d", lpProp->Value.cur.int64); // STRING_OK
			if (szTmp.length() > 4)
			{
				szTmp.insert(szTmp.length() - 4, L".");
			}

			szAltTmp = format(L"0x%08X:0x%08X", (int)(lpProp->Value.cur.Hi), (int)lpProp->Value.cur.Lo); // STRING_OK
			break;
		case PT_APPTIME:
			szTmp = to_wstring(lpProp->Value.at); // STRING_OK
			break;
		case PT_ERROR:
			szTmp = ErrorNameFromErrorCode(lpProp->Value.err); // STRING_OK
			szAltTmp = BuildErrorPropString(lpProp);

			attributes.AddAttribute(L"err", format(L"0x%08X", lpProp->Value.err)); // STRING_OK
			break;
		case PT_BOOLEAN:
			szTmp = loadstring(lpProp->Value.b ? IDS_TRUE : IDS_FALSE);
			break;
		case PT_OBJECT:
			szTmp = loadstring(IDS_OBJECT);
			break;
		case PT_I8: // LARGE_INTEGER
			szTmp = format(L"0x%08X:0x%08X", (int)(lpProp->Value.li.HighPart), (int)lpProp->Value.li.LowPart); // STRING_OK
			szAltTmp = format(L"%I64d", lpProp->Value.li.QuadPart); // STRING_OK
			break;
		case PT_STRING8:
			if (CheckStringProp(lpProp, PT_STRING8))
			{
				szTmp = LPCSTRToWstring(lpProp->Value.lpszA);
				bPropXMLSafe = false;

				SBinary sBin = { 0 };
				sBin.cb = (ULONG)szTmp.length();
				sBin.lpb = (LPBYTE)lpProp->Value.lpszA;
				szAltTmp = BinToHexString(&sBin, false);

				altAttributes.AddAttribute(L"cb", to_wstring(sBin.cb)); // STRING_OK
			}
			break;
		case PT_UNICODE:
			if (CheckStringProp(lpProp, PT_UNICODE))
			{
				szTmp = lpProp->Value.lpszW;
				bPropXMLSafe = false;

				SBinary sBin = { 0 };
				sBin.cb = ((ULONG)szTmp.length()) * sizeof(WCHAR);
				sBin.lpb = (LPBYTE)lpProp->Value.lpszW;
				szAltTmp = BinToHexString(&sBin, false);

				altAttributes.AddAttribute(L"cb", to_wstring(sBin.cb)); // STRING_OK
			}
			break;
		case PT_SYSTIME:
			FileTimeToString(&lpProp->Value.ft, szTmp, szAltTmp);
			break;
		case PT_CLSID:
			// TODO: One string matches current behavior - look at splitting to two strings in future change
			szTmp = GUIDToStringAndName(lpProp->Value.lpguid);
			break;
		case PT_BINARY:
			szTmp = BinToHexString(&lpProp->Value.bin, false);
			szAltTmp = BinToTextString(&lpProp->Value.bin, true);
			bAltPropXMLSafe = false;

			attributes.AddAttribute(L"cb", to_wstring(lpProp->Value.bin.cb)); // STRING_OK
			break;
		case PT_SRESTRICTION:
			szTmp = RestrictionToString((LPSRestriction)lpProp->Value.lpszA, NULL);
			bPropXMLSafe = false;
			break;
		case PT_ACTIONS:
			szTmp = ActionsToString((ACTIONS*)lpProp->Value.lpszA);
			bPropXMLSafe = false;
			break;
		default:
			break;
		}

		Parsing mainParsing(szTmp, bPropXMLSafe, attributes);
		Parsing altParsing(szAltTmp, bAltPropXMLSafe, altAttributes);
		properties.AddParsing(mainParsing, altParsing);
	}

	return properties;
}