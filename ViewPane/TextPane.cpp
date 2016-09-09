#include "stdafx.h"
#include "TextPane.h"
#include "String.h"
#include "UIFunctions.h"

static wstring CLASS = L"TextPane";

ViewPane* CreateMultiLinePane(UINT uidLabel, bool bReadOnly)
{
	return CreateSingleLinePane(uidLabel, bReadOnly, true);
}

ViewPane* CreateMultiLinePane(UINT uidLabel, _In_ wstring szVal, bool bReadOnly)
{
	return CreateSingleLinePane(uidLabel, szVal, bReadOnly, true);
}

ViewPane* CreateSingleLinePane(UINT uidLabel, bool bReadOnly, bool bMultiLine)
{
	return new TextPane(uidLabel, bReadOnly, bMultiLine);
}

ViewPane* CreateSingleLinePane(UINT uidLabel, _In_ wstring szVal, bool bReadOnly, bool bMultiLine)
{
	auto lpPane = new TextPane(uidLabel, bReadOnly, bMultiLine);
	if (lpPane)
	{
		lpPane->SetStringW(szVal.c_str());
	}

	return lpPane;
}

ViewPane* CreateSingleLinePaneID(UINT uidLabel, UINT uidVal, bool bReadOnly)
{
	auto lpPane = new TextPane(uidLabel, bReadOnly, false);

	if (lpPane)
	{
		wstring szTemp;

		if (uidVal)
		{
			szTemp = loadstring(uidVal);
			lpPane->SetStringW(szTemp.c_str());
		}
	}

	return lpPane;
}

// Imports binary data from a stream, converting it to hex format before returning
_Check_return_ static DWORD CALLBACK EditStreamReadCallBack(
	DWORD_PTR dwCookie,
	_In_ LPBYTE pbBuff,
	LONG cb,
	_In_count_(cb) LONG *pcb)
{
	auto hRes = S_OK;
	if (!pbBuff || !pcb || !dwCookie) return 0;

	auto stmData = reinterpret_cast<LPSTREAM>(dwCookie);

	*pcb = 0;

	DebugPrint(DBGStream, L"EditStreamReadCallBack: cb = %d\n", cb);

	auto cbTemp = cb / 2;
	ULONG cbTempRead = 0;
	auto pbTempBuff = new BYTE[cbTemp];

	if (pbTempBuff)
	{
		EC_MAPI(stmData->Read(pbTempBuff, cbTemp, &cbTempRead));
		DebugPrint(DBGStream, L"EditStreamReadCallBack: read %u bytes\n", cbTempRead);

		memset(pbBuff, 0, cbTempRead * 2);
		ULONG iBinPos = 0;
		for (ULONG i = 0; i < cbTempRead; i++)
		{
			BYTE bLow;
			BYTE bHigh;
			CHAR szLow;
			CHAR szHigh;

			bLow = static_cast<BYTE>(pbTempBuff[i] & 0xf);
			bHigh = static_cast<BYTE>(pbTempBuff[i] >> 4 & 0xf);
			szLow = static_cast<CHAR>(bLow <= 0x9 ? '0' + bLow : 'A' + bLow - 0xa);
			szHigh = static_cast<CHAR>(bHigh <= 0x9 ? '0' + bHigh : 'A' + bHigh - 0xa);

			pbBuff[iBinPos] = szHigh;
			pbBuff[iBinPos + 1] = szLow;

			iBinPos += 2;
		}

		*pcb = cbTempRead * 2;

		delete[] pbTempBuff;
	}

	return 0;
}

TextPane::TextPane(UINT uidLabel, bool bReadOnly, bool bMultiLine) :ViewPane(uidLabel, bReadOnly)
{
	m_bMultiline = bMultiLine;
}

bool TextPane::IsType(__ViewTypes vType)
{
	return CTRL_TEXTPANE == vType;
}

ULONG TextPane::GetFlags()
{
	ULONG ulFlags = vpNone;
	if (m_EditBox.m_hWnd && m_EditBox.GetModify()) ulFlags |= vpDirty;
	if (m_bReadOnly) ulFlags |= vpReadonly;
	return ulFlags;
}

