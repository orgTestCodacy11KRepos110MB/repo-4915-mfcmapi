#pragma once

namespace cache
{
	class CMapiObjects;
}

#include <UI/Dialogs/HierarchyTable/HierarchyTableDlg.h>

namespace dialog
{
	class CMsgStoreDlg : public CHierarchyTableDlg
	{
	public:
		CMsgStoreDlg(
			_In_ std::shared_ptr<cache::CMapiObjects> lpMapiObjects,
			_In_opt_ LPMAPIPROP lpMDB,
			_In_opt_ LPMAPIPROP lpRootFolder,
			tableDisplayFlags displayFlags);
		~CMsgStoreDlg();

	private:
		// Overrides from base class
		void OnDeleteSelectedItem() override;
		void HandleAddInMenuSingle(
			_In_ LPADDINMENUPARAMS lpParams,
			_In_ LPMAPIPROP lpMAPIProp,
			_In_ LPMAPICONTAINER lpContainer) override;
		void HandleCopy() override;
		_Check_return_ bool HandlePaste() override;
		void OnInitMenu(_In_ CMenu* pMenu) override;
		LPMAPIFOLDER GetSelectedFolder(modifyType bModify) const;

		// Menu items
		void OnCreateSubFolder();
		void OnDisplayACLTable();
		void OnDisplayAssociatedContents();
		void OnDisplayCalendarFolder();
		void OnDisplayContactsFolder();
		void OnDisplayDeletedContents();
		void OnDisplayDeletedSubFolders();
		void OnDisplayInbox();
		void OnDisplayMailboxTable();
		void OnDisplayOutgoingQueueTable();
		void OnDisplayReceiveFolderTable();
		void OnDisplayRulesTable();
		void OnDisplaySpecialFolder(ULONG ulFolder);
		void OnDisplayTasksFolder();
		void OnEmptyFolder();
		void OnOpenFormContainer();
		void OnSaveFolderContentsAsMSG();
		void OnSaveFolderContentsAsTextFiles();
		void OnResolveMessageClass();
		void OnSelectForm();
		void OnSetReceiveFolder();
		void OnResendAllMessages();
		void OnResetPermissionsOnItems();
		void OnRestoreDeletedFolder();
		void OnValidateIPMSubtree();
		void OnPasteFolder();
		void OnPasteFolderContents();
		void OnPasteRules();
		void OnPasteMessages();
		void OnExportMessages();

		LPMDB m_lpMDB;
		DECLARE_MESSAGE_MAP()
	};
} // namespace dialog