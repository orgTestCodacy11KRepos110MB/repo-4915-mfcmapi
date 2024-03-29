#pragma once
#include <UI/Dialogs/ContentsTable/ContentsTableDlg.h>

namespace
{
	class CMapiObjects;
}

namespace dialog
{
	class CProfileListDlg : public CContentsTableDlg
	{
	public:
		CProfileListDlg(_In_ std::shared_ptr<cache::CMapiObjects> lpMapiObjects, _In_ LPMAPITABLE lpMAPITable);
		~CProfileListDlg();

	private:
		// Overrides from base class
		void OnDeleteSelectedItem() override;
		void OnDisplayItem() override;
		void OnRefreshView() override;
		void HandleCopy() override;
		_Check_return_ bool HandlePaste() override;

		// Menu items
		void OnAddExchangeToProfile();
		void OnAddPSTToProfile();
		void OnAddUnicodePSTToProfile();
		void OnAddServicesToMAPISVC();
		void OnAddServiceToProfile();
		void OnCreateProfile();
		void OnGetMAPISVC();
		void OnGetProfileServiceVersion();
		void OnInitMenu(_In_ CMenu* pMenu) override;
		void OnLaunchProfileWizard();
		void OnRemoveServicesFromMAPISVC();
		void OnSetDefaultProfile();
		void OnOpenProfileByName();
		void OnExportProfile();

		void AddPSTToProfile(bool bUnicodePST);

		DECLARE_MESSAGE_MAP()
	};
} // namespace dialog