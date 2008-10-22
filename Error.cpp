#include "stdafx.h"
#include "Error.h"
#include "Editor.h"

// This function WILL DebugPrint if it is called
void LogError(
				  HRESULT hRes,
				  UINT uidErrorMsg,
				  LPCSTR szFile,
				  int iLine,
				  LPCSTR szFunction,
				  BOOL bShowDialog,
				  BOOL bErrorMsgFromSystem)
{
	// Get our error message if we have one
	CString szErrorMsg;
	if (bErrorMsgFromSystem)
	{
		// We do this ourselves because CString doesn't know how to
		LPTSTR szErr = NULL;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
			0,
			uidErrorMsg,
			0,
			(LPTSTR)&szErr,
			0,
			0);
		szErrorMsg = szErr;
		LocalFree(szErr);
	}
	else if (uidErrorMsg) szErrorMsg.LoadString(uidErrorMsg);

	CString szErrString;
	szErrString.FormatMessage(
		FAILED(hRes)?IDS_ERRFORMATSTRING:IDS_WARNFORMATSTRING,
		szErrorMsg,
		ErrorNameFromErrorCode(hRes),
		hRes,
		szFunction,
		szFile,
		iLine);

	DebugPrint(DBGHRes,szErrString);
	_Output(DBGHRes,NULL, false, _T("\n"));

	if (bShowDialog)
	{
		CEditor Err(
			NULL,
			ID_PRODUCTNAME,
			NULL,
			(ULONG) 0,
			CEDITOR_BUTTON_OK);
		Err.SetPromptPostFix(szErrString);
		Err.DisplayDialog();
	}
}

BOOL CheckHResFn(HRESULT hRes,LPCSTR szFunction,UINT uidErrorMsg,LPCSTR szFile,int iLine)
{
	if (S_OK == hRes) return true;
	LogError(hRes,uidErrorMsg,szFile,iLine,szFunction,true,false);

	return false;
}

// Warn logs an error but never displays a dialog
// We can log MAPI_W errors along with normal ones
BOOL WarnHResFn(HRESULT hRes,LPCSTR szFunction,UINT uidErrorMsg,LPCSTR szFile,int iLine)
{
	if (fIsSet(DBGHRes) && S_OK != hRes)
	{
		LogError(hRes,uidErrorMsg,szFile,iLine,szFunction,false,false);
	}
	return SUCCEEDED(hRes);
}

HRESULT DialogOnWin32Error(LPCSTR szFile,int iLine, LPCSTR szFunction)
{
	DWORD dwErr = GetLastError();
	if (0 == dwErr) return S_OK;

	HRESULT hRes = HRESULT_FROM_WIN32(dwErr);
	if (S_OK == hRes) return S_OK;
	LogError(hRes,dwErr,szFile,iLine,szFunction,true,true);

	return hRes;
}

HRESULT WarnOnWin32Error(LPCSTR szFile,int iLine, LPCSTR szFunction)
{
	DWORD dwErr = GetLastError();
	HRESULT hRes = HRESULT_FROM_WIN32(dwErr);

	if (fIsSet(DBGHRes) && S_OK != hRes)
	{
		LogError(hRes,dwErr,szFile,iLine,szFunction,false,true);
	}

	return hRes;
}

void __cdecl ErrDialog(LPCSTR szFile,int iLine, UINT uidErrorFmt,...)
{
	CString szErrorFmt;
	szErrorFmt.LoadString(uidErrorFmt);
	CString szErrorBegin;
	CString szErrorEnd;
	CString szCombo;

	// Build out error message from the variant argument list
	va_list argList = NULL;
	va_start(argList, uidErrorFmt);
	szErrorBegin.FormatV(szErrorFmt,argList);
	va_end(argList);

	szErrorEnd.FormatMessage(IDS_INFILEONLINE,szFile,iLine);

	szCombo = szErrorBegin+szErrorEnd;

	DebugPrint(DBGHRes,szCombo);
	_Output(DBGHRes,NULL, false, _T("\n"));

	CEditor Err(
		NULL,
		ID_PRODUCTNAME,
		NULL,
		(ULONG) 0,
		CEDITOR_BUTTON_OK);
	Err.SetPromptPostFix(szCombo);
	Err.DisplayDialog();
}


#define E_ACCT_NOT_FOUND 0x800C8101
#define E_ACCT_WRONG_SORT_ORDER 0x800C8105
#define E_OLK_REGISTRY  0x800C8001
#define E_OLK_ALREADY_INITIALIZED  0x800C8002
#define E_OLK_PARAM_NOT_SUPPORTED 0x800C8003
#define E_OLK_NOT_INITIALIZED 0x800C8005
#define E_OLK_PROP_READ_ONLY  0x800C800D

// [MS-OXCDATA]
#ifndef MAPI_E_LOCKID_LIMIT
#define MAPI_E_LOCKID_LIMIT MAKE_MAPI_E( 0x60D )
#endif
#ifndef MAPI_E_NAMED_PROP_QUOTA_EXCEEDED
#define MAPI_E_NAMED_PROP_QUOTA_EXCEEDED MAKE_MAPI_E( 0x900 )
#endif

#ifndef MAPI_E_PROFILE_DELETED
#define MAPI_E_PROFILE_DELETED MAKE_MAPI_E( 0x204 )
#endif

#ifndef SYNC_E_CYCLE
#define SYNC_E_CYCLE MAKE_SYNC_E(0x804)
#endif

#define RETURN_ERR_CASE(err) case (err): return(_T(#err))

// Function to convert error codes to their names
LPTSTR	ErrorNameFromErrorCode(HRESULT hrErr)
{
	switch (hrErr)
	{
		RETURN_ERR_CASE(S_FALSE);
		RETURN_ERR_CASE(MAPI_E_CALL_FAILED);
		RETURN_ERR_CASE(MAPI_E_NOT_ENOUGH_MEMORY);
		RETURN_ERR_CASE(MAPI_E_INVALID_PARAMETER);
		RETURN_ERR_CASE(MAPI_E_INTERFACE_NOT_SUPPORTED);
		RETURN_ERR_CASE(MAPI_E_NO_ACCESS);

		RETURN_ERR_CASE(MAPI_E_NO_SUPPORT);
		RETURN_ERR_CASE(MAPI_E_BAD_CHARWIDTH);
		RETURN_ERR_CASE(MAPI_E_STRING_TOO_LONG);
		RETURN_ERR_CASE(MAPI_E_UNKNOWN_FLAGS);
		RETURN_ERR_CASE(MAPI_E_INVALID_ENTRYID);
		RETURN_ERR_CASE(MAPI_E_INVALID_OBJECT);
		RETURN_ERR_CASE(MAPI_E_OBJECT_CHANGED);
		RETURN_ERR_CASE(MAPI_E_OBJECT_DELETED);
		RETURN_ERR_CASE(MAPI_E_BUSY);
		RETURN_ERR_CASE(MAPI_E_NOT_ENOUGH_DISK);
		RETURN_ERR_CASE(MAPI_E_NOT_ENOUGH_RESOURCES);
		RETURN_ERR_CASE(MAPI_E_NOT_FOUND);
		RETURN_ERR_CASE(MAPI_E_VERSION);
		RETURN_ERR_CASE(MAPI_E_LOGON_FAILED);
		RETURN_ERR_CASE(MAPI_E_SESSION_LIMIT);
		RETURN_ERR_CASE(MAPI_E_USER_CANCEL);
		RETURN_ERR_CASE(MAPI_E_UNABLE_TO_ABORT);
		RETURN_ERR_CASE(MAPI_E_NETWORK_ERROR);
		RETURN_ERR_CASE(MAPI_E_DISK_ERROR);
		RETURN_ERR_CASE(MAPI_E_TOO_COMPLEX);
		RETURN_ERR_CASE(MAPI_E_BAD_COLUMN);
		RETURN_ERR_CASE(MAPI_E_EXTENDED_ERROR);
		RETURN_ERR_CASE(MAPI_E_COMPUTED);
		RETURN_ERR_CASE(MAPI_E_CORRUPT_DATA);
		RETURN_ERR_CASE(MAPI_E_UNCONFIGURED);
		RETURN_ERR_CASE(MAPI_E_FAILONEPROVIDER);
		RETURN_ERR_CASE(MAPI_E_UNKNOWN_CPID);
		RETURN_ERR_CASE(MAPI_E_UNKNOWN_LCID);

		/* Flavors of E_ACCESSDENIED, used at logon */

		RETURN_ERR_CASE(MAPI_E_PASSWORD_CHANGE_REQUIRED);
		RETURN_ERR_CASE(MAPI_E_PASSWORD_EXPIRED);
		RETURN_ERR_CASE(MAPI_E_INVALID_WORKSTATION_ACCOUNT);
		RETURN_ERR_CASE(MAPI_E_INVALID_ACCESS_TIME);
		RETURN_ERR_CASE(MAPI_E_ACCOUNT_DISABLED);

		/* MAPI base function and status object specific errors and warnings */

		RETURN_ERR_CASE(MAPI_E_END_OF_SESSION);
		RETURN_ERR_CASE(MAPI_E_UNKNOWN_ENTRYID);
		RETURN_ERR_CASE(MAPI_E_MISSING_REQUIRED_COLUMN);
		RETURN_ERR_CASE(MAPI_W_NO_SERVICE);

		/* Property specific errors and warnings */

		RETURN_ERR_CASE(MAPI_E_BAD_VALUE);
		RETURN_ERR_CASE(MAPI_E_INVALID_TYPE);
		RETURN_ERR_CASE(MAPI_E_TYPE_NO_SUPPORT);
		RETURN_ERR_CASE(MAPI_E_UNEXPECTED_TYPE);
		RETURN_ERR_CASE(MAPI_E_TOO_BIG);
		RETURN_ERR_CASE(MAPI_E_DECLINE_COPY);
		RETURN_ERR_CASE(MAPI_E_UNEXPECTED_ID);

		RETURN_ERR_CASE(MAPI_W_ERRORS_RETURNED);

		/* Table specific errors and warnings */

		RETURN_ERR_CASE(MAPI_E_UNABLE_TO_COMPLETE);
		RETURN_ERR_CASE(MAPI_E_TIMEOUT);
		RETURN_ERR_CASE(MAPI_E_TABLE_EMPTY);
		RETURN_ERR_CASE(MAPI_E_TABLE_TOO_BIG);

		RETURN_ERR_CASE(MAPI_E_INVALID_BOOKMARK);

		RETURN_ERR_CASE(MAPI_W_POSITION_CHANGED);
		RETURN_ERR_CASE(MAPI_W_APPROX_COUNT);

		/* Transport specific errors and warnings */

		RETURN_ERR_CASE(MAPI_E_WAIT);
		RETURN_ERR_CASE(MAPI_E_CANCEL);
		RETURN_ERR_CASE(MAPI_E_NOT_ME);

		RETURN_ERR_CASE(MAPI_W_CANCEL_MESSAGE);

		/* Message Store, Folder, and Message specific errors and warnings */

		RETURN_ERR_CASE(MAPI_E_CORRUPT_STORE);
		RETURN_ERR_CASE(MAPI_E_NOT_IN_QUEUE);
		RETURN_ERR_CASE(MAPI_E_NO_SUPPRESS);
		RETURN_ERR_CASE(MAPI_E_COLLISION);
		RETURN_ERR_CASE(MAPI_E_NOT_INITIALIZED);
		RETURN_ERR_CASE(MAPI_E_NON_STANDARD);
		RETURN_ERR_CASE(MAPI_E_NO_RECIPIENTS);
		RETURN_ERR_CASE(MAPI_E_SUBMITTED);
		RETURN_ERR_CASE(MAPI_E_HAS_FOLDERS);
		RETURN_ERR_CASE(MAPI_E_HAS_MESSAGES);
		RETURN_ERR_CASE(MAPI_E_FOLDER_CYCLE);

		RETURN_ERR_CASE(MAPI_E_LOCKID_LIMIT);
		RETURN_ERR_CASE(MAPI_E_NAMED_PROP_QUOTA_EXCEEDED);

		RETURN_ERR_CASE(MAPI_W_PARTIAL_COMPLETION);

		/* Address Book specific errors and warnings */

		RETURN_ERR_CASE(MAPI_E_AMBIGUOUS_RECIP);

		RETURN_ERR_CASE(MAPI_E_PROFILE_DELETED);

		/*StrSafe.h error codes: */
		RETURN_ERR_CASE(STRSAFE_E_INSUFFICIENT_BUFFER);
		// STRSAFE_E_INVALID_PARAMETER == MAPI_E_INVALID_PARAMETER, so already handled
		RETURN_ERR_CASE(STRSAFE_E_END_OF_FILE);

		/*IStorage errors*/
		RETURN_ERR_CASE(STG_E_INVALIDFUNCTION);
		RETURN_ERR_CASE(STG_E_FILENOTFOUND);
		RETURN_ERR_CASE(STG_E_PATHNOTFOUND);
		RETURN_ERR_CASE(STG_E_TOOMANYOPENFILES);
		RETURN_ERR_CASE(STG_E_ACCESSDENIED);
		RETURN_ERR_CASE(STG_E_INVALIDHANDLE);
		RETURN_ERR_CASE(STG_E_INSUFFICIENTMEMORY);
		RETURN_ERR_CASE(STG_E_INVALIDPOINTER);
		RETURN_ERR_CASE(STG_E_NOMOREFILES);
		RETURN_ERR_CASE(STG_E_DISKISWRITEPROTECTED);
		RETURN_ERR_CASE(STG_E_SEEKERROR);
		RETURN_ERR_CASE(STG_E_WRITEFAULT);
		RETURN_ERR_CASE(STG_E_READFAULT);
		RETURN_ERR_CASE(STG_E_SHAREVIOLATION);
		RETURN_ERR_CASE(STG_E_LOCKVIOLATION);
		RETURN_ERR_CASE(STG_E_FILEALREADYEXISTS);
		RETURN_ERR_CASE(STG_E_INVALIDPARAMETER);
		RETURN_ERR_CASE(STG_E_MEDIUMFULL);
		RETURN_ERR_CASE(STG_E_PROPSETMISMATCHED);
		RETURN_ERR_CASE(STG_E_ABNORMALAPIEXIT);
		RETURN_ERR_CASE(STG_E_INVALIDHEADER);
		RETURN_ERR_CASE(STG_E_INVALIDNAME);
		RETURN_ERR_CASE(STG_E_UNKNOWN);
		RETURN_ERR_CASE(STG_E_UNIMPLEMENTEDFUNCTION);
		RETURN_ERR_CASE(STG_E_INVALIDFLAG);
		RETURN_ERR_CASE(STG_E_INUSE);
		RETURN_ERR_CASE(STG_E_NOTCURRENT);
		RETURN_ERR_CASE(STG_E_REVERTED);
		RETURN_ERR_CASE(STG_E_CANTSAVE);
		RETURN_ERR_CASE(STG_E_OLDFORMAT);
		RETURN_ERR_CASE(STG_E_OLDDLL);
		RETURN_ERR_CASE(STG_E_SHAREREQUIRED);
		RETURN_ERR_CASE(STG_E_NOTFILEBASEDSTORAGE);
		RETURN_ERR_CASE(STG_E_EXTANTMARSHALLINGS);
		RETURN_ERR_CASE(STG_E_DOCFILECORRUPT);
		RETURN_ERR_CASE(STG_E_BADBASEADDRESS);
		RETURN_ERR_CASE(STG_E_DOCFILETOOLARGE);
		RETURN_ERR_CASE(STG_E_NOTSIMPLEFORMAT);
		RETURN_ERR_CASE(STG_E_INCOMPLETE);
		RETURN_ERR_CASE(STG_E_TERMINATED);
		RETURN_ERR_CASE(STG_S_CONVERTED);
		RETURN_ERR_CASE(STG_S_BLOCK);
		RETURN_ERR_CASE(STG_S_RETRYNOW);
		RETURN_ERR_CASE(STG_S_MONITORING);
		RETURN_ERR_CASE(STG_S_MULTIPLEOPENS);
		RETURN_ERR_CASE(STG_S_CANNOTCONSOLIDATE);

		// COM errors (for CLSIDFromString)
		RETURN_ERR_CASE(CO_E_CLASSSTRING);
		RETURN_ERR_CASE(REGDB_E_WRITEREGDB);
		RETURN_ERR_CASE(REGDB_E_CLASSNOTREG);

		RETURN_ERR_CASE(E_ACCT_NOT_FOUND);
		RETURN_ERR_CASE(E_ACCT_WRONG_SORT_ORDER);
		RETURN_ERR_CASE(E_OLK_REGISTRY);
		RETURN_ERR_CASE(E_OLK_ALREADY_INITIALIZED);
		RETURN_ERR_CASE(E_OLK_NOT_INITIALIZED);
		RETURN_ERR_CASE(E_OLK_PARAM_NOT_SUPPORTED);
		RETURN_ERR_CASE(E_OLK_PROP_READ_ONLY);

		RETURN_ERR_CASE(SYNC_E_OBJECT_DELETED);
		RETURN_ERR_CASE(SYNC_E_IGNORE);
		RETURN_ERR_CASE(SYNC_E_CONFLICT);
		RETURN_ERR_CASE(SYNC_E_NO_PARENT);
		RETURN_ERR_CASE(SYNC_E_CYCLE);
		RETURN_ERR_CASE(SYNC_E_UNSYNCHRONIZED);
		RETURN_ERR_CASE(SYNC_W_PROGRESS);
		RETURN_ERR_CASE(SYNC_W_CLIENT_CHANGE_NEWER);

		RETURN_ERR_CASE(OLEOBJ_S_CANNOT_DOVERB_NOW);
default:
		{
			HRESULT hRes = S_OK;
			static TCHAR szErrCode[35]; // see string on default

			EC_H(StringCchPrintf(szErrCode, CCH(szErrCode), _T("0x%08X"), hrErr)); // STRING_OK

			return(szErrCode);
		}
	}
} // ErrorNameFromErrorCode

void PrintSkipNote(HRESULT hRes,LPCSTR szFunc)
{
	DebugPrint(DBGHRes,
		_T("Skipping %hs because hRes = 0x%8x = %s.\n"), // STRING_OK
		szFunc,
		hRes,
		ErrorNameFromErrorCode(hRes));
}