// FormContainerDlg.cpp : implementation file
// Displays the contents of a form container

#include "stdafx.h"
#include "Error.h"

#include "FormContainerDlg.h"

#include "ContentsTableListCtrl.h"
#include "MapiObjects.h"
#include "SingleMAPIPropListCtrl.h"
#include "ColumnTags.h"
#include "MFCUtilityFunctions.h"
#include "Editor.h"
#include "MAPIProgress.h"
#include "MAPIFunctions.h"
#include "FileDialogEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static TCHAR* CLASS = _T("CFormContainerDlg");

/////////////////////////////////////////////////////////////////////////////
// CFormContainerDlg dialog


CFormContainerDlg::CFormContainerDlg(
			   CParentWnd* pParentWnd,
			   CMapiObjects *lpMapiObjects,
			   LPMAPIFORMCONTAINER lpFormContainer
			   ):
CContentsTableDlg(
						  pParentWnd,
						  lpMapiObjects,
						  IDS_FORMCONTAINER,
						  mfcmapiDO_NOT_CALL_CREATE_DIALOG,
						  NULL,
						  (LPSPropTagArray) &sptDEFCols,
						  NUMDEFCOLUMNS,
						  DEFColumns,
						  IDR_MENU_FORM_CONTAINER_POPUP,
						  MENU_CONTEXT_FORM_CONTAINER)
{
	TRACE_CONSTRUCTOR(CLASS);

	m_lpFormContainer = lpFormContainer;
	if (m_lpFormContainer)
	{
		m_lpFormContainer->AddRef();
		LPTSTR lpszDisplayName = NULL;
		(void)m_lpFormContainer->GetDisplay(fMapiUnicode,&lpszDisplayName);
		if (lpszDisplayName)
		{
			m_szTitle = lpszDisplayName;
			MAPIFreeBuffer(lpszDisplayName);
		}
	}
	CreateDialogAndMenu(IDR_MENU_FORM_CONTAINER);
}

CFormContainerDlg::~CFormContainerDlg()
{
	TRACE_DESTRUCTOR(CLASS);
	if (m_lpFormContainer) m_lpFormContainer->Release();
}

BEGIN_MESSAGE_MAP(CFormContainerDlg, CContentsTableDlg)
//{{AFX_MSG_MAP(CFormContainerDlg)
	ON_COMMAND(ID_DELETESELECTEDITEM, OnDeleteSelectedItem)
	ON_COMMAND(ID_INSTALLFORM, OnInstallForm)
	ON_COMMAND(ID_REMOVEFORM, OnRemoveForm)
	ON_COMMAND(ID_RESOLVEMESSAGECLASS, OnResolveMessageClass)
	ON_COMMAND(ID_RESOLVEMULTIPLEMESSAGECLASSES, OnResolveMultipleMessageClasses)
	ON_COMMAND(ID_CALCFORMPOPSET, OnCalcFormPropSet)
	ON_COMMAND(ID_GETDISPLAY, OnGetDisplay)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFormContainerDlg message handlers

void CFormContainerDlg::OnInitMenu(CMenu* pMenu)
{
	if (pMenu && m_lpContentsTableListCtrl)
	{
		int iNumSel = m_lpContentsTableListCtrl->GetSelectedCount();
		pMenu->EnableMenuItem(ID_DELETESELECTEDITEM,DIMMSOK(iNumSel));
	}
	CContentsTableDlg::OnInitMenu(pMenu);
}

BOOL CFormContainerDlg::OnInitDialog()
{
	HRESULT			hRes = S_OK;
	EC_B(CContentsTableDlg::OnInitDialog());

	if (m_lpContentsTableListCtrl)
	{
		OnRefreshView();
	}

	return HRES_TO_BOOL(hRes);
}//CFormContainerDlg::OnInitDialog

//Clear the current list and get a new one
void CFormContainerDlg::OnRefreshView()
{
	HRESULT hRes = S_OK;

	if (!m_lpContentsTableListCtrl) return;

	if (m_lpContentsTableListCtrl->m_bInLoadOp) m_lpContentsTableListCtrl->OnCancelTableLoad();
	DebugPrintEx(DBGForms,CLASS,_T("OnRefreshView"),_T("\n"));

	EC_B(m_lpContentsTableListCtrl->DeleteAllItems());
	if (m_lpFormContainer)
	{
		LPSMAPIFORMINFOARRAY lpMAPIFormInfoArray = NULL;
		WC_H(m_lpFormContainer->ResolveMultipleMessageClasses(0,NULL,&lpMAPIFormInfoArray));
		if (lpMAPIFormInfoArray)
		{
			ULONG i = NULL;
			for (i=0; i<lpMAPIFormInfoArray->cForms; i++)
			{
				if (0 == i)
				{
					LPSPropTagArray lpTagArray = NULL;
					EC_H(lpMAPIFormInfoArray->aFormInfo[i]->GetPropList(fMapiUnicode,&lpTagArray));
					EC_H(m_lpContentsTableListCtrl->SetUIColumns(lpTagArray));
					MAPIFreeBuffer(lpTagArray);
				}
				if (lpMAPIFormInfoArray->aFormInfo[i])
				{
					ULONG ulPropVals = NULL;
					LPSPropValue lpPropVals = NULL;
					EC_H_GETPROPS(GetPropsNULL(lpMAPIFormInfoArray->aFormInfo[i],fMapiUnicode,&ulPropVals,&lpPropVals));
					if (lpPropVals)
					{
						SRow sRow = {0};
						sRow.cValues = ulPropVals;
						sRow.lpProps = lpPropVals;
						(void)::SendMessage(
							m_lpContentsTableListCtrl->m_hWnd,
							WM_MFCMAPI_THREADADDITEM,
							i,
							(LPARAM)&sRow);
					}
					lpMAPIFormInfoArray->aFormInfo[i]->Release();
				}
			}
			MAPIFreeBuffer(lpMAPIFormInfoArray);
		}
	}
	m_lpContentsTableListCtrl->AutoSizeColumns();
}//CFormContainerDlg::OnRefreshView

HRESULT CFormContainerDlg::OpenItemProp(int iSelectedItem, __mfcmapiModifyEnum /*bModify*/, LPMAPIPROP* lppMAPIProp)
{
	HRESULT			hRes = S_OK;
	SortListData*	lpListData = NULL;

	DebugPrintEx(DBGOpenItemProp,CLASS,_T("OpenItemProp"),_T("iSelectedItem = 0x%X\n"),iSelectedItem);

	if (!lppMAPIProp || !m_lpContentsTableListCtrl || !m_lpFormContainer) return MAPI_E_INVALID_PARAMETER;

	lpListData = (SortListData*) m_lpContentsTableListCtrl->GetItemData(iSelectedItem);
	if (lpListData)
	{
		LPSPropValue lpProp = NULL;//do not free this
		lpProp = PpropFindProp(
			lpListData->lpSourceProps,
			lpListData->cSourceProps,
			PR_MESSAGE_CLASS_A); // ResolveMessageClass requires an ANSI string
		if (CheckStringProp(lpProp,PT_STRING8))
		{
			LPMAPIFORMINFO lpFormInfoProp = NULL;
			EC_H(m_lpFormContainer->ResolveMessageClass(
				lpProp->Value.lpszA,
				MAPIFORM_EXACTMATCH,
				&lpFormInfoProp));
			if (SUCCEEDED(hRes))
			{
				*lppMAPIProp = lpFormInfoProp;
			}
			else if (lpFormInfoProp) lpFormInfoProp->Release();
		}
	}
	return hRes;
}//CFormContainerDlg::OpenItemProp

void CFormContainerDlg::OnDeleteSelectedItem()
{
	HRESULT		hRes = S_OK;
	int			iItem = -1;
	SortListData*	lpListData = NULL;

	if (!m_lpFormContainer || !m_lpContentsTableListCtrl) return;

	do
	{
		hRes = S_OK;
		//Find the highlighted item AttachNum
		lpListData = m_lpContentsTableListCtrl->GetNextSelectedItemData(&iItem);
		if (!lpListData) break;

		LPSPropValue lpProp = NULL;//do not free this
		lpProp = PpropFindProp(
			lpListData->lpSourceProps,
			lpListData->cSourceProps,
			PR_MESSAGE_CLASS_A); // RemoveForm requires an ANSI string
		if (CheckStringProp(lpProp,PT_STRING8))
		{
			DebugPrintEx(
				DBGDeleteSelectedItem,
				CLASS,
				_T("OnDeleteSelectedItem"),// STRING_OK
				_T("Removing form \"%hs\"\n"),// STRING_OK
				lpProp->Value.lpszA);
			EC_H(m_lpFormContainer->RemoveForm(
				lpProp->Value.lpszA));
		}
	}
	while (iItem != -1);

	OnRefreshView();//Update the view since we don't have notifications here.
} //CFormContainerDlg::OnDeleteSelectedItem

void CFormContainerDlg::OnInstallForm()
{
	HRESULT			hRes = S_OK;
	if (!m_lpFormContainer) return;

	DebugPrintEx(DBGForms,CLASS,_T("OnInstallForm"),_T("installing form\n"));
	CEditor MyFlags(
		this,
		IDS_INSTALLFORM,
		IDS_INSTALLFORMPROMPT,
		(ULONG) 1,
		CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL);
	MyFlags.InitSingleLine(0,IDS_FLAGS,NULL,false);
	MyFlags.SetHex(0,MAPIFORM_INSTALL_DIALOG);

	WC_H(MyFlags.DisplayDialog());
	if (S_OK == hRes)
	{
		INT_PTR	iDlgRet = IDOK;
		CString szFileSpec;
		szFileSpec.LoadString(IDS_CFGFILES);

		CFileDialogEx dlgFilePicker(
			TRUE,
			_T("cfg"),// STRING_OK
			NULL,
			OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT,
			szFileSpec,
			this);

		EC_D_DIALOG(dlgFilePicker.DoModal());
		if (iDlgRet == IDOK)
		{
			ULONG ulFlags = MyFlags.GetHex(0);
			HWND hwnd = (ulFlags & MAPIFORM_INSTALL_DIALOG)?m_hWnd:0;
			POSITION pos = dlgFilePicker.GetStartPosition();
			while (pos)
			{
				hRes = S_OK;
				LPSTR szPath = NULL; // InstallForm requires an ANSI string
#ifdef _UNICODE
				EC_H(UnicodeToAnsi(dlgFilePicker.GetNextPathName(pos),&szPath));
#else
				szPath = (LPTSTR) (LPCTSTR) dlgFilePicker.GetNextPathName(pos);
#endif
				if (szPath)
				{
					DebugPrintEx(DBGForms,CLASS,_T("OnInstallForm"),
						_T("Calling InstallForm(0x%08X,0x%08X,\"%s\")\n"),hwnd,ulFlags,szPath);// STRING_OK
					WC_H(m_lpFormContainer->InstallForm((ULONG_PTR)hwnd,ulFlags,(LPTSTR)szPath));
					if (MAPI_E_EXTENDED_ERROR == hRes)
					{
						LPMAPIERROR lpErr = NULL;
						hRes = m_lpFormContainer->GetLastError(hRes,fMapiUnicode,&lpErr);
						if (lpErr)
						{
							EC_MAPIERR(fMapiUnicode,lpErr);
							MAPIFreeBuffer(lpErr);
						}
					}
					else CHECKHRES(hRes);
#ifdef _UNICODE
					delete[] szPath;
#endif
				}
				else break;
				if (bShouldCancel(this,hRes)) break;
			}
			OnRefreshView();//Update the view since we don't have notifications here.
		}
	}
} //CFormContainerDlg::OnInstallForm

void CFormContainerDlg::OnRemoveForm()
{
	HRESULT			hRes = S_OK;
	if (!m_lpFormContainer) return;

	DebugPrintEx(DBGForms,CLASS,_T("OnRemoveForm"),_T("removing form\n"));
	CEditor MyClass(
		this,
		IDS_REMOVEFORM,
		IDS_REMOVEFORMPROMPT,
		(ULONG) 1,
		CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL);
	MyClass.InitSingleLine(0,IDS_CLASS,NULL,false);

	WC_H(MyClass.DisplayDialog());
	if (S_OK == hRes)
	{
		LPSTR szClass = MyClass.GetStringA(0); // RemoveForm requires an ANSI string
		if (szClass)
		{
			DebugPrintEx(DBGForms,CLASS,_T("OnRemoveForm"),
				_T("Calling RemoveForm(\"%s\")\n"),szClass);// STRING_OK
			EC_H(m_lpFormContainer->RemoveForm(szClass));
			OnRefreshView();//Update the view since we don't have notifications here.
		}
	}
} //CFormContainerDlg::OnRemoveForm

void CFormContainerDlg::OnResolveMessageClass()
{
	HRESULT			hRes = S_OK;
	if (!m_lpFormContainer) return;

	DebugPrintEx(DBGForms,CLASS,_T("OnResolveMessageClass"),_T("resolving message class\n"));
	CEditor MyData(
		this,
		IDS_RESOLVECLASS,
		IDS_RESOLVECLASSPROMPT,
		(ULONG) 2,
		CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL);
	MyData.InitSingleLine(0,IDS_CLASS,NULL,false);
	MyData.InitSingleLine(1,IDS_FLAGS,NULL,false);

	WC_H(MyData.DisplayDialog());
	if (S_OK == hRes)
	{
		LPSTR szClass = MyData.GetStringA(0); // ResolveMessageClass requires an ANSI string
		ULONG ulFlags = MyData.GetHex(1);
		if (szClass)
		{
			LPMAPIFORMINFO lpMAPIFormInfo = NULL;
			DebugPrintEx(DBGForms,CLASS,_T("OnResolveMessageClass"),
				_T("Calling ResolveMessageClass(\"%s\",0x%08X)\n"),szClass,ulFlags);// STRING_OK
			EC_H(m_lpFormContainer->ResolveMessageClass(szClass,ulFlags,&lpMAPIFormInfo));
			if (lpMAPIFormInfo)
			{
				OnUpdateSingleMAPIPropListCtrl(lpMAPIFormInfo, NULL);
				DebugPrintFormInfo(DBGForms,lpMAPIFormInfo);
				lpMAPIFormInfo->Release();
			}
		}
	}
} //CFormContainerDlg::OnResolveMessageClass

void CFormContainerDlg::OnResolveMultipleMessageClasses()
{
	HRESULT	hRes = S_OK;
	ULONG i = 0;
	if (!m_lpFormContainer) return;

	DebugPrintEx(DBGForms,CLASS,_T("OnResolveMultipleMessageClasses"),_T("resolving multiple message classes\n"));
	CEditor MyData(
		this,
		IDS_RESOLVECLASSES,
		IDS_RESOLVECLASSESPROMPT,
		(ULONG) 2,
		CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL);
	MyData.InitSingleLine(0,IDS_NUMBER,NULL,false);
	MyData.SetDecimal(0,1);
	MyData.InitSingleLine(1,IDS_FLAGS,NULL,false);

	WC_H(MyData.DisplayDialog());
	if (S_OK == hRes)
	{
		BOOL bCancel = false;
		ULONG ulNumClasses = MyData.GetDecimal(0);
		ULONG ulFlags = MyData.GetHex(1);
		LPSMESSAGECLASSARRAY lpMSGClassArray = NULL;
		if (ulNumClasses && ulNumClasses < MAXMessageClassArray)
		{
			EC_H(MAPIAllocateBuffer(CbMessageClassArray(ulNumClasses),(LPVOID*)&lpMSGClassArray));

			if (lpMSGClassArray)
			{
				lpMSGClassArray->cValues = ulNumClasses;
				for (i = 0 ; i < ulNumClasses ; i++)
				{
					CEditor MyClass(
						this,
						IDS_ENTERMSGCLASS,
						IDS_ENTERMSGCLASSPROMPT,
						(ULONG) 1,
						CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL);
					MyClass.InitSingleLine(0,IDS_CLASS,NULL,false);

					WC_H(MyClass.DisplayDialog());
					if (S_OK == hRes)
					{
						LPSTR szClass = MyClass.GetStringA(0); // MSDN says always use ANSI strings here
						size_t cbClass = 0;
						EC_H(StringCbLengthA(szClass,STRSAFE_MAX_CCH * sizeof(char),&cbClass));

						if (cbClass)
						{
							cbClass++;// for the NULL terminator
							EC_H(MAPIAllocateMore((ULONG)cbClass,lpMSGClassArray,(LPVOID*)&lpMSGClassArray->aMessageClass[i]));
							EC_H(StringCbCopyA((LPSTR)lpMSGClassArray->aMessageClass[i],cbClass,szClass));
						}
						else bCancel = true;
					}
					else bCancel = true;
					if (bCancel) break;
				}
			}
		}
		if (!bCancel)
		{
			LPSMAPIFORMINFOARRAY lpMAPIFormInfoArray = NULL;
			DebugPrintEx(DBGForms,CLASS,_T("OnResolveMultipleMessageClasses"),
				_T("Calling ResolveMultipleMessageClasses(Num Classes = 0x%08X,0x%08X)\n"),ulNumClasses,ulFlags);// STRING_OK
			EC_H(m_lpFormContainer->ResolveMultipleMessageClasses(lpMSGClassArray,ulFlags,&lpMAPIFormInfoArray));
			if (lpMAPIFormInfoArray)
			{
				DebugPrintEx(DBGForms,CLASS,_T("OnResolveMultipleMessageClasses"),_T("Got 0x%08X forms\n"),lpMAPIFormInfoArray->cForms);
				for (i=0; i<lpMAPIFormInfoArray->cForms; i++)
				{
					if (lpMAPIFormInfoArray->aFormInfo[i])
					{
						DebugPrintFormInfo(DBGForms,lpMAPIFormInfoArray->aFormInfo[i]);
						lpMAPIFormInfoArray->aFormInfo[i]->Release();
					}
				}
				MAPIFreeBuffer(lpMAPIFormInfoArray);
			}
		}
		MAPIFreeBuffer(lpMSGClassArray);
	}
} //CFormContainerDlg::OnResolveMultipleMessageClasses

void CFormContainerDlg::OnCalcFormPropSet()
{
	HRESULT			hRes = S_OK;
	if (!m_lpFormContainer) return;

	DebugPrintEx(DBGForms,CLASS,_T("OnCalcFormPropSet"),_T("calculating form property set\n"));
	CEditor MyData(
		this,
		IDS_CALCFORMPROPSET,
		IDS_CALCFORMPROPSETPROMPT,
		(ULONG) 1,
		CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL);
	MyData.InitSingleLine(0,IDS_FLAGS,NULL,false);
	MyData.SetHex(0,FORMPROPSET_UNION);

	WC_H(MyData.DisplayDialog());
	if (S_OK == hRes)
	{
		ULONG ulFlags = MyData.GetHex(0);

		LPMAPIFORMPROPARRAY lpFormPropArray = NULL;
		DebugPrintEx(DBGForms,CLASS,_T("OnCalcFormPropSet"),
			_T("Calling CalcFormPropSet(0x%08X)\n"),ulFlags);// STRING_OK
		EC_H(m_lpFormContainer->CalcFormPropSet(ulFlags,&lpFormPropArray));
		if (lpFormPropArray)
		{
			DebugPrintFormPropArray(DBGForms,lpFormPropArray);
			MAPIFreeBuffer(lpFormPropArray);
		}
	}
} //CFormContainerDlg::OnCalcFormPropSet

void CFormContainerDlg::OnGetDisplay()
{
	HRESULT			hRes = S_OK;
	if (!m_lpFormContainer) return;

	LPTSTR lpszDisplayName = NULL;
	EC_H(m_lpFormContainer->GetDisplay(fMapiUnicode,&lpszDisplayName));

	if (lpszDisplayName)
	{
		DebugPrintEx(DBGForms,CLASS,_T("OnGetDisplay"),_T("Got display name \"%s\"\n"),lpszDisplayName);
		CEditor MyOutput(
			this,
			IDS_GETDISPLAY,
			IDS_GETDISPLAYPROMPT,
			1,
			CEDITOR_BUTTON_OK|CEDITOR_BUTTON_CANCEL);
		MyOutput.InitSingleLineSz(0,IDS_GETDISPLAY,lpszDisplayName,true);
		WC_H(MyOutput.DisplayDialog());
		MAPIFreeBuffer(lpszDisplayName);
	}
} //CFormContainerDlg::OnGetDisplay

void CFormContainerDlg::HandleAddInMenuSingle(
									   LPADDINMENUPARAMS lpParams,
									   LPMAPIPROP lpMAPIProp,
									   LPMAPICONTAINER /*lpContainer*/)
{
	if (lpParams)
	{
		lpParams->lpFormContainer = m_lpFormContainer;
		lpParams->lpFormInfoProp = (LPMAPIFORMINFO) lpMAPIProp; // OpenItemProp returns LPMAPIFORMINFO
	}

	InvokeAddInMenu(lpParams);
} // CFormContainerDlg::HandleAddInMenuSingle