int TextPane::GetFixedHeight()
{
	auto iHeight = 0;
	if (0 != m_iControl) iHeight += m_iSmallHeightMargin; // Top margin

	if (m_bUseLabelControl)
	{
		// Text labels will bump directly against their edit control, so we don't add a margin here
		iHeight += m_iLabelHeight;
	}

	if (!m_bMultiline)
	{
		iHeight += m_iEditHeight;
	}

	iHeight += m_iSmallHeightMargin; // Bottom margin

	return iHeight;
}

int TextPane::GetLines()
{
	if (m_bMultiline)
	{
		return LINES_MULTILINEEDIT;
	}

	return 0;
}

void TextPane::SetWindowPos(int x, int y, int width, int height)
{
	auto hRes = S_OK;
	if (0 != m_iControl)
	{
		y += m_iSmallHeightMargin;
		height -= m_iSmallHeightMargin;
	}

	if (m_bUseLabelControl)
	{
		EC_B(m_Label.SetWindowPos(
			nullptr,
			x,
			y,
			width,
			m_iLabelHeight,
			SWP_NOZORDER));
		y += m_iLabelHeight;
		height -= m_iLabelHeight;
	}

	height -= m_iSmallHeightMargin; // This is the bottom margin

	EC_B(m_EditBox.SetWindowPos(NULL, x, y, width, height, SWP_NOZORDER));
}

void TextPane::Initialize(int iControl, _In_ CWnd* pParent, _In_ HDC /*hdc*/)
{
	ViewPane::Initialize(iControl, pParent, nullptr);

	auto hRes = S_OK;

	EC_B(m_EditBox.Create(
		WS_TABSTOP
		| WS_CHILD
		| WS_CLIPSIBLINGS
		| WS_BORDER
		| WS_VISIBLE
		| WS_VSCROLL
		| ES_AUTOVSCROLL
		| (m_bReadOnly ? ES_READONLY : 0)
		| (m_bMultiline ? (ES_MULTILINE | ES_WANTRETURN) : (ES_AUTOHSCROLL)),
		CRect(0, 0, 0, 0),
		pParent,
		m_nID));
	SubclassEdit(m_EditBox.m_hWnd, pParent ? pParent->m_hWnd : NULL, m_bReadOnly);

	m_bInitialized = true; // We can now call SetEditBoxText

	// Set maximum text size
	// Use -1 to allow for VERY LARGE strings
	(void) ::SendMessage(
		m_EditBox.m_hWnd,
		EM_EXLIMITTEXT,
		static_cast<WPARAM>(0),
		static_cast<LPARAM>(-1));

	SetEditBoxText();

	m_EditBox.SetEventMask(ENM_CHANGE);

	// Clear the modify bits so we can detect changes later
	m_EditBox.SetModify(false);

	// Remove the awful autoselect of the edit control that scrolls to the end of multiline text
	if (m_bMultiline)
	{
		::PostMessage(
			m_EditBox.m_hWnd,
			EM_SETSEL,
			static_cast<WPARAM>(0),
			static_cast<LPARAM>(0));
	}
}

struct FakeStream
{
	wstring lpszW;
	size_t cbszW;
	size_t cbCur;
};

_Check_return_ static DWORD CALLBACK FakeEditStreamReadCallBack(
	DWORD_PTR dwCookie,
	_In_ LPBYTE pbBuff,
	LONG cb,
	_In_count_(cb) LONG *pcb)
{
	if (!pbBuff || !pcb || !dwCookie) return 0;

	auto lpfs = reinterpret_cast<FakeStream*>(dwCookie);
	if (!lpfs) return 0;
	auto cbRemaining = static_cast<ULONG>(lpfs->cbszW - lpfs->cbCur);
	auto cbRead = min((ULONG)cb, cbRemaining);

	*pcb = cbRead;

	if (cbRead) memcpy(pbBuff, LPBYTE(lpfs->lpszW.c_str()) + lpfs->cbCur, cbRead);

	lpfs->cbCur += cbRead;

	return 0;
}

void TextPane::SetEditBoxText()
{
	if (!m_bInitialized) return;
	if (!m_EditBox.m_hWnd) return;

	ULONG ulEventMask = m_EditBox.GetEventMask(); // Get original mask
	m_EditBox.SetEventMask(ENM_NONE);

	// In order to support strings with embedded NULLs, we're going to stream the string in
	// We don't have to build a real stream interface - we can fake a lightweight one
	EDITSTREAM es = { 0, 0, FakeEditStreamReadCallBack };
	UINT uFormat = SF_TEXT | SF_UNICODE;

	FakeStream fs;
	fs.lpszW = m_lpszW;
	fs.cbszW = m_lpszW.length() * sizeof(WCHAR);
	fs.cbCur = 0;

	es.dwCookie = reinterpret_cast<DWORD_PTR>(&fs);

	// read the 'text stream' into control
	auto lBytesRead = m_EditBox.StreamIn(uFormat, es);
	DebugPrintEx(DBGStream, CLASS, L"SetEditBoxText", L"read %d bytes from the stream\n", lBytesRead);

	// Clear the modify bit so this stream appears untouched
	m_EditBox.SetModify(false);

	m_EditBox.SetEventMask(ulEventMask); // put original mask back
}

// Sets m_lpControls[i].UI.lpEdit->lpszW using SetStringW
// cchsz of -1 lets AnsiToUnicode and SetStringW calculate the length on their own
void TextPane::SetStringA(_In_opt_z_ LPCSTR szMsg, size_t cchsz)
{
	if (!szMsg) szMsg = "";
	auto hRes = S_OK;

	LPWSTR szMsgW = nullptr;
	size_t cchszW = 0;
	EC_H(AnsiToUnicode(szMsg, &szMsgW, &cchszW, cchsz));
	if (SUCCEEDED(hRes))
	{
		SetStringW(szMsgW, cchszW);
	}

	delete[] szMsgW;
}

// Sets m_lpControls[i].UI.lpEdit->lpszW
// cchsz may or may not include the NULL terminator (if one is present)
// If it is missing, we'll make sure we add it
void TextPane::SetStringW(_In_opt_z_ LPCWSTR szMsg, size_t cchsz)
{
	m_lpszW.clear();

	if (!szMsg) szMsg = L"";
	auto hRes = S_OK;
	auto cchszW = cchsz;

	if (-1 == cchszW)
	{
		EC_H(StringCchLengthW(szMsg, STRSAFE_MAX_CCH, &cchszW));
	}
	// If we were passed a NULL terminated string,
	// cchszW counts the NULL terminator. Back off one.
	else if (cchszW && szMsg[cchszW - 1] == NULL)
	{
		cchszW--;
	}
	// cchszW is now the length of our string not counting the NULL terminator

	m_lpszW = wstring(szMsg, szMsg + cchszW);

	SetEditBoxText();
}

void TextPane::SetBinary(_In_opt_count_(cb) LPBYTE lpb, size_t cb)
{
	if (!lpb || !cb)
	{
		SetStringW(nullptr);
	}
	else
	{
		SetStringW(BinToHexString(lpb, cb, false).c_str());
	}
}

// This is used by the DbgView - don't call any debugger functions here!!!
void TextPane::AppendString(_In_z_ LPCTSTR szMsg)
{
	m_EditBox.HideSelection(false, true);

	auto cchText = m_EditBox.GetWindowTextLength();
	m_EditBox.SetSel(cchText, cchText);
	m_EditBox.ReplaceSel(szMsg);
}

// This is used by the DbgView - don't call any debugger functions here!!!
void TextPane::ClearView()
{
	m_lpszW.clear();
	::SendMessage(
		m_EditBox.m_hWnd,
		WM_SETTEXT,
		NULL,
		reinterpret_cast<LPARAM>(""));
}

void TextPane::SetEditReadOnly()
{
	m_EditBox.SetBackgroundColor(false, MyGetSysColor(cBackgroundReadOnly));
	m_EditBox.SetReadOnly();
}

wstring TextPane::GetStringW() const
{
	return m_lpszW;
}

string TextPane::GetStringA() const
{
	return wstringTostring(m_lpszW);
}

// Gets string from edit box and places it in m_lpControls[i].UI.lpEdit->lpszW
void TextPane::CommitUIValues()
{
	m_lpszW.clear();

	GETTEXTLENGTHEX getTextLength = { 0 };
	getTextLength.flags = GTL_PRECISE | GTL_NUMCHARS;
	getTextLength.codepage = 1200;

	auto cchText = static_cast<size_t>(::SendMessage(
		m_EditBox.m_hWnd,
		EM_GETTEXTLENGTHEX,
		reinterpret_cast<WPARAM>(&getTextLength),
		static_cast<LPARAM>(0)));
	if (E_INVALIDARG == cchText)
	{
		// we didn't get a length - try another method
		cchText = static_cast<size_t>(::SendMessage(
			m_EditBox.m_hWnd,
			WM_GETTEXTLENGTH,
			static_cast<WPARAM>(0),
			static_cast<LPARAM>(0)));
	}

	// cchText will never include the NULL terminator, so add one to our count
	cchText += 1;

	auto lpszW = new WCHAR[cchText];

	if (lpszW)
	{
		memset(lpszW, 0, cchText * sizeof(WCHAR));

		if (cchText >= 1) // No point in checking if the string is just a null terminator
		{
			GETTEXTEX getText = { 0 };
			getText.cb = static_cast<DWORD>(cchText) * sizeof(WCHAR);
			getText.flags = GT_DEFAULT;
			getText.codepage = 1200;

			auto cchW = ::SendMessage(
				m_EditBox.m_hWnd,
				EM_GETTEXTEX,
				reinterpret_cast<WPARAM>(&getText),
				reinterpret_cast<LPARAM>(lpszW));
			if (0 == cchW)
			{
				// Didn't get a string from this message, fall back to WM_GETTEXT
				auto lpszA = new CHAR[cchText];
				if (lpszA)
				{
					memset(lpszA, 0, cchText * sizeof(CHAR));
					auto hRes = S_OK;
					cchW = ::SendMessage(
						m_EditBox.m_hWnd,
						WM_GETTEXT,
						static_cast<WPARAM>(cchText),
						reinterpret_cast<LPARAM>(lpszA));
					if (0 != cchW)
					{
						EC_H(StringCchPrintfW(lpszW, cchText, L"%hs", lpszA)); // STRING_OK
					}
					delete[] lpszA;
				}
			}
		}

		m_lpszW = wstring(lpszW, lpszW + cchText);
	}
}

// No need to free this - treat it like a static
_Check_return_ string TextPane::GetEditBoxTextA(_Out_ size_t* lpcchText)
{
	CommitUIValues();
	if (lpcchText) *lpcchText = m_lpszW.length();
	return GetStringA();
}

_Check_return_ wstring TextPane::GetEditBoxTextW(_Out_ size_t* lpcchText)
{
	CommitUIValues();
	if (lpcchText) *lpcchText = m_lpszW.length();
	return GetStringW();
}

_Check_return_ wstring TextPane::GetStringUseControl() const
{
	auto len = GetWindowTextLength(m_EditBox.m_hWnd) + 1;
	vector<wchar_t> text(len);
	GetWindowTextW(m_EditBox.m_hWnd, &text[0], len);
	return &text[0];
}

// Takes a binary stream and initializes an edit control with the HEX version of this stream
void TextPane::InitEditFromBinaryStream(_In_ LPSTREAM lpStreamIn)
{
	EDITSTREAM es = { 0, 0, EditStreamReadCallBack };
	UINT uFormat = SF_TEXT;

	es.dwCookie = reinterpret_cast<DWORD_PTR>(lpStreamIn);

	// read the 'text' stream into control
	auto lBytesRead = m_EditBox.StreamIn(uFormat, es);
	DebugPrintEx(DBGStream, CLASS, L"InitEditFromStream", L"read %d bytes from the stream\n", lBytesRead);

	// Clear the modify bit so this stream appears untouched
	m_EditBox.SetModify(false);
}

// Writes a hex pane out to a binary stream
void TextPane::WriteToBinaryStream(_In_ LPSTREAM lpStreamOut) const
{
	auto hRes = S_OK;

	auto bin = HexStringToBin(GetStringUseControl());
	if (bin.data() != nullptr)
	{
		ULONG cbWritten = 0;
		EC_MAPI(lpStreamOut->Write(bin.data(), static_cast<ULONG>(bin.size()), &cbWritten));
		DebugPrintEx(DBGStream, CLASS, L"WriteToBinaryStream", L"wrote 0x%X bytes to the stream\n", cbWritten);

		EC_MAPI(lpStreamOut->Commit(STGC_DEFAULT));
	}
}

void TextPane::ShowWindow(int nCmdShow)
{
	m_EditBox.ShowWindow(nCmdShow);
}
