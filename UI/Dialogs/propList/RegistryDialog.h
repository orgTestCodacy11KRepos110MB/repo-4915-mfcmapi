#pragma once
#include <UI/Dialogs/BaseDialog.h>
#include <UI/Controls/StyleTree/StyleTreeCtrl.h>
#include <core/sortlistdata/sortListData.h>

namespace dialog
{
	class RegistryDialog : public CBaseDialog
	{
	public:
		RegistryDialog(_In_ std::shared_ptr<cache::CMapiObjects> lpMapiObjects);
		~RegistryDialog();

	protected:
		// Overrides from base class
		BOOL OnInitDialog() override;
		void OnInitMenu(_In_ CMenu* pMenu) override;
		void OnRefreshView() override;

	private:
		controls::StyleTreeCtrl m_lpRegKeyTree{};

		void AddProfileRoot(const std::wstring& szName, const std::wstring& szRoot);
		void AddChildren(const HTREEITEM hParent, const HKEY hRootKey);
		void EnumRegistry();
		DECLARE_MESSAGE_MAP()
	};
} // namespace dialog