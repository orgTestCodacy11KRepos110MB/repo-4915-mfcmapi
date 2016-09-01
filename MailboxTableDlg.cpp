// MailboxTableDlg.cpp : implementation file

#include "stdafx.h"
#include "MailboxTableDlg.h"
#include "ContentsTableListCtrl.h"
#include "MapiObjects.h"
#include "MAPIFunctions.h"
#include "MAPIStoreFunctions.h"
#include "SingleMAPIPropListCtrl.h"
#include "ColumnTags.h"
#include "MFCUtilityFunctions.h"
#include "UIFunctions.h"
#include "Editor.h"
#include "PropertyTagEditor.h"
#include "InterpretProp2.h"
#include "SortList/ContentsData.h"

static wstring CLASS = L"CMailboxTableDlg";

CMailboxTableDlg::CMailboxTableDlg(
	_In_ CParentWnd* pParentWnd,
	_In_ CMapiObjects* lpMapiObjects,
	_In_ wstring lpszServerName,
	_In_ LPMAPITABLE lpMAPITable
) :
	CContentsTableDlg(
		pParentWnd,
		lpMapiObjects,
		IDS_MAILBOXTABLE,
		mfcmapiDO_NOT_CALL_CREATE_DIALOG,
		lpMAPITable,
		(LPSPropTagArray)&sptMBXCols,
		NUMMBXCOLUMNS,
		MBXColumns,
		IDR_MENU_MAILBOX_TABLE_POPUP,
		MENU_CONTEXT_MAILBOX_TABLE
	)
{
	TRACE_CONSTRUCTOR(CLASS);
	m_lpszServerName = lpszServerName;
	CreateDialogAndMenu(IDR_MENU_MAILBOX_TABLE);
}

CMailboxTableDlg::~CMailboxTableDlg()
{
	TRACE_DESTRUCTOR(CLASS);
}

BEGIN_MESSAGE_MAP(CMailboxTableDlg, CContentsTableDlg)
	ON_COMMAND(ID_OPENWITHFLAGS, OnOpenWithFlags)
END_MESSAGE_MAP()

void CMailboxTableDlg::OnInitMenu(_In_ CMenu* pMenu)
{
	if (pMenu)
	{
		if (m_lpContentsTableListCtrl)
		{
			int iNumSel = m_lpContentsTableListCtrl->GetSelectedCount();
			pMenu->EnableMenuItem(ID_OPENWITHFLAGS, DIMMSOK(iNumSel));
		}
	}

	CContentsTableDlg::OnInitMenu(pMenu);
}

void CMailboxTableDlg::CreateDialogAndMenu(UINT nIDMenuResource)
{
	DebugPrintEx(DBGCreateDialog, CLASS, L"CreateDialogAndMenu", L"id = 0x%X\n", nIDMenuResource);
	CContentsTableDlg::CreateDialogAndMenu(nIDMenuResource);

	UpdateMenuString(
		m_hWnd,
		ID_CREATEPROPERTYSTRINGRESTRICTION,
		IDS_MBRESMENU);
}

void CMailboxTableDlg::DisplayItem(ULONG ulFlags)
{
	HRESULT hRes = S_OK;
	CWaitCursor Wait; // Change the mouse to an hourglass while we work.

	LPMAPISESSION lpMAPISession = m_lpMapiObjects->GetSession(); // do not release
	if (!lpMAPISession) return;

	LPMDB lpMDB = m_lpMapiObjects->GetMDB(); // do not release
	LPMDB lpGUIDMDB = NULL;
	if (!lpMDB)
	{
		EC_H(OpenMessageStoreGUID(lpMAPISession, pbExchangeProviderPrimaryUserGuid, &lpGUIDMDB));
	}

	LPMDB lpSourceMDB = NULL; // do not release

	lpSourceMDB = lpMDB ? lpMDB : lpGUIDMDB;

	if (lpSourceMDB)
	{
		LPMDB lpNewMDB = NULL;
		wstring szMailboxDN = NULL;
		int iItem = -1;
		SortListData* lpListData = NULL;
		do
		{
			hRes = S_OK;
			lpListData = m_lpContentsTableListCtrl->GetNextSelectedItemData(&iItem);
			if (lpListData && lpListData->Contents())
			{
				szMailboxDN = LPCTSTRToWstring(lpListData->Contents()->szDN);

				if (!szMailboxDN.empty())
				{
					EC_H(OpenOtherUsersMailbox(
						lpMAPISession,
						lpSourceMDB,
						m_lpszServerName,
						szMailboxDN,
						ulFlags,
						false,
						&lpNewMDB));

					if (lpNewMDB)
					{
						EC_H(DisplayObject(
							(LPMAPIPROP)lpNewMDB,
							NULL,
							otStore,
							this));
						lpNewMDB->Release();
						lpNewMDB = NULL;
					}
				}
			}
		} while (iItem != -1);

	}

	if (lpGUIDMDB) lpGUIDMDB->Release();
}

void CMailboxTableDlg::OnDisplayItem()
{
	DisplayItem(OPENSTORE_USE_ADMIN_PRIVILEGE | OPENSTORE_TAKE_OWNERSHIP);
}

void CMailboxTableDlg::OnOpenWithFlags()
{
	HRESULT hRes = S_OK;

	CEditor MyPrompt(
		this,
		IDS_OPENWITHFLAGS,
		IDS_OPENWITHFLAGSPROMPT,
		1,
		CEDITOR_BUTTON_OK | CEDITOR_BUTTON_CANCEL);
	MyPrompt.SetPromptPostFix(AllFlagsToString(PROP_ID(PR_PROFILE_OPEN_FLAGS), true));
	MyPrompt.InitPane(0, CreateSingleLinePane(IDS_CREATESTORENTRYIDFLAGS, false));
	MyPrompt.SetHex(0, OPENSTORE_USE_ADMIN_PRIVILEGE | OPENSTORE_TAKE_OWNERSHIP);
	WC_H(MyPrompt.DisplayDialog());
	if (S_OK == hRes)
	{
		DisplayItem(MyPrompt.GetHex(0));
	}
}

void CMailboxTableDlg::OnCreatePropertyStringRestriction()
{
	HRESULT hRes = S_OK;
	LPSRestriction lpRes = NULL;

	CPropertyTagEditor MyPropertyTag(
		IDS_PROPRES,
		NULL, // prompt
		PR_DISPLAY_NAME,
		m_bIsAB,
		m_lpContainer,
		this);

	WC_H(MyPropertyTag.DisplayDialog());
	if (S_OK == hRes)
	{
		CEditor MyData(
			this,
			IDS_SEARCHCRITERIA,
			IDS_MBSEARCHCRITERIAPROMPT,
			2,
			CEDITOR_BUTTON_OK | CEDITOR_BUTTON_CANCEL);
		MyData.SetPromptPostFix(AllFlagsToString(flagFuzzyLevel, true));

		MyData.InitPane(0, CreateSingleLinePane(IDS_NAME, false));
		MyData.InitPane(1, CreateSingleLinePane(IDS_ULFUZZYLEVEL, false));
		MyData.SetHex(1, FL_IGNORECASE | FL_PREFIX);

		WC_H(MyData.DisplayDialog());
		if (S_OK != hRes) return;

		wstring szString = MyData.GetStringW(0);
		// Allocate and create our SRestriction
		EC_H(CreatePropertyStringRestriction(
			CHANGE_PROP_TYPE(MyPropertyTag.GetPropertyTag(), PT_UNICODE),
			szString,
			MyData.GetHex(1),
			NULL,
			&lpRes));
		if (hRes != S_OK)
		{
			MAPIFreeBuffer(lpRes);
			lpRes = NULL;
		}

		m_lpContentsTableListCtrl->SetRestriction(lpRes);

		SetRestrictionType(mfcmapiNORMAL_RESTRICTION);
	}
}