#pragma once
// MailboxTableDlg.h : header file
//

//forward definitions
class CContentsTableListCtrl;
class CSingleMAPIPropListCtrl;
class CParentWnd;
class CMapiObjects;

#include "ContentsTableDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CMailboxTableDlg dialog

class CMailboxTableDlg : public CContentsTableDlg
{
	// Construction
public:
	CMailboxTableDlg(
		CParentWnd* pParentWnd,
		CMapiObjects *lpMapiObjects,
		LPCTSTR lpszServerName,
		LPMAPITABLE	lpMAPITable);
	virtual ~CMailboxTableDlg();

	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMailboxTableDlg)
	afx_msg void OnInitMenu(CMenu* pMenu);
	afx_msg void OnDisplayItem();
	afx_msg void OnOpenWithFlags();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	BOOL CreateDialogAndMenu(UINT nIDMenuResource);
	virtual void	OnCreatePropertyStringRestriction();
	void DisplayItem(ULONG ulFlags);

	LPTSTR m_lpszServerName;
};
