// MAPIfunctions.cpp : Collection of useful MAPI functions

#include "stdafx.h"
#include "Error.h"

#include "MAPIFunctions.h"
#include "MAPIStoreFunctions.h"
#include "MAPIABFunctions.h"
#include "InterpretProp.h"
#include "InterpretProp2.h"

#include "Registry.h"
#include "ImportProcs.h"
#include "ExtraPropTags.h"
#include "MAPIProgress.h"
#include "Guids.h"

//I don't use MAPIOID.h, which is needed to deal with PR_ATTACH_TAG, but if I did, here's how to include it
/*
#include <mapiguid.h>
#define USES_OID_TNEF
#define USES_OID_OLE
#define USES_OID_OLE1
#define USES_OID_OLE1_STORAGE
#define USES_OID_OLE2
#define USES_OID_OLE2_STORAGE
#define USES_OID_MAC_BINARY
#define USES_OID_MIMETAG
#define INITOID
//Major hack to get MAPIOID to compile
#define _MAC
#include <MAPIOID.h>
#undef _MAC
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

HRESULT CallOpenEntry(
						LPMDB lpMDB,
						LPADRBOOK lpAB,
						LPMAPICONTAINER lpContainer,
						LPMAPISESSION lpMAPISession,
						ULONG cbEntryID,
						LPENTRYID lpEntryID,
						LPCIID lpInterface,
						ULONG ulFlags,
						ULONG* ulObjTypeRet,//optional - can be NULL
						LPUNKNOWN* lppUnk)//required
{
	if (!lppUnk) return MAPI_E_INVALID_PARAMETER;
	HRESULT			hRes = S_OK;
	ULONG			ulObjType = NULL;
	LPUNKNOWN		lpUnk = NULL;
	ULONG			ulNoCacheFlags = NULL;

	*lppUnk = NULL;

	if (RegKeys[regKeyMAPI_NO_CACHE].ulCurDWORD)
	{
		ulFlags |= MAPI_NO_CACHE;
	}

	//in case we need to retry without MAPI_NO_CACHE - do not add MAPI_NO_CACHE to ulFlags after this point
	if (MAPI_NO_CACHE & ulFlags) ulNoCacheFlags = ulFlags & ~MAPI_NO_CACHE;

	if (lpInterface && fIsSet(DBGGeneric))
	{
		LPTSTR szGuid = GUIDToStringAndName(lpInterface);
		if (szGuid)
		{
			DebugPrint(DBGGeneric,_T("CallOpenEntry: OpenEntry asking for %s\n"),szGuid);
			delete[] szGuid;
		}
	}

	if (lpMDB)
	{
		DebugPrint(DBGGeneric,_T("CallOpenEntry: Calling OpenEntry on MDB with ulFlags = 0x%X\n"),ulFlags);
		WC_H(lpMDB->OpenEntry(
			cbEntryID,
			lpEntryID,
			lpInterface,
			ulFlags,
			&ulObjType,
			&lpUnk));
		if (MAPI_E_UNKNOWN_FLAGS == hRes && ulNoCacheFlags)
		{
			hRes = S_OK;
			if (lpUnk) (lpUnk)->Release();
			lpUnk = NULL;
			WC_H(lpMDB->OpenEntry(
				cbEntryID,
				lpEntryID,
				lpInterface,
				ulNoCacheFlags,
				&ulObjType,
				&lpUnk));
		}
		if (FAILED(hRes))
		{
			if (lpUnk) (lpUnk)->Release();
			lpUnk = NULL;
		}
	}
	if (lpAB && !lpUnk)
	{
		hRes = S_OK;
		DebugPrint(DBGGeneric,_T("CallOpenEntry: Calling OpenEntry on AB with ulFlags = 0x%X\n"),ulFlags);
		WC_H(lpAB->OpenEntry(
			cbEntryID,
			lpEntryID,
			NULL,//no interface
			ulFlags,
			&ulObjType,
			&lpUnk));
		if (MAPI_E_UNKNOWN_FLAGS == hRes && ulNoCacheFlags)
		{
			hRes = S_OK;
			if (lpUnk) (lpUnk)->Release();
			lpUnk = NULL;
			WC_H(lpAB->OpenEntry(
				cbEntryID,
				lpEntryID,
				NULL,//no interface
				ulNoCacheFlags,
				&ulObjType,
				&lpUnk));
		}
		if (FAILED(hRes))
		{
			if (lpUnk) (lpUnk)->Release();
			lpUnk = NULL;
		}
	}

	if (lpContainer && !lpUnk)
	{
		hRes = S_OK;
		DebugPrint(DBGGeneric,_T("CallOpenEntry: Calling OpenEntry on Container with ulFlags = 0x%X\n"),ulFlags);
		WC_H(lpContainer->OpenEntry(
			cbEntryID,
			lpEntryID,
			lpInterface,
			ulFlags,
			&ulObjType,
			&lpUnk));
		if (MAPI_E_UNKNOWN_FLAGS == hRes && ulNoCacheFlags)
		{
			hRes = S_OK;
			if (lpUnk) (lpUnk)->Release();
			lpUnk = NULL;
			WC_H(lpContainer->OpenEntry(
				cbEntryID,
				lpEntryID,
				lpInterface,
				ulNoCacheFlags,
				&ulObjType,
				&lpUnk));
		}
		if (FAILED(hRes))
		{
			if (lpUnk) (lpUnk)->Release();
			lpUnk = NULL;
		}
	}

	if (lpMAPISession && !lpUnk)
	{
		hRes = S_OK;
		DebugPrint(DBGGeneric,_T("CallOpenEntry: Calling OpenEntry on Session with ulFlags = 0x%X\n"),ulFlags);
		WC_H(lpMAPISession->OpenEntry(
			cbEntryID,
			lpEntryID,
			lpInterface,
			ulFlags,
			&ulObjType,
			&lpUnk));
		if (MAPI_E_UNKNOWN_FLAGS == hRes && ulNoCacheFlags)
		{
			hRes = S_OK;
			if (lpUnk) (lpUnk)->Release();
			lpUnk = NULL;
			WC_H(lpMAPISession->OpenEntry(
				cbEntryID,
				lpEntryID,
				lpInterface,
				ulNoCacheFlags,
				&ulObjType,
				&lpUnk));
		}
		if (FAILED(hRes))
		{
			if (lpUnk) (lpUnk)->Release();
			lpUnk = NULL;
		}
	}

	if (lpUnk)
	{
		LPTSTR szFlags = NULL;
		EC_H(InterpretFlags(PROP_ID(PR_OBJECT_TYPE), ulObjType, &szFlags));
		DebugPrint(DBGGeneric,_T("OnOpenEntryID: Got object (0x%08X) of type 0x%08X = %s\n"),lpUnk,ulObjType,szFlags);
		delete[] szFlags;
		szFlags = NULL;
		*lppUnk = lpUnk;
	}
	if (ulObjTypeRet) *ulObjTypeRet = ulObjType;
	return hRes;
}

HRESULT CallOpenEntry(
								  LPMDB lpMDB,
								  LPADRBOOK lpAB,
								  LPMAPICONTAINER lpContainer,
								  LPMAPISESSION lpMAPISession,
								  LPSBinary lpSBinary,
								  LPCIID lpInterface,
								  ULONG ulFlags,
								  ULONG* ulObjTypeRet,
								  LPUNKNOWN* lppUnk)
{
	HRESULT			hRes = S_OK;
	WC_H(CallOpenEntry(
		lpMDB,
		lpAB,
		lpContainer,
		lpMAPISession,
		lpSBinary?lpSBinary->cb:0,
		(LPENTRYID)(lpSBinary?lpSBinary->lpb:0),
		lpInterface,
		ulFlags,
		ulObjTypeRet,
		lppUnk));
	return hRes;
}

//Concatenate two property arrays without duplicates
HRESULT ConcatSPropTagArrays(
								  LPSPropTagArray lpArray1,
								  LPSPropTagArray lpArray2,
								  LPSPropTagArray *lpNewArray)
{
	if (!lpNewArray) return MAPI_E_INVALID_PARAMETER;
	HRESULT hRes = S_OK;
	ULONG iSourceArray = 0;
	ULONG iTargetArray = 0;
	ULONG iNewArraySize = 0;
	LPSPropTagArray lpLocalArray = NULL;

	*lpNewArray = NULL;

	//Add the sizes of the passed in arrays (0 if they were NULL)
	iNewArraySize = (lpArray1?lpArray1->cValues:0);

	if (lpArray2 && lpArray1)
	{
		for (iSourceArray = 0; iSourceArray < lpArray2->cValues; iSourceArray++)
		{
			if (!IsDuplicateProp(lpArray1, lpArray2->aulPropTag[iSourceArray]))
			{
				iNewArraySize++;
			}
		}
	}
	else
	{
		iNewArraySize = iNewArraySize + (lpArray2?lpArray2->cValues:0);
	}

	if (!iNewArraySize) return MAPI_E_CALL_FAILED;

	//Allocate memory for the new prop tag array
	EC_H(MAPIAllocateBuffer(
		CbNewSPropTagArray(iNewArraySize),
		(LPVOID*) &lpLocalArray));

	if (lpLocalArray)
	{
		iTargetArray = 0;
		if (lpArray1)
		{
			for (iSourceArray = 0;iSourceArray<lpArray1->cValues;iSourceArray++)
			{
				if (PROP_TYPE(lpArray1->aulPropTag[iSourceArray]) != PT_NULL)//ditch bad props
				{
					lpLocalArray->aulPropTag[iTargetArray++] = lpArray1->aulPropTag[iSourceArray];
				}
			}
		}
		if (lpArray2)
		{
			for (iSourceArray = 0;iSourceArray<lpArray2->cValues;iSourceArray++)
			{
				if (PROP_TYPE(lpArray2->aulPropTag[iSourceArray]) != PT_NULL)//ditch bad props
				{
					if (!IsDuplicateProp(lpArray1, lpArray2->aulPropTag[iSourceArray]))
					{
						lpLocalArray->aulPropTag[iTargetArray++] = lpArray2->aulPropTag[iSourceArray];
					}
				}
			}
		}

		// <= since we may have thrown some PT_NULL tags out - just make sure we didn't overrun.
		EC_H((iTargetArray <= iNewArraySize)?S_OK:MAPI_E_CALL_FAILED);

		//since we may have ditched some tags along the way, reset our size
		lpLocalArray->cValues = iTargetArray;

		if (FAILED(hRes))
		{
			MAPIFreeBuffer(lpLocalArray);
		}
		else
		{
			*lpNewArray = (LPSPropTagArray) lpLocalArray;
		}
	}

	return hRes;
}//ConcatSPropTagArrays

//May not behave correctly if lpSrcFolder == lpDestFolder
//We can check that the pointers aren't equal, but they could be different
//and still refer to the same folder.
HRESULT CopyFolderContents(LPMAPIFOLDER lpSrcFolder, LPMAPIFOLDER lpDestFolder, BOOL bCopyAssociatedContents, BOOL bMove, BOOL bSingleCall, HWND hWnd)
{
	HRESULT			hRes = S_OK;
	LPMAPITABLE		lpSrcContents = NULL;
	LPSRowSet		pRows = NULL;
	ULONG			ulRowsCopied = 0;

	enum {fldPR_ENTRYID,
		fldNUM_COLS};

	static SizedSPropTagArray(fldNUM_COLS,fldCols) = {fldNUM_COLS,
		PR_ENTRYID,
	};

	DebugPrint(DBGGeneric,_T("CopyFolderContents: lpSrcFolder = 0x%08X, lpDestFolder = 0x%08X, bCopyAssociatedContents = %d, bMove = %d\n"),
		lpSrcFolder,
		lpDestFolder,
		bCopyAssociatedContents,
		bMove
		);

	if (!lpSrcFolder || !lpDestFolder) return MAPI_E_INVALID_PARAMETER;

	EC_H(lpSrcFolder->GetContentsTable(
		fMapiUnicode | (bCopyAssociatedContents?MAPI_ASSOCIATED:NULL),
		&lpSrcContents));

	if (lpSrcContents)
	{
		EC_H(lpSrcContents->SetColumns((LPSPropTagArray)&fldCols, TBL_BATCH));

		ULONG			ulRowCount = 0;
		EC_H(lpSrcContents->GetRowCount(0,&ulRowCount));

		if (bSingleCall && ulRowCount < ULONG_MAX/sizeof(SBinary))
		{
			SBinaryArray sbaEID = {0};
			sbaEID.cValues = ulRowCount;
			EC_H(MAPIAllocateBuffer(sizeof(SBinary) * ulRowCount,(LPVOID*) &sbaEID.lpbin));
			ZeroMemory(sbaEID.lpbin, sizeof(SBinary) * ulRowCount);

			if (!FAILED(hRes)) for (ulRowsCopied = 0; ulRowsCopied < ulRowCount;ulRowsCopied++)
			{
				hRes = S_OK;
				if (pRows) FreeProws(pRows);
				pRows = NULL;
				EC_H(lpSrcContents->QueryRows(
					1,
					NULL,
					&pRows));
				if (FAILED(hRes) || !pRows || (pRows && !pRows->cRows)) break;

				if (pRows && PT_ERROR != PROP_TYPE(pRows->aRow->lpProps[fldPR_ENTRYID].ulPropTag))
				{
					EC_H(CopySBinary(&sbaEID.lpbin[ulRowsCopied],&pRows->aRow->lpProps[fldPR_ENTRYID].Value.bin,sbaEID.lpbin));
				}
			}

			LPMAPIPROGRESS lpProgress = GetMAPIProgress(_T("IMAPIFolder::CopyMessages"), hWnd);// STRING_OK

			ULONG ulCopyFlags = bMove ? MESSAGE_MOVE : 0;

			if(lpProgress)
				ulCopyFlags |= MESSAGE_DIALOG;

			EC_H(lpSrcFolder->CopyMessages(
				&sbaEID,
				&IID_IMAPIFolder,
				lpDestFolder,
				lpProgress ? (ULONG_PTR)hWnd : NULL,
				lpProgress,
				ulCopyFlags));
			MAPIFreeBuffer(sbaEID.lpbin);

			if(lpProgress)
				lpProgress->Release();

			lpProgress = NULL;
		}
		else
		{
			if (!FAILED(hRes)) for (ulRowsCopied = 0; ulRowsCopied < ulRowCount;ulRowsCopied++)
			{
				hRes = S_OK;
				if (pRows) FreeProws(pRows);
				pRows = NULL;
				EC_H(lpSrcContents->QueryRows(
					1,
					NULL,
					&pRows));
				if (FAILED(hRes) || !pRows || (pRows && !pRows->cRows)) break;

				if (pRows && PT_ERROR != PROP_TYPE(pRows->aRow->lpProps[fldPR_ENTRYID].ulPropTag))
				{
					SBinaryArray sbaEID = {0};
					DebugPrint(DBGGeneric,_T("Source Message =\n"));
					DebugPrintBinary(DBGGeneric,&pRows->aRow->lpProps[fldPR_ENTRYID].Value.bin);

					sbaEID.cValues = 1;
					sbaEID.lpbin = &pRows->aRow->lpProps[fldPR_ENTRYID].Value.bin;

					LPMAPIPROGRESS lpProgress = GetMAPIProgress(_T("IMAPIFolder::CopyMessages"), hWnd);// STRING_OK

					ULONG ulCopyFlags = bMove ? MESSAGE_MOVE : 0;

					if(lpProgress)
						ulCopyFlags |= MESSAGE_DIALOG;

					EC_H(lpSrcFolder->CopyMessages(
						&sbaEID,
						&IID_IMAPIFolder,
						lpDestFolder,
						lpProgress ? (ULONG_PTR)hWnd : NULL,
						lpProgress,
						ulCopyFlags));

					if (S_OK == hRes) DebugPrint(DBGGeneric,_T("Message Copied\n"));

					if(lpProgress)
						lpProgress->Release();

					lpProgress = NULL;
				}

				if (S_OK != hRes) DebugPrint(DBGGeneric,_T("Message Copy Failed\n"));
			}
		}
		lpSrcContents->Release();
	}

	if (pRows) FreeProws(pRows);
	return hRes;
}//CopyFolderContents

HRESULT CopyFolderRules(LPMAPIFOLDER lpSrcFolder, LPMAPIFOLDER lpDestFolder,BOOL bReplace)
{
	if (!lpSrcFolder || !lpDestFolder) return MAPI_E_INVALID_PARAMETER;
	HRESULT					hRes = S_OK;
	LPEXCHANGEMODIFYTABLE	lpSrcTbl = NULL;
	LPEXCHANGEMODIFYTABLE	lpDstTbl = NULL;

	EC_H(lpSrcFolder->OpenProperty(
		PR_RULES_TABLE,
		(LPGUID)&IID_IExchangeModifyTable,
		0,
		MAPI_DEFERRED_ERRORS,
		(LPUNKNOWN FAR *)&lpSrcTbl));

	EC_H(lpDestFolder->OpenProperty(
		PR_RULES_TABLE,
		(LPGUID)&IID_IExchangeModifyTable,
		0,
		MAPI_DEFERRED_ERRORS,
		(LPUNKNOWN FAR *)&lpDstTbl));

	if (lpSrcTbl && lpDstTbl)
	{
		LPMAPITABLE lpTable = NULL;
		lpSrcTbl->GetTable(0,&lpTable);

		if (lpTable)
		{
			static SizedSPropTagArray(9,ruleTags) = {9,
				PR_RULE_ACTIONS,
				PR_RULE_CONDITION,
				PR_RULE_LEVEL,
				PR_RULE_NAME,
				PR_RULE_PROVIDER,
				PR_RULE_PROVIDER_DATA,
				PR_RULE_SEQUENCE,
				PR_RULE_STATE,
				PR_RULE_USER_FLAGS,
			};

			EC_H(lpTable->SetColumns((LPSPropTagArray)&ruleTags,0));

			LPSRowSet lpRows = NULL;

			EC_H(HrQueryAllRows(
				lpTable,
				NULL,
				NULL,
				NULL,
				NULL,
				&lpRows));

			if (lpRows && lpRows->cRows < MAXNewROWLIST)
			{
				LPROWLIST lpTempList = NULL;

				EC_H(MAPIAllocateBuffer(CbNewROWLIST(lpRows->cRows),(LPVOID*) &lpTempList));

				if (lpTempList)
				{
					lpTempList->cEntries = lpRows->cRows;
					ULONG iArrayPos = 0;

					for(iArrayPos = 0 ; iArrayPos < lpRows->cRows ; iArrayPos++)
					{
						lpTempList->aEntries[iArrayPos].ulRowFlags = ROW_ADD;
						EC_H(MAPIAllocateMore(
							lpRows->aRow[iArrayPos].cValues * sizeof(SPropValue),
							lpTempList,
							(LPVOID*) &lpTempList->aEntries[iArrayPos].rgPropVals));
						if (SUCCEEDED(hRes) && lpTempList->aEntries[iArrayPos].rgPropVals)
						{
							ULONG ulSrc = 0;
							ULONG ulDst = 0;
							for (ulSrc = 0; ulSrc < lpRows->aRow[iArrayPos].cValues; ulSrc++)
							{
								if (lpRows->aRow[iArrayPos].lpProps[ulSrc].ulPropTag == PR_RULE_PROVIDER_DATA)
								{
									if (!lpRows->aRow[iArrayPos].lpProps[ulSrc].Value.bin.cb ||
										!lpRows->aRow[iArrayPos].lpProps[ulSrc].Value.bin.lpb)
									{
										// PR_RULE_PROVIDER_DATA was NULL - we don't want this
										continue;
									}
								}

								// This relies on our augmented PropCopyMore that understands PT_SRESTRICTION and PT_ACTIONS
								EC_H(PropCopyMore(
									&lpTempList->aEntries[iArrayPos].rgPropVals[ulDst],
									&lpRows->aRow[iArrayPos].lpProps[ulSrc],
									MAPIAllocateMore,
									lpTempList));
								ulDst++;
							}
							lpTempList->aEntries[iArrayPos].cValues = ulDst;
						}
					}
					ULONG ulFlags = 0;
					if (bReplace) ulFlags = ROWLIST_REPLACE;

					EC_H(lpDstTbl->ModifyTable(ulFlags,lpTempList));

					MAPIFreeBuffer(lpTempList);
				}
				MAPIFreeBuffer(lpRows);
			}
			lpTable->Release();
		}
	}

	if (lpDstTbl) lpDstTbl->Release();
	if (lpSrcTbl) lpSrcTbl->Release();
	return hRes;
}


HRESULT	CopyPropertyAsStream(LPMAPIPROP lpSourcePropObj,
							 LPMAPIPROP lpTargetPropObj,
							 ULONG ulSourceTag,
							 ULONG ulTargetTag)
{
	HRESULT			hRes = S_OK;
	LPSTREAM		lpStmSource = NULL;
	LPSTREAM		lpStmTarget = NULL;
	LARGE_INTEGER	li;
	ULARGE_INTEGER	uli;
	ULARGE_INTEGER	ulBytesRead;
	ULARGE_INTEGER	ulBytesWritten;

	if (!lpSourcePropObj || !lpTargetPropObj || !ulSourceTag || !ulTargetTag) return MAPI_E_INVALID_PARAMETER;
	if (PROP_TYPE(ulSourceTag) != PROP_TYPE(ulTargetTag)) return MAPI_E_INVALID_PARAMETER;

	EC_H(lpSourcePropObj->OpenProperty(
		ulSourceTag,
		&IID_IStream,
		STGM_READ | STGM_DIRECT,
		NULL,
		(LPUNKNOWN *)&lpStmSource));

	EC_H(lpTargetPropObj->OpenProperty(
		ulTargetTag,
		&IID_IStream,
		STGM_READWRITE | STGM_DIRECT,
		MAPI_CREATE | MAPI_MODIFY,
		(LPUNKNOWN *)&lpStmTarget));

	if (lpStmSource && lpStmTarget)
	{
		li.QuadPart = 0;
		uli.QuadPart = MAXLONGLONG;

		EC_H(lpStmSource->Seek(li,STREAM_SEEK_SET,NULL));

		EC_H(lpStmTarget->Seek(li,STREAM_SEEK_SET,NULL));

		EC_H(lpStmSource->CopyTo(lpStmTarget,uli,&ulBytesRead,&ulBytesWritten));

		//This may not be necessary since we opened with STGM_DIRECT
		EC_H(lpStmTarget->Commit(STGC_DEFAULT));

		//leave it to the caller to do this
//		EC_H(lpTargetPropObj->SaveChanges(KEEP_OPEN_READWRITE));
	}

	if (lpStmTarget) lpStmTarget->Release();
	if (lpStmSource) lpStmSource->Release();
	return hRes;
}//CopyPropertyAsStream

///////////////////////////////////////////////////////////////////////////////
//	CopySBinary()
//
//	Parameters
//		psbDest - Address of the destination binary
//		psbSrc  - Address of the source binary
//		lpParent - Pointer to parent object (not, however, pointer to pointer!)
//
//	Purpose
//	  Allocates a new SBinary and copies psbSrc into it
//
HRESULT CopySBinary(LPSBinary psbDest,const LPSBinary psbSrc, LPVOID lpParent)
{
	HRESULT	 hRes = S_OK;

	if (!psbDest || !psbSrc) return MAPI_E_INVALID_PARAMETER;

	psbDest->cb = psbSrc->cb;

	if (psbSrc->cb)
	{
		if (lpParent)
			EC_H(MAPIAllocateMore(
					psbSrc->cb,
					lpParent,
					(LPVOID *) &psbDest->lpb))
		else
			EC_H(MAPIAllocateBuffer(
					psbSrc->cb,
					(LPVOID *) &psbDest->lpb));
		if (S_OK == hRes)
			CopyMemory(psbDest->lpb,psbSrc->lpb,psbSrc->cb);
	}

	return hRes;
}//CopySBinary

///////////////////////////////////////////////////////////////////////////////
//	CopyString()
//
//	Parameters
//		lpszDestination - Address of pointer to destination string
//		szSource		- Pointer to source string
//		lpParent - Pointer to parent object (not, however, pointer to pointer!)
//
//	Purpose
//	  Uses MAPI to allocate a new string (szDestination) and copy szSource into it
//		Uses lpParent as the parent for MAPIAllocateMore if possible
//
HRESULT CopyStringA(LPSTR* lpszDestination,LPCSTR szSource, LPVOID pParent)
{
	HRESULT	hRes = S_OK;
	size_t	cbSource = 0;

	if (!szSource)
	{
		*lpszDestination = NULL;
		return hRes;
	}

	EC_H(StringCbLengthA(szSource,STRSAFE_MAX_CCH * sizeof(char),&cbSource));
	cbSource += sizeof(char);

	if (pParent)
	{
		EC_H(MAPIAllocateMore(
			(ULONG) cbSource,
			pParent,
			(LPVOID*) lpszDestination));
	}
	else
	{
		EC_H(MAPIAllocateBuffer(
			(ULONG) cbSource,
			(LPVOID*) lpszDestination));
	}
	EC_H(StringCbCopyA(*lpszDestination, cbSource, szSource));

	return hRes;
}//CopyStringA

HRESULT CopyStringW(LPWSTR* lpszDestination,LPCWSTR szSource, LPVOID pParent)
{
	HRESULT	hRes = S_OK;
	size_t	cbSource = 0;

	if (!szSource)
	{
		*lpszDestination = NULL;
		return hRes;
	}

	EC_H(StringCbLengthW(szSource,STRSAFE_MAX_CCH * sizeof(WCHAR),&cbSource));
	cbSource += sizeof(WCHAR);

	if (pParent)
	{
		EC_H(MAPIAllocateMore(
			(ULONG) cbSource,
			pParent,
			(LPVOID*) lpszDestination));
	}
	else
	{
		EC_H(MAPIAllocateBuffer(
			(ULONG) cbSource,
			(LPVOID*) lpszDestination));
	}
	EC_H(StringCbCopyW(*lpszDestination, cbSource, szSource));

	return hRes;
}//CopyStringW

//Allocates and creates a restriction that looks for existence of
//a particular property that matches the given string
//If lpParent is passed in, it is used as the allocation parent.
HRESULT CreatePropertyStringRestriction(ULONG ulPropTag,
										LPCTSTR szString,
										ULONG ulFuzzyLevel,
										LPVOID lpParent,
										LPSRestriction* lppRes)
{
	HRESULT hRes = S_OK;
	LPSRestriction	lpRes = NULL;
	LPSRestriction	lpResLevel1 = NULL;
	LPSPropValue	lpspvSubject = NULL;
	LPVOID			lpAllocationParent = NULL;

	*lppRes = NULL;

	if (!szString) return MAPI_E_INVALID_PARAMETER;

	//Allocate and create our SRestriction
	//Allocate base memory:
	if (lpParent)
	{
		EC_H(MAPIAllocateMore(
			sizeof(SRestriction),
			lpParent,
			(LPVOID*)&lpRes));

		lpAllocationParent = lpParent;
	}
	else
	{
		EC_H(MAPIAllocateBuffer(
			sizeof(SRestriction),
			(LPVOID*)&lpRes));

		lpAllocationParent = lpRes;
	}

	EC_H(MAPIAllocateMore(
		sizeof(SRestriction)*2,
		lpAllocationParent,
		(LPVOID*)&lpResLevel1));

	EC_H(MAPIAllocateMore(
		sizeof(SPropValue),
		lpAllocationParent,
		(LPVOID*)&lpspvSubject));

	if (lpRes && lpResLevel1 && lpspvSubject)
	{
		// Zero out allocated memory.
		ZeroMemory(lpRes, sizeof(SRestriction));
		ZeroMemory(lpResLevel1, sizeof(SRestriction)*2);
		ZeroMemory(lpspvSubject, sizeof(SPropValue));

		//Root Node
		lpRes->rt = RES_AND;
		lpRes->res.resAnd.cRes = 2;
		lpRes->res.resAnd.lpRes = lpResLevel1;

		lpResLevel1[0].rt = RES_EXIST;
		lpResLevel1[0].res.resExist.ulPropTag = ulPropTag;
		lpResLevel1[0].res.resExist.ulReserved1 = 0;
		lpResLevel1[0].res.resExist.ulReserved2 = 0;

		lpResLevel1[1].rt = RES_CONTENT;
		lpResLevel1[1].res.resContent.ulPropTag = ulPropTag;
		lpResLevel1[1].res.resContent.ulFuzzyLevel = ulFuzzyLevel;
		lpResLevel1[1].res.resContent.lpProp = lpspvSubject;

		//Allocate and fill out properties:
		lpspvSubject->ulPropTag = ulPropTag;

		EC_H(CopyString(
			&lpspvSubject->Value.LPSZ,
			szString,
			lpAllocationParent));

		DebugPrint(DBGGeneric,_T("CreatePropertyStringRestriction built restriction:\n"));
		DebugPrintRestriction(DBGGeneric,lpRes,NULL);

		*lppRes = lpRes;
	}

	if (hRes != S_OK)
	{
		DebugPrint(DBGGeneric,_T("Failed to create restriction\n"));
		MAPIFreeBuffer(lpRes);
		*lppRes = NULL;
	}
	return hRes;
}//CreatePropertyStringRestriction

HRESULT CreateRangeRestriction(ULONG ulPropTag,
							   LPCTSTR szString,
							   LPVOID lpParent,
							   LPSRestriction* lppRes)
{
	HRESULT hRes = S_OK;
	LPSRestriction	lpRes = NULL;
	LPSRestriction	lpResLevel1 = NULL;
	LPSPropValue	lpspvSubject = NULL;
	LPVOID			lpAllocationParent = NULL;

	*lppRes = NULL;

	if (!szString) return MAPI_E_INVALID_PARAMETER;

	//Allocate and create our SRestriction
	//Allocate base memory:
	if (lpParent)
	{
		EC_H(MAPIAllocateMore(
			sizeof(SRestriction),
			lpParent,
			(LPVOID*)&lpRes));

		lpAllocationParent = lpParent;
	}
	else
	{
		EC_H(MAPIAllocateBuffer(
			sizeof(SRestriction),
			(LPVOID*)&lpRes));

		lpAllocationParent = lpRes;
	}

	EC_H(MAPIAllocateMore(
		sizeof(SRestriction)*2,
		lpAllocationParent,
		(LPVOID*)&lpResLevel1));

	EC_H(MAPIAllocateMore(
		sizeof(SPropValue),
		lpAllocationParent,
		(LPVOID*)&lpspvSubject));

	if (lpRes && lpResLevel1 && lpspvSubject)
	{
		// Zero out allocated memory.
		ZeroMemory(lpRes, sizeof(SRestriction));
		ZeroMemory(lpResLevel1, sizeof(SRestriction)*2);
		ZeroMemory(lpspvSubject, sizeof(SPropValue));

		//Root Node
		lpRes->rt = RES_AND;
		lpRes->res.resAnd.cRes = 2;
		lpRes->res.resAnd.lpRes = lpResLevel1;

		lpResLevel1[0].rt = RES_EXIST;
		lpResLevel1[0].res.resExist.ulPropTag = ulPropTag;
		lpResLevel1[0].res.resExist.ulReserved1 = 0;
		lpResLevel1[0].res.resExist.ulReserved2 = 0;

		lpResLevel1[1].rt = RES_PROPERTY;
		lpResLevel1[1].res.resProperty.ulPropTag = ulPropTag;
		lpResLevel1[1].res.resProperty.relop = RELOP_GE;
		lpResLevel1[1].res.resProperty.lpProp = lpspvSubject;

		//Allocate and fill out properties:
		lpspvSubject->ulPropTag = ulPropTag;

		EC_H(CopyString(
			&lpspvSubject->Value.LPSZ,
			szString,
			lpAllocationParent));

		DebugPrint(DBGGeneric,_T("CreateRangeRestriction built restriction:\n"));
		DebugPrintRestriction(DBGGeneric,lpRes,NULL);

		*lppRes = lpRes;
	}

	if (hRes != S_OK)
	{
		DebugPrint(DBGGeneric,_T("Failed to create restriction\n"));
		MAPIFreeBuffer(lpRes);
		*lppRes = NULL;
	}
	return hRes;
}

HRESULT DeleteProperty(LPMAPIPROP lpMAPIProp,ULONG ulPropTag)
{
	HRESULT hRes = S_OK;
	SPropTagArray		ptaTag;
	LPSPropProblemArray pProbArray = NULL;

	if (!lpMAPIProp) return MAPI_E_INVALID_PARAMETER;

	if (PT_ERROR == PROP_TYPE(ulPropTag))
		ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNSPECIFIED);

	ptaTag.cValues = 1;
	ptaTag.aulPropTag[0] = ulPropTag;

	DebugPrint(DBGGeneric,_T("DeleteProperty: Deleting prop 0x%08X from MAPI item 0x%X.\n"),ulPropTag,lpMAPIProp);

	EC_H(lpMAPIProp->DeleteProps(
		&ptaTag,
		&pProbArray));
	if (MAPI_E_NO_ACCESS == hRes)
	{
		CHECKHRESMSG(hRes,IDS_ACCESSDENIED);
	}
	else if (MAPI_E_NO_SUPPORT == hRes)
	{
		CHECKHRESMSG(hRes,IDS_PROPDELETIONNOTSUPPORTED);
	}

	if (S_OK == hRes && pProbArray)
	{
		EC_PROBLEMARRAY(pProbArray);
	}

	if (SUCCEEDED(hRes))
	{
		EC_H(lpMAPIProp->SaveChanges(KEEP_OPEN_READWRITE));
	}
	MAPIFreeBuffer(pProbArray);

	return hRes;
}//DeleteProperty

//Delete items to the wastebasket of the passed in mdb, if it exists.
HRESULT DeleteToDeletedItems(LPMDB lpMDB, LPMAPIFOLDER lpSourceFolder, LPENTRYLIST lpEIDs, HWND hWnd)
{
	HRESULT hRes = S_OK;
	ULONG cProps;
	LPSPropValue pProps = NULL;
	LPMAPIFOLDER lpWasteFolder = NULL;

	static SizedSPropTagArray(1, sptWastebasket) = {1,PR_IPM_WASTEBASKET_ENTRYID};

	if (!lpMDB || !lpSourceFolder || !lpEIDs) return MAPI_E_INVALID_PARAMETER;

	DebugPrint(DBGGeneric,_T("DeleteToDeletedItems: Deleting from folder 0x%X in store 0x%X\n"),
		lpSourceFolder,
		lpMDB);

	EC_H_GETPROPS(lpMDB->GetProps((LPSPropTagArray)&sptWastebasket,
		fMapiUnicode,
		&cProps,
		&pProps));

	if (pProps)
	{
		DebugPrint(DBGGeneric,_T("DeleteToDeletedItems: Messages will be copied to wastebasket. EID =\n"));
		DebugPrintBinary(DBGGeneric,&pProps[0].Value.bin);

		EC_H(CallOpenEntry(
			lpMDB,
			NULL,
			NULL,
			NULL,
			pProps[0].Value.bin.cb,
			(LPENTRYID)pProps[0].Value.bin.lpb,
			NULL,
			MAPI_MODIFY,
			NULL,
			(LPUNKNOWN*)&lpWasteFolder));

		LPMAPIPROGRESS lpProgress = GetMAPIProgress(_T("IMAPIFolder::CopyMessages"), hWnd);// STRING_OK

		ULONG ulCopyFlags = MESSAGE_MOVE;

		if(lpProgress)
			ulCopyFlags |= MESSAGE_DIALOG;

		EC_H(lpSourceFolder->CopyMessages(
			lpEIDs,
			NULL,//default interface
			lpWasteFolder,
			lpProgress ? (ULONG_PTR)hWnd : NULL,
			lpProgress,
			ulCopyFlags));

		if(lpProgress)
			lpProgress->Release();

		lpProgress = NULL;
	}

	if (lpWasteFolder) lpWasteFolder->Release();
	MAPIFreeBuffer(pProps);
	return hRes;
}//DeleteToDeletedItems

BOOL FindPropInPropTagArray(LPSPropTagArray lpspTagArray, ULONG ulPropToFind, ULONG* lpulRowFound)
{
	ULONG i = 0;
	*lpulRowFound = 0;
	if (!lpspTagArray) return FALSE;
	for (i = 0 ; i < lpspTagArray->cValues ; i++)
	{
		if (PROP_ID(ulPropToFind) == PROP_ID(lpspTagArray->aulPropTag[i]))
		{
			*lpulRowFound = i;
			return TRUE;
		}
	}
	return FALSE;
}//FindPropInPropTagArray

// See list of types (like MAPI_FOLDER) in mapidefs.h
ULONG GetMAPIObjectType(LPMAPIPROP lpMAPIProp)
{
	HRESULT hRes = S_OK;
	ULONG ulObjType = NULL;
	LPSPropValue lpProp = NULL;

	if (!lpMAPIProp) return 0; // 0's not a valid Object type

	WC_H(HrGetOneProp(
		lpMAPIProp,
		PR_OBJECT_TYPE,
		&lpProp));

	if (lpProp)
		ulObjType = lpProp->Value.ul;

	MAPIFreeBuffer(lpProp);
	return ulObjType;
}

HRESULT GetInbox(LPMDB lpMDB, LPMAPIFOLDER* lpInbox)
{
	HRESULT			hRes = S_OK;
	ULONG			cbInboxEID = NULL;
	LPENTRYID		lpInboxEID = NULL;

	DebugPrint(DBGGeneric, _T("GetInbox: getting Inbox from 0x%X\n"),lpMDB);

	*lpInbox = NULL;

	if (!lpMDB) return MAPI_E_INVALID_PARAMETER;

	EC_H(lpMDB->GetReceiveFolder(
		_T("IPM.Note"),// STRING_OK this is the class of message we want
		fMapiUnicode,//flags
		&cbInboxEID,//size and...
		(LPENTRYID *) &lpInboxEID,//value of entry ID
		NULL));//returns a message class if not NULL

	if (cbInboxEID && lpInboxEID)
	{
		//Get the Inbox...
		WC_H(CallOpenEntry(
			lpMDB,
			NULL,
			NULL,
			NULL,
			cbInboxEID,
			lpInboxEID,
			NULL,
			MAPI_BEST_ACCESS,
			NULL,
			(LPUNKNOWN*)lpInbox));
	}

	MAPIFreeBuffer(lpInboxEID);
	return hRes;
}

HRESULT GetParentFolder(LPMAPIFOLDER lpChildFolder, LPMDB lpMDB, LPMAPIFOLDER* lpParentFolder)
{
	HRESULT			hRes = S_OK;
	ULONG			cProps;
	LPSPropValue	lpProps = NULL;

	*lpParentFolder = NULL;

	if (!lpChildFolder) return MAPI_E_INVALID_PARAMETER;

	enum {PARENTEID,NUM_COLS};
	SizedSPropTagArray(NUM_COLS,sptaSrcFolder) = { NUM_COLS, {
		PR_PARENT_ENTRYID}
	};

	// Get PR_PARENT_ENTRYID
	EC_H_GETPROPS(lpChildFolder->GetProps(
		(LPSPropTagArray) &sptaSrcFolder,
		fMapiUnicode,
		&cProps,
		&lpProps));

	if (lpProps && PT_ERROR != PROP_TYPE(lpProps[PARENTEID].ulPropTag))
	{
		WC_H(CallOpenEntry(
			lpMDB,
			NULL,
			NULL,
			NULL,
			lpProps[PARENTEID].Value.bin.cb,
			(LPENTRYID) lpProps[PARENTEID].Value.bin.lpb,
			NULL,
			MAPI_BEST_ACCESS,
			NULL,
			(LPUNKNOWN*)lpParentFolder));
	}

	MAPIFreeBuffer(lpProps);
	return hRes;
}//GetParentFolder

HRESULT GetPropsNULL(LPMAPIPROP lpMAPIProp,ULONG ulFlags, ULONG * lpcValues, LPSPropValue *	lppPropArray)
{
	HRESULT hRes = S_OK;
	*lpcValues = NULL;
	*lppPropArray = NULL;

	if (!lpMAPIProp) return MAPI_E_INVALID_PARAMETER;
	LPSPropTagArray lpTags = NULL;
	if (RegKeys[regkeyUSE_GETPROPLIST].ulCurDWORD)
	{
		DebugPrint(DBGGeneric, _T("GetPropsNULL: Calling GetPropList on 0x%X\n"),lpMAPIProp);
		WC_H(lpMAPIProp->GetPropList(
			ulFlags,
			&lpTags));

		if (MAPI_E_BAD_CHARWIDTH == hRes)
		{
			hRes = S_OK;
			EC_H(lpMAPIProp->GetPropList(
				NULL,
				&lpTags));
		}
		else
		{
			CHECKHRESMSG(hRes,IDS_NOPROPLIST);
		}
	}
	else
	{
		DebugPrint(DBGGeneric, _T("GetPropsNULL: Calling GetProps(NULL) on 0x%X\n"),lpMAPIProp);
	}

	WC_H(lpMAPIProp->GetProps(
		lpTags,
		ulFlags,
		lpcValues,
		lppPropArray));
	MAPIFreeBuffer(lpTags);

	return hRes;
}

HRESULT GetSpecialFolder(LPMDB lpMDB, ULONG ulFolderPropTag, LPMAPIFOLDER *lpSpecialFolder)
{
	HRESULT			hRes = S_OK;
	LPMAPIFOLDER	lpInbox = NULL;

	*lpSpecialFolder = NULL;

	DebugPrint(DBGGeneric, _T("GetSpecialFolder: getting 0x%X from 0x%X\n"),ulFolderPropTag,lpMDB);

	if (!lpMDB) return MAPI_E_INVALID_PARAMETER;

	LPSPropValue lpProp = NULL;
	WC_H(GetInbox(lpMDB,&lpInbox));
	if (lpInbox)
	{
		WC_H_MSG(HrGetOneProp(lpInbox,ulFolderPropTag,&lpProp),
			IDS_GETSPECIALFOLDERINBOXMISSINGPROP);
		lpInbox->Release();
	}

	if (!lpProp)
	{
		hRes = S_OK;
		LPMAPIFOLDER lpRootFolder= NULL;
		// Open root container.
		EC_H(CallOpenEntry(
			lpMDB,
			NULL,
			NULL,
			NULL,
			NULL,//open root container
			NULL,
			MAPI_BEST_ACCESS,
			NULL,
			(LPUNKNOWN*)&lpRootFolder));
		if (lpRootFolder)
		{
			EC_H_MSG(HrGetOneProp(lpRootFolder,ulFolderPropTag,&lpProp),
				IDS_GETSPECIALFOLDERROOTMISSINGPROP);
			lpRootFolder->Release();
		}
	}

	if (lpProp && PT_BINARY == PROP_TYPE(lpProp->ulPropTag) && lpProp->Value.bin.cb)
	{
		WC_H(CallOpenEntry(
			lpMDB,
			NULL,
			NULL,
			NULL,
			lpProp->Value.bin.cb,
			(LPENTRYID) lpProp->Value.bin.lpb,
			NULL,
			MAPI_BEST_ACCESS,
			NULL,
			(LPUNKNOWN*)lpSpecialFolder));
	}
	if (MAPI_E_NOT_FOUND == hRes)
	{
		DebugPrint(DBGGeneric,_T("Special folder not found.\n"));
		if (*lpSpecialFolder) (*lpSpecialFolder)->Release();
		*lpSpecialFolder = NULL;
	}
	MAPIFreeBuffer(lpProp);
	return hRes;
}

HRESULT IsAttachmentBlocked(LPMAPISESSION lpMAPISession, LPCWSTR pwszFileName, BOOL* pfBlocked)
{
	if (!lpMAPISession || !pwszFileName || !pfBlocked) return MAPI_E_INVALID_PARAMETER;

	HRESULT hRes = S_OK;
	IAttachmentSecurity* lpAttachSec = NULL;
	BOOL bBlocked = false;

	EC_H(lpMAPISession->QueryInterface(IID_IAttachmentSecurity,(void**)&lpAttachSec));
	if (SUCCEEDED(hRes) && lpAttachSec)
	{
		EC_H(lpAttachSec->IsAttachmentBlocked(pwszFileName,&bBlocked));
	}
	if (lpAttachSec) lpAttachSec->Release();

	*pfBlocked = bBlocked;
	return hRes;
}// IsAttachmentBlocked

BOOL IsDuplicateProp(LPSPropTagArray lpArray, ULONG ulPropTag)
{
	ULONG i = 0;

	if (!lpArray) return FALSE;

	for (i = 0; i < lpArray->cValues; i++)
	{
		//They're dupes if the IDs are the same
		if (RegKeys[regkeyALLOW_DUPE_COLUMNS].ulCurDWORD)
		{
			if (lpArray->aulPropTag[i] == ulPropTag)
				return TRUE;
		}
		else
		{
			if (PROP_ID(lpArray->aulPropTag[i]) == PROP_ID(ulPropTag))
				return TRUE;
		}
	}

	return FALSE;
}//IsDuplicateProp

//returns pointer to a string
//delete with delete[]
void MyHexFromBin(LPBYTE lpb, size_t cb, LPTSTR* lpsz)
{
	ULONG i = 0;
	ULONG iBinPos = 0;
	if (!lpsz)
	{
		DebugPrint(DBGGeneric, _T("MyHexFromBin called with null lpsz\n"));
		return;
	}
	*lpsz = NULL;
	if (!lpb || !cb)
	{
		DebugPrint(DBGGeneric, _T("MyHexFromBin called with null lpb or null cb\n"));
		return;
	}
	*lpsz = new TCHAR[1+2*cb];
	if (*lpsz)
	{
		memset(*lpsz, 0, 1+2*cb);
		for (i = 0; i < cb; i++)
		{
			BYTE bLow;
			BYTE bHigh;
			TCHAR szLow;
			TCHAR szHigh;

			bLow = (BYTE) ((lpb[i]) & 0xf);
			bHigh = (BYTE) ((lpb[i] >> 4) & 0xf);
			szLow = (TCHAR) ((bLow <= 0x9) ? _T('0') + bLow : _T('A') + bLow - 0xa);
			szHigh = (TCHAR) ((bHigh <= 0x9) ? _T('0') + bHigh : _T('A') + bHigh - 0xa);

			(*lpsz)[iBinPos] = szHigh;
			(*lpsz)[iBinPos+1] = szLow;

			iBinPos += 2;
		}
		(*lpsz)[iBinPos] = _T('\0');
	}
}

//must allocate first
//Note that cb should be the number of bytes allocated for the lpb
void MyBinFromHex(LPCTSTR lpsz, LPBYTE lpb, size_t cb)
{
	HRESULT hRes = S_OK;
	if (!lpb || !cb)
	{
		DebugPrint(DBGGeneric, _T("MyBinFromHex called with null lpb\n"));
		return;
	}
	size_t cchStrLen = NULL;
	EC_H(StringCchLength(lpsz,STRSAFE_MAX_CCH,&cchStrLen));
	if (!lpsz || !cchStrLen)
	{
		DebugPrint(DBGGeneric, _T("MyBinFromHex called with null lpsz\n"));
		return;
	}
	ULONG iBinPos = 0;
	ULONG i = 0;
	TCHAR szTmp[3] = {0};
	szTmp[2] = 0;
	//In case the string starts with 'x' or '0x'
	if (lpsz[0] == _T('x') || lpsz[0] == _T('X')) i = 1;
	if (lpsz[1] == _T('x') || lpsz[1] == _T('X')) i = 2;
	//convert two characters at a time
	for (; i < cchStrLen && cb > 0; i+=2)
	{
		szTmp[0] = lpsz[i];
		szTmp[1] = lpsz[i+1];

		lpb[iBinPos] = (BYTE) _tcstol(szTmp,NULL,16);

		iBinPos += 1;
		cb--;//so we can't run off the end of the lpb
	}
}

ULONG aulOneOffIDs[] = {dispidFormStorage,
dispidPageDirStream,
dispidFormPropStream,
dispidScriptStream,
dispidPropDefStream, // dispidPropDefStream must remain next to last in list
dispidCustomFlag}; // dispidCustomFlag must remain last in list

#define ulNumOneOffIDs (sizeof(aulOneOffIDs)/sizeof(aulOneOffIDs[0]))

HRESULT RemoveOneOff(LPMESSAGE lpMessage, BOOL bRemovePropDef)
{
	if (!lpMessage) return MAPI_E_INVALID_PARAMETER;
	DebugPrint(DBGNamedProp,_T("RemoveOneOff - removing one off named properties.\n"));

	HRESULT hRes = S_OK;
	MAPINAMEID  rgnmid[ulNumOneOffIDs];
	LPMAPINAMEID rgpnmid[ulNumOneOffIDs];
	LPSPropTagArray lpTags = NULL;

	ULONG i = 0;
	for (i = 0 ; i < ulNumOneOffIDs ; i++)
	{
		rgnmid[i].lpguid = (LPGUID)&PSETID_Common;
		rgnmid[i].ulKind = MNID_ID;
		rgnmid[i].Kind.lID = aulOneOffIDs[i];
		rgpnmid[i] = &rgnmid[i];
	}

	EC_H(lpMessage->GetIDsFromNames(
		ulNumOneOffIDs,
		rgpnmid,
		0,
		&lpTags));
	if (lpTags)
	{
		LPSPropProblemArray lpProbArray = NULL;

		DebugPrint(DBGNamedProp,_T("RemoveOneOff - identified the following properties.\n"));
		DebugPrintPropTagArray(DBGNamedProp,lpTags);

		// The last prop is the flag value we'll be updating, don't count it
		lpTags->cValues = ulNumOneOffIDs-1;

		// If we're not removing the prop def stream, then don't count it
		if (!bRemovePropDef)
		{
			lpTags->cValues = lpTags->cValues-1;
		}

		EC_H(lpMessage->DeleteProps(
			lpTags,
			&lpProbArray));
		if (SUCCEEDED(hRes))
		{
			if (lpProbArray)
			{
				DebugPrint(DBGNamedProp,_T("RemoveOneOff - DeleteProps problem array:\n%s\n"),ProblemArrayToString(lpProbArray));
			}

			SPropTagArray	pTag = {0};
			ULONG			cProp = 0;
			LPSPropValue	lpCustomFlag = NULL;

			// Grab dispidCustomFlag, the last tag in the array
			pTag.cValues = 1;
			pTag.aulPropTag[0] = CHANGE_PROP_TYPE(lpTags->aulPropTag[ulNumOneOffIDs-1],PT_LONG);

			WC_H(lpMessage->GetProps(
				&pTag,
				fMapiUnicode,
				&cProp,
				&lpCustomFlag));
			if (SUCCEEDED(hRes) && 1 == cProp && lpCustomFlag && PT_LONG == PROP_TYPE(lpCustomFlag->ulPropTag))
			{
				LPSPropProblemArray lpProbArray2 = NULL;
				// Clear the INSP_ONEOFFFLAGS bits so OL doesn't look for the props we deleted
				lpCustomFlag->Value.l = lpCustomFlag->Value.l & ~(INSP_ONEOFFFLAGS);
				if (bRemovePropDef)
				{
					lpCustomFlag->Value.l = lpCustomFlag->Value.l & ~(INSP_PROPDEFINITION);
				}
				EC_H(lpMessage->SetProps(
					1,
					lpCustomFlag,
					&lpProbArray2));
				if (S_OK == hRes && lpProbArray2)
				{
					DebugPrint(DBGNamedProp,_T("RemoveOneOff - SetProps problem array:\n%s\n"),ProblemArrayToString(lpProbArray2));
				}
				MAPIFreeBuffer(lpProbArray2);
			}
			hRes = S_OK;

			EC_H(lpMessage->SaveChanges(KEEP_OPEN_READWRITE));

			if (SUCCEEDED(hRes))
			{
				DebugPrint(DBGNamedProp,_T("RemoveOneOff - One-off properties removed.\n"));
			}
		}
		MAPIFreeBuffer(lpProbArray);
	}
	MAPIFreeBuffer(lpTags);

	return hRes;
}

HRESULT ResendMessages(LPMAPIFOLDER lpFolder, HWND hWnd)
{
	HRESULT		hRes = S_OK;
	LPMAPITABLE	lpContentsTable = NULL;
	LPSRowSet	pRows = NULL;
	ULONG		i;

	//You define a SPropTagArray array here using the SizedSPropTagArray Macro
	//This enum will allows you to access portions of the array by a name instead of a number.
	//If more tags are added to the array, appropriate constants need to be added to the enum.
	enum {
			ePR_ENTRYID,
			NUM_COLS};
	//These tags represent the message information we would like to pick up
	static SizedSPropTagArray(NUM_COLS,sptCols) = { NUM_COLS,
		PR_ENTRYID
	};

	if (!lpFolder) return MAPI_E_INVALID_PARAMETER;

	EC_H(lpFolder->GetContentsTable(0,&lpContentsTable));

	if (lpContentsTable)
	{
		EC_H(HrQueryAllRows(
			lpContentsTable,
			(LPSPropTagArray) &sptCols,
			NULL,//restriction...we're not using this parameter
			NULL,//sort order...we're not using this parameter
			0,
			&pRows));

		if (pRows)
		{
			if (!FAILED(hRes)) for (i = 0; i < pRows -> cRows; i++)
			{
				LPMESSAGE lpMessage = NULL;

				hRes = S_OK;
				WC_H(CallOpenEntry(
					NULL,
					NULL,
					lpFolder,
					NULL,
					pRows->aRow[i].lpProps[ePR_ENTRYID].Value.bin.cb,
					(LPENTRYID) pRows->aRow[i].lpProps[ePR_ENTRYID].Value.bin.lpb,
					NULL,
					MAPI_BEST_ACCESS,
					NULL,
					(LPUNKNOWN*)&lpMessage));
				if (lpMessage)
				{
					EC_H(ResendSingleMessage(lpFolder,lpMessage,hWnd));
					lpMessage->Release();
				}
			}
		}
	}

	if (pRows) FreeProws(pRows);
	if (lpContentsTable) lpContentsTable->Release();
	return hRes;
}//ResendMessages

HRESULT ResendSingleMessage(
							LPMAPIFOLDER lpFolder,
							LPSBinary MessageEID,
							HWND hWnd)
{
	HRESULT hRes = S_OK;
	LPMESSAGE lpMessage = NULL;

	if (!lpFolder || ! MessageEID) return MAPI_E_INVALID_PARAMETER;

	EC_H(CallOpenEntry(
		NULL,
		NULL,
		lpFolder,
		NULL,
		MessageEID->cb,
		(LPENTRYID)MessageEID->lpb,
		NULL,
		MAPI_BEST_ACCESS,
		NULL,
		(LPUNKNOWN*)&lpMessage));
	if (lpMessage)
	{
		EC_H(ResendSingleMessage(
			lpFolder,
			lpMessage,
			hWnd));
	}

	if (lpMessage) lpMessage->Release();
	return hRes;
}

HRESULT ResendSingleMessage(
							LPMAPIFOLDER lpFolder,
							LPMESSAGE lpMessage,
							HWND hWnd)
{
	HRESULT			hRes = S_OK;
	LPATTACH		lpAttach = NULL;
	LPMESSAGE		lpAttachMsg = NULL;
	LPMAPITABLE		lpAttachTable = NULL;
	LPSRowSet		pRows = NULL;
	LPMESSAGE		lpNewMessage = NULL;
	LPSPropTagArray lpsMessageTags = NULL;
	LPSPropProblemArray lpsPropProbs = NULL;
	ULONG			ulProp;
	SPropValue		sProp;

	enum {atPR_ATTACH_METHOD,
		atPR_ATTACH_NUM,
		atPR_DISPLAY_NAME,
		atNUM_COLS};

	static SizedSPropTagArray(atNUM_COLS,atCols) = {atNUM_COLS,
		PR_ATTACH_METHOD,
		PR_ATTACH_NUM,
		PR_DISPLAY_NAME
	};

	static SizedSPropTagArray(2,atObjs) = {2,
		PR_MESSAGE_RECIPIENTS,
		PR_MESSAGE_ATTACHMENTS
	};

	if (!lpMessage || !lpFolder) return MAPI_E_INVALID_PARAMETER;

	DebugPrint(DBGGeneric,_T("ResendSingleMessage: Checking message for embedded messages\n"));

	EC_H(lpMessage->GetAttachmentTable(
		NULL,
		&lpAttachTable));

	if (lpAttachTable)
	{
		EC_H(lpAttachTable->SetColumns((LPSPropTagArray)&atCols, TBL_BATCH));

		//Now we iterate through each of the attachments
		if (!FAILED(hRes)) for (;;)
		{
			hRes = S_OK;
			if (pRows) FreeProws(pRows);
			pRows = NULL;
			EC_H(lpAttachTable->QueryRows(
				1,
				NULL,
				&pRows));
			if (FAILED(hRes)) break;
			if (!pRows || (pRows && !pRows->cRows)) break;

			if (ATTACH_EMBEDDED_MSG == pRows->aRow->lpProps[atPR_ATTACH_METHOD].Value.l)
			{
				DebugPrint(DBGGeneric,_T("Found an embedded message to resend.\n"));

				if (lpAttach) lpAttach->Release();
				lpAttach = NULL;
				EC_H(lpMessage->OpenAttach(
					pRows->aRow->lpProps[atPR_ATTACH_NUM].Value.l,
					NULL,
					MAPI_BEST_ACCESS,
					(LPATTACH*)&lpAttach));
				if (!lpAttach) continue;

				if (lpAttachMsg) lpAttachMsg->Release();
				lpAttachMsg = NULL;
				EC_H(lpAttach->OpenProperty(
					PR_ATTACH_DATA_OBJ,
					(LPIID)&IID_IMessage,
					0,
					MAPI_MODIFY,
					(LPUNKNOWN *)&lpAttachMsg));
				if (MAPI_E_INTERFACE_NOT_SUPPORTED == hRes)
				{
					CHECKHRESMSG(hRes,IDS_ATTNOTEMBEDDEDMSG);
					continue;
				}
				else if (FAILED(hRes)) continue;

				DebugPrint(DBGGeneric,_T("Message opened.\n"));

				if (CheckStringProp(&pRows->aRow->lpProps[atPR_DISPLAY_NAME],PT_TSTRING))
				{
					DebugPrint(DBGGeneric,_T("Resending \"%s\"\n"),pRows->aRow->lpProps[atPR_DISPLAY_NAME].Value.LPSZ);
				}

				DebugPrint(DBGGeneric,_T("Creating new message.\n"));
				if (lpNewMessage) lpNewMessage->Release();
				lpNewMessage= NULL;
				EC_H(lpFolder->CreateMessage(NULL, 0, &lpNewMessage));
				if (!lpNewMessage) continue;

				EC_H(lpNewMessage->SaveChanges(KEEP_OPEN_READWRITE));

				// Copy all the transmission properties
				DebugPrint(DBGGeneric,_T("Getting list of properties.\n"));
				MAPIFreeBuffer(lpsMessageTags);
				lpsMessageTags = NULL;
				EC_H(lpAttachMsg->GetPropList(0, &lpsMessageTags));
				if (lpsMessageTags) continue;

				DebugPrint(DBGGeneric,_T("Copying properties to new message.\n"));
				if (!FAILED(hRes)) for(ulProp = 0; ulProp < lpsMessageTags->cValues; ulProp++)
				{
					hRes = S_OK;
					//it would probably be quicker to use this loop to construct an array of properties
					//we desire to copy, and then pass that array to GetProps and then SetProps
					if (FIsTransmittable(lpsMessageTags->aulPropTag[ulProp]))
					{
						LPSPropValue lpProp = NULL;
						DebugPrint(DBGGeneric,_T("Copying 0x%08X\n"),lpsMessageTags->aulPropTag[ulProp]);
						WC_H(HrGetOneProp(lpAttachMsg,lpsMessageTags->aulPropTag[ulProp],&lpProp));

						WC_H(HrSetOneProp(lpNewMessage,lpProp));

						MAPIFreeBuffer(lpProp);
					}
				}

				EC_H(lpNewMessage->SaveChanges(KEEP_OPEN_READWRITE));

				DebugPrint(DBGGeneric,_T("Copying recipients and attachments to new message.\n"));

				LPMAPIPROGRESS lpProgress = GetMAPIProgress(_T("IMAPIProp::CopyProps"), hWnd);// STRING_OK

				EC_H(lpAttachMsg->CopyProps(
					(LPSPropTagArray)&atObjs,
					lpProgress ? (ULONG_PTR)hWnd : NULL,
					lpProgress,
					&IID_IMessage,
					lpNewMessage,
					lpProgress ? MAPI_DIALOG : 0,
					&lpsPropProbs));
				if (lpsPropProbs)
				{
					EC_PROBLEMARRAY(lpsPropProbs);
					MAPIFreeBuffer(lpsPropProbs);
					lpsPropProbs = NULL;
					continue;
				}

				if(lpProgress)
					lpProgress->Release();

				lpProgress = NULL;

				sProp.dwAlignPad = 0;
				sProp.ulPropTag = PR_DELETE_AFTER_SUBMIT;
				sProp.Value.b = TRUE;

				DebugPrint(DBGGeneric,_T("Setting PR_DELETE_AFTER_SUBMIT to TRUE.\n"));
				EC_H(HrSetOneProp(lpNewMessage,&sProp));

				SPropTagArray sPropTagArray;

				sPropTagArray.cValues = 1;
				sPropTagArray.aulPropTag[0] = PR_SENTMAIL_ENTRYID;

				DebugPrint(DBGGeneric,_T("Deleting PR_SENTMAIL_ENTRYID\n"));
				EC_H(lpNewMessage->DeleteProps(&sPropTagArray,NULL));

				EC_H(lpNewMessage->SaveChanges(KEEP_OPEN_READWRITE));

				DebugPrint(DBGGeneric,_T("Submitting new message.\n"));
				EC_H(lpNewMessage->SubmitMessage(0));

				DebugPrint(DBGGeneric,_T("Message resent successfully.\n"));
			}
			else
			{
				DebugPrint(DBGGeneric,_T("Attachment is not an embedded message.\n"));
			}
		}
	}

	MAPIFreeBuffer(lpsMessageTags);
	if (lpNewMessage) lpNewMessage->Release();
	if (lpAttachMsg) lpAttachMsg->Release();
	if (lpAttach) lpAttach->Release();
	if (pRows) FreeProws(pRows);
	if (lpAttachTable) lpAttachTable->Release();
	return hRes;
}//ResendSingleMessage

HRESULT ResetPermissionsOnItems(LPMDB lpMDB, LPMAPIFOLDER lpMAPIFolder)
{
	LPSRowSet		pRows = NULL;
	HRESULT			hRes = S_OK;
	HRESULT			hResOverall = S_OK;
	ULONG			iItemCount = 0;
	ULONG			iCurPropRow = 0;
	ULONG			ulFlags = NULL;
	LPMAPITABLE		lpContentsTable = NULL;
	LPMESSAGE		lpMessage = NULL;
	CWaitCursor		Wait;//Change the mouse to an hourglass while we work.
	int				i = 0;

	enum {eidPR_ENTRYID,
		eidNUM_COLS};

	static SizedSPropTagArray(eidNUM_COLS,eidCols) = {eidNUM_COLS,
		PR_ENTRYID,
	};

	if (!lpMDB || !lpMAPIFolder) return MAPI_E_INVALID_PARAMETER;

	//We pass through this code twice, once for regular contents, once for associated contents
	if (!FAILED(hRes)) for (i = 0;i<=1;i++)
	{
		hRes = S_OK;
		ulFlags = (1 == i?MAPI_ASSOCIATED:NULL) |
			fMapiUnicode;

		if (lpContentsTable) lpContentsTable->Release();
		lpContentsTable = NULL;
		//Get the table of contents of the folder
		WC_H(lpMAPIFolder->GetContentsTable(
			ulFlags,
			&lpContentsTable));
		if (hRes == MAPI_E_NO_ACCESS)
		{
			CHECKHRESMSG(hRes,IDS_GETCONTENTSNOACCESS);
			lpContentsTable = NULL;
		}
		else if (hRes == MAPI_E_CALL_FAILED)
		{
			CHECKHRESMSG(hRes,IDS_GETCONTENTSFAILED);
			lpContentsTable = NULL;
		}
		else if (hRes == MAPI_E_NO_SUPPORT)
		{
			CHECKHRESMSG(hRes,IDS_GETCONTENTSNOTSUPPORTED);
			lpContentsTable = NULL;
		}
		else if (hRes == MAPI_E_UNKNOWN_FLAGS)
		{
			CHECKHRESMSG(hRes,IDS_GETCONTENTSBADFLAG);
			lpContentsTable = NULL;
		}
		else CHECKHRES(hRes);

		EC_H(lpContentsTable->SetColumns(
			(LPSPropTagArray) &eidCols,
			TBL_BATCH));

		//go to the first row
		EC_H(lpContentsTable->SeekRow(
			BOOKMARK_BEGINNING,
			0,
			NULL));
		hRes = S_OK;//don't let failure here fail the whole op

		//get rows and delete PR_NT_SECURITY_DESCRIPTOR
		if (!FAILED(hRes)) for (;;)
		{
			hRes = S_OK;
			if (pRows) FreeProws(pRows);
			pRows = NULL;
			//Pull back a sizable block of rows to modify
			EC_H(lpContentsTable->QueryRows(
				200,
				NULL,
				&pRows));
			if (!FAILED(hRes) || !pRows || !pRows->cRows) break;

			for(iCurPropRow = 0;iCurPropRow<pRows->cRows;iCurPropRow++)
			{
				hRes = S_OK;
				if (lpMessage) lpMessage->Release();
				lpMessage = NULL;

				WC_H(CallOpenEntry(
					lpMDB,
					NULL,
					NULL,
					NULL,
					pRows->aRow[iCurPropRow].lpProps[eidPR_ENTRYID].Value.bin.cb,
					(LPENTRYID)pRows->aRow[iCurPropRow].lpProps[eidPR_ENTRYID].Value.bin.lpb,
					NULL,
					MAPI_BEST_ACCESS,
					NULL,
					(LPUNKNOWN*)&lpMessage));
				if (FAILED(hRes))
				{
					hResOverall = hRes;
					continue;
				}

				WC_H(DeleteProperty(lpMessage,PR_NT_SECURITY_DESCRIPTOR));
				if (FAILED(hRes))
				{
					hResOverall = hRes;
					continue;
				}

				iItemCount++;
			}
		}
		DebugPrint(DBGGeneric,_T("ResetPermissionsOnItems reset permissions on %d items\n"),iItemCount);
	}

	if (pRows) FreeProws(pRows);
	if (lpMessage) lpMessage->Release();
	if (lpContentsTable) lpContentsTable->Release();
	if (S_OK != hResOverall) return hResOverall;
	return hRes;
}//ResetPermissionsOnItems

//This function creates a new message based in m_lpContainer
//Then sends the message
HRESULT SendTestMessage(
							  LPMAPISESSION lpMAPISession,
							  LPMAPIFOLDER lpFolder,
							  LPCTSTR szRecipient,
							  LPCTSTR szBody,
							  LPCTSTR szSubject)
{
	HRESULT			hRes = S_OK;
	LPMESSAGE		lpNewMessage = NULL;

	if (!lpMAPISession || !lpFolder) return MAPI_E_INVALID_PARAMETER;

	EC_H(lpFolder->CreateMessage(
		NULL,//default interface
		0,//flags
		&lpNewMessage));

	if (lpNewMessage)
	{
		SPropValue sProp;

		sProp.dwAlignPad = 0;
		sProp.ulPropTag = PR_DELETE_AFTER_SUBMIT;
		sProp.Value.b = TRUE;

		DebugPrint(DBGGeneric,_T("Setting PR_DELETE_AFTER_SUBMIT to TRUE.\n"));
		EC_H(HrSetOneProp(lpNewMessage,&sProp));

		sProp.dwAlignPad = 0;
		sProp.ulPropTag = PR_BODY;
		sProp.Value.LPSZ = (LPTSTR) szBody;

		DebugPrint(DBGGeneric,_T("Setting PR_BODY to %s.\n"),szBody);
		EC_H(HrSetOneProp(lpNewMessage,&sProp));

		sProp.dwAlignPad = 0;
		sProp.ulPropTag = PR_SUBJECT;
		sProp.Value.LPSZ = (LPTSTR) szSubject;

		DebugPrint(DBGGeneric,_T("Setting PR_SUBJECT to %s.\n"),szSubject);
		EC_H(HrSetOneProp(lpNewMessage,&sProp));

		SPropTagArray sPropTagArray;

		sPropTagArray.cValues = 1;
		sPropTagArray.aulPropTag[0] = PR_SENTMAIL_ENTRYID;

		DebugPrint(DBGGeneric,_T("Deleting PR_SENTMAIL_ENTRYID\n"));
		EC_H(lpNewMessage->DeleteProps(&sPropTagArray,NULL));

		DebugPrint(DBGGeneric,_T("Adding recipient: %s.\n"),szRecipient);
		EC_H(AddRecipient(
			lpMAPISession,
			lpNewMessage,
			szRecipient,
			MAPI_TO));

		DebugPrint(DBGGeneric,_T("Submitting message\n"));
		EC_H(lpNewMessage->SubmitMessage(NULL));
	}

	if (lpNewMessage) lpNewMessage->Release();
	return hRes;
}//SendTestMessage

HRESULT WrapStreamForRTF(
				 LPSTREAM lpCompressedRTFStream,
				 BOOL bUseWrapEx,
				 ULONG ulFlags,
				 ULONG ulInCodePage,
				 ULONG ulOutCodePage,
				 LPSTREAM FAR * lpUncompressedRTFStream,
				 ULONG FAR * pulStreamFlags)
{
	if (!lpCompressedRTFStream || !lpUncompressedRTFStream) return MAPI_E_INVALID_PARAMETER;
	HRESULT hRes = S_OK;

	if (!bUseWrapEx)
	{
		WC_H(WrapCompressedRTFStream(
			lpCompressedRTFStream,
			ulFlags,
			lpUncompressedRTFStream));
	}
	else
	{
		if (pfnWrapEx)
		{
			RTF_WCSINFO wcsinfo = {0};
			RTF_WCSRETINFO retinfo = {0};

			retinfo.size = sizeof(RTF_WCSRETINFO);

			wcsinfo.size = sizeof (RTF_WCSINFO);
			wcsinfo.ulFlags = ulFlags;
			wcsinfo.ulInCodePage = ulInCodePage;			//Get ulCodePage from PR_INTERNET_CPID on the IMessage
			wcsinfo.ulOutCodePage = ulOutCodePage;			//Desired code page for return

			WC_H(pfnWrapEx(
				lpCompressedRTFStream,
				&wcsinfo,
				lpUncompressedRTFStream,
				&retinfo));
			*pulStreamFlags = retinfo.ulStreamFlags;
		}
		else
		{
			ErrDialog(__FILE__,__LINE__,IDS_EDWRAPEXNOTFOUND);
		}
	}

	return hRes;
}

HRESULT CopyNamedProps(LPMAPIPROP lpSource, LPGUID lpPropSetGUID, BOOL bDoMove, BOOL bDoNoReplace, LPMAPIPROP lpTarget, HWND hWnd)
{
	if((!lpSource) || (!lpTarget)) return MAPI_E_INVALID_PARAMETER;

	HRESULT			hRes = S_OK;
	LPSPropTagArray	lpPropTags = NULL;

	EC_H(GetNamedPropsByGUID(lpSource, lpPropSetGUID, &lpPropTags));

	if (!FAILED(hRes) && lpPropTags)
	{
		LPSPropProblemArray	lpProblems = NULL;
		ULONG				ulFlags = NULL;
		if (bDoMove)		ulFlags |= MAPI_MOVE;
		if (bDoNoReplace)	ulFlags |= MAPI_NOREPLACE;

		LPMAPIPROGRESS lpProgress = GetMAPIProgress(_T("IMAPIProp::CopyProps"), hWnd);// STRING_OK

		if(lpProgress)
			ulFlags |= MAPI_DIALOG;

		EC_H(lpSource->CopyProps(lpPropTags,
								 lpProgress ? (ULONG_PTR)hWnd : NULL,
								 lpProgress,
								 &IID_IMAPIProp,
								 lpTarget,
								 ulFlags,
								 &lpProblems));

		if(lpProgress)
			lpProgress->Release();

		lpProgress = NULL;

		EC_PROBLEMARRAY(lpProblems);
		MAPIFreeBuffer(lpProblems);
	}

	MAPIFreeBuffer(lpPropTags);

	return hRes;
}

HRESULT GetNamedPropsByGUID(LPMAPIPROP lpSource, LPGUID lpPropSetGUID, LPSPropTagArray* lpOutArray)
{
	if(!lpSource || !lpPropSetGUID || *lpOutArray)
		return MAPI_E_INVALID_PARAMETER;

	HRESULT hRes = S_OK;
	LPSPropTagArray lpAllProps = NULL;

	*lpOutArray = NULL;

	WC_H(lpSource->GetPropList(0, &lpAllProps));

	if(S_OK == hRes && lpAllProps)
	{
		ULONG			cProps = 0;
		LPMAPINAMEID*	lppNameIDs = NULL;

		WC_H(lpSource->GetNamesFromIDs(&lpAllProps,
									   NULL,
									   0,
									   &cProps,
									   &lppNameIDs));

		if(S_OK == hRes && lppNameIDs)
		{
			ULONG i = 0;
			ULONG ulNumProps = 0;//count of props that match our guid
			for (i = 0; i < cProps; i++)
			{
				if (PROP_ID(lpAllProps->aulPropTag[i]) > 0x7FFF
					&& lppNameIDs[i]
				    && !memcmp(lppNameIDs[i]->lpguid, lpPropSetGUID, sizeof(GUID)))
				{
					ulNumProps++;
				}
			}

			LPSPropTagArray lpFilteredProps = NULL;

			WC_H(MAPIAllocateBuffer(CbNewSPropTagArray(ulNumProps),
									(LPVOID*)&lpFilteredProps));

			if (S_OK == hRes && lpFilteredProps)
			{
				lpFilteredProps->cValues = 0;

				for (i = 0; i < cProps; i++)
				{
					if (PROP_ID(lpAllProps->aulPropTag[i]) > 0x7FFF
						&& lppNameIDs[i]
						&& !memcmp(lppNameIDs[i]->lpguid, lpPropSetGUID, sizeof(GUID)))
					{
						lpFilteredProps->aulPropTag[lpFilteredProps->cValues] = lpAllProps->aulPropTag[i];
						lpFilteredProps->cValues++;
					}
				}
				*lpOutArray = lpFilteredProps;
			}
		}
		MAPIFreeBuffer(lppNameIDs);
	}
	MAPIFreeBuffer(lpAllProps);
	return hRes;
}

// Delete with delete[]
HRESULT AnsiToUnicode(LPCSTR pszA, LPWSTR* ppszW)
{
	HRESULT hRes = S_OK;
	if (!ppszW) return MAPI_E_INVALID_PARAMETER;
	*ppszW = NULL;
	if (NULL == pszA) return S_OK;

	// Get our buffer size
	int iRet = 0;
	EC_D(iRet, MultiByteToWideChar(
		CP_ACP,
		0,
		pszA,
		(int) -1,
		NULL,
		NULL));

	if (SUCCEEDED(hRes) && 0 != iRet)
	{
		// MultiByteToWideChar returns num of chars
		LPWSTR pszW = new WCHAR[iRet];

		EC_D(iRet, MultiByteToWideChar(
			CP_ACP,
			0,
			pszA,
			(int) -1,
			pszW,
			iRet));
		if (SUCCEEDED(hRes))
		{
			*ppszW = pszW;
		}
	}
	return hRes;
}

// if cbszW == -1, let WideCharToMultiByte compute the length
// Delete with delete[]
HRESULT UnicodeToAnsi(LPCWSTR pszW, LPSTR* ppszA, size_t cchszW)
{
	HRESULT hRes = S_OK;
	if (!ppszA) return MAPI_E_INVALID_PARAMETER;
	*ppszA = NULL;
	if (NULL == pszW) return S_OK;

	// Get our buffer size
	int iRet = 0;
	EC_D(iRet, WideCharToMultiByte(
		CP_ACP,
		0,
		pszW,
		(int) cchszW,
		NULL,
		NULL,
		NULL,
		NULL));

	if (SUCCEEDED(hRes) && 0 != iRet)
	{
		// WideCharToMultiByte returns num of bytes
		LPSTR pszA = (LPSTR) new BYTE[iRet];

		EC_D(iRet, WideCharToMultiByte(
			CP_ACP,
			0,
			pszW,
			(int) cchszW,
			pszA,
			iRet,
			NULL,
			NULL));
		if (SUCCEEDED(hRes))
		{
			*ppszA = pszA;
		}
	}
	return hRes;
}

BOOL CheckStringProp(LPSPropValue lpProp, ULONG ulPropType)
{
	if (PT_STRING8 != ulPropType && PT_UNICODE != ulPropType)
	{
		DebugPrint(DBGGeneric,_T("CheckStringProp: Called with invalid ulPropType of 0x%X\n"),ulPropType);
		return false;
	}
	if (!lpProp)
	{
		DebugPrint(DBGGeneric,_T("CheckStringProp: lpProp is NULL\n"));
		return false;
	}

	if (PT_ERROR == PROP_TYPE(lpProp->ulPropTag))
	{
		DebugPrint(DBGGeneric,_T("CheckStringProp: lpProp->ulPropTag is of type PT_ERROR\n"));
		return false;
	}

	if (ulPropType != PROP_TYPE(lpProp->ulPropTag))
	{
		DebugPrint(DBGGeneric,_T("CheckStringProp: lpProp->ulPropTag is not of type 0x%X\n"),ulPropType);
		return false;
	}

	if (NULL == lpProp->Value.LPSZ)
	{
		DebugPrint(DBGGeneric,_T("CheckStringProp: lpProp->Value.LPSZ is NULL\n"));
		return false;
	}

	if (PT_STRING8 == ulPropType && NULL == lpProp->Value.lpszA[0])
	{
		DebugPrint(DBGGeneric,_T("CheckStringProp: lpProp->Value.lpszA[0] is NULL\n"));
		return false;
	}

	if (PT_UNICODE == ulPropType && NULL == lpProp->Value.lpszW[0])
	{
		DebugPrint(DBGGeneric,_T("CheckStringProp: lpProp->Value.lpszW[0] is NULL\n"));
		return false;
	}

	return true;
}

DWORD ComputeStoreHash(ULONG cbStoreEID, LPENTRYID pbStoreEID, LPCWSTR pwzFileName)
{
	DWORD  dwHash = 0;
	ULONG  cdw    = 0;
	DWORD* pdw    = NULL;
	ULONG  cb     = 0;
	BYTE*  pb     = NULL;
	ULONG  i      = 0;

	if (!cbStoreEID || !pbStoreEID) return dwHash;

	// Get the Store Entry ID
	// pbStoreEID is a pointer to the Entry ID
	// cbStoreEID is the size in bytes of the Entry ID
	pdw = (DWORD*)pbStoreEID;
	cdw = cbStoreEID / sizeof(DWORD);

	for (i = 0; i < cdw; i++)
	{
		dwHash = (dwHash << 5) + dwHash + *pdw++;
	}

	pb = (BYTE *)pdw;
	cb = cbStoreEID % sizeof(DWORD);

	for (i = 0; i < cb; i++)
	{
		dwHash = (dwHash << 5) + dwHash + *pb++;
	}

	// You may want to also include the store file name in the hash calculation
	// Get store FileName
	// pwzFileName is a NULL terminated string with the path and filename of the store
	if (pwzFileName)
	{
		while (*pwzFileName)
		{
			dwHash = (dwHash << 5) + dwHash + *pwzFileName++;
		}
	}

	// dwHash now contains the hash to be used. It should be written in hex when building the URL.
	return dwHash;
}// ComputeStoreHash

const WORD kwBaseOffset = 0xAC00;  // Hangul char range (AC00-D7AF)
// Allocates with new, free with delete[]
LPWSTR EncodeID(ULONG cbEID, LPENTRYID rgbID)
{
	ULONG   i = 0;
	LPWSTR  pwzDst = NULL;
	LPBYTE  pbSrc = NULL;
	LPWSTR  pwzIDEncoded = NULL;

	// rgbID is the item Entry ID or the attachment ID
	// cbID is the size in bytes of rgbID

	// Allocate memory for pwzIDEncoded
	pwzIDEncoded = new WCHAR[cbEID+1];
	if (!pwzIDEncoded) return NULL;

	for (i = 0, pbSrc = (LPBYTE)rgbID, pwzDst = pwzIDEncoded;
		i < cbEID;
		i++, pbSrc++, pwzDst++)
	{
		*pwzDst = (WCHAR) (*pbSrc + kwBaseOffset);
	}

	// Ensure NULL terminated
	*pwzDst = L'\0';

	// pwzIDEncoded now contains the entry ID encoded.
	return pwzIDEncoded;
}// EncodeID