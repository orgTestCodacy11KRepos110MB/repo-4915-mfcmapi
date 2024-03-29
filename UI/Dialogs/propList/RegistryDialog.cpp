#include <StdAfx.h>
#include <UI/Dialogs/propList/RegistryDialog.h>
#include <UI/Controls/SortList/SingleMAPIPropListCtrl.h>
#include <core/mapi/mapiFunctions.h>
#include <core/utility/output.h>
#include <core/propertyBag/registryPropertyBag.h>
#include <UI/UIFunctions.h>

namespace dialog
{
	static std::wstring CLASS = L"RegistryDialog";

	RegistryDialog::RegistryDialog(_In_ std::shared_ptr<cache::CMapiObjects> lpMapiObjects)
		: CBaseDialog(lpMapiObjects, NULL)
	{
		TRACE_CONSTRUCTOR(CLASS);
		m_szTitle = strings::loadstring(IDS_REGISTRY_DIALOG);

		CBaseDialog::CreateDialogAndMenu(NULL, NULL, NULL);
	}

	RegistryDialog::~RegistryDialog() { TRACE_DESTRUCTOR(CLASS); }

	BOOL RegistryDialog::OnInitDialog()
	{
		const auto bRet = CBaseDialog::OnInitDialog();

		UpdateTitleBarText();

		if (m_lpFakeSplitter)
		{
			m_lpRegKeyTree.Create(&m_lpFakeSplitter, true);
			m_lpFakeSplitter.SetPaneOne(m_lpRegKeyTree.GetSafeHwnd());
			m_lpRegKeyTree.FreeNodeDataCallback = [&](auto hKey) {
				if (hKey) WC_W32_S(RegCloseKey(reinterpret_cast<HKEY>(hKey)));
			};
			m_lpRegKeyTree.ItemSelectedCallback = [&](auto hItem) {
				auto hKey = reinterpret_cast<HKEY>(m_lpRegKeyTree.GetItemData(hItem));
				m_lpPropDisplay->SetDataSource(std::make_shared<propertybag::registryPropertyBag>(hKey));
			};

			m_lpRegKeyTree.KeyDownCallback = [&](auto _1, auto _2, auto _3, auto _4) {
				return HandleKeyDown(_1, _2, _3, _4);
			};

			EnumRegistry();

			m_lpFakeSplitter.SetPercent(0.25);
		}

		return bRet;
	}

	void RegistryDialog::OnInitMenu(_In_ CMenu* pMenu)
	{
		CBaseDialog::OnInitMenu(pMenu);

		if (pMenu)
		{
			ui::DeleteMenu(pMenu->m_hMenu, ID_DISPLAYPROPERTYASSECURITYDESCRIPTORPROPSHEET);
			ui::DeleteMenu(pMenu->m_hMenu, ID_ATTACHMENTPROPERTIES);
			ui::DeleteMenu(pMenu->m_hMenu, ID_RECIPIENTPROPERTIES);
			ui::DeleteMenu(pMenu->m_hMenu, ID_RTFSYNC);
			ui::DeleteMenu(pMenu->m_hMenu, ID_TESTEDITBODY);
			ui::DeleteMenu(pMenu->m_hMenu, ID_TESTEDITHTML);
			ui::DeleteMenu(pMenu->m_hMenu, ID_TESTEDITRTF);

			// Locate and delete "Edit as stream" menu by searching for an item on it
			WC_B_S(ui::DeleteSubmenu(pMenu->m_hMenu, ID_EDITPROPERTYASASCIISTREAM));
		}
	}

	void RegistryDialog::OnRefreshView()
	{
		m_lpPropDisplay->SetDataSource({});
		m_lpRegKeyTree.DeleteAllItems();
		EnumRegistry();
	}

	static const wchar_t* profileWMS =
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles";
	static const wchar_t* profile15 = L"SOFTWARE\\Microsoft\\Office\\15.0\\Outlook\\Profiles";
	static const wchar_t* profile16 = L"SOFTWARE\\Microsoft\\Office\\16.0\\Outlook\\Profiles";
	void RegistryDialog::EnumRegistry()
	{
		AddProfileRoot(L"WMS", profileWMS);
		AddProfileRoot(L"15", profile15);
		AddProfileRoot(L"16", profile16);
	}

	void RegistryDialog::AddProfileRoot(const std::wstring& szName, const std::wstring& szRoot)
	{
		HKEY hRootKey = nullptr;
		auto hRes = WC_W32(RegOpenKeyExW(HKEY_CURRENT_USER, szRoot.c_str(), NULL, KEY_ALL_ACCESS, &hRootKey));

		if (SUCCEEDED(hRes))
		{
			const auto rootNode = m_lpRegKeyTree.AddChildNode(szName.c_str(), TVI_ROOT, hRootKey, nullptr);
			AddChildren(rootNode, hRootKey);
		}
	}

	void RegistryDialog::AddChildren(const HTREEITEM hParent, const HKEY hRootKey)
	{
		auto cchMaxSubKeyLen = DWORD{}; // Param in RegQueryInfoKeyW is misnamed
		auto cSubKeys = DWORD{};
		auto hRes = WC_W32(RegQueryInfoKeyW(
			hRootKey,
			nullptr, // lpClass
			nullptr, // lpcchClass
			nullptr, // lpReserved
			&cSubKeys, // lpcSubKeys
			&cchMaxSubKeyLen, // lpcbMaxSubKeyLen
			nullptr, // lpcbMaxClassLen
			nullptr, // lpcValues
			nullptr, // lpcbMaxValueNameLen
			nullptr, // lpcbMaxValueLen
			nullptr, // lpcbSecurityDescriptor
			nullptr)); // lpftLastWriteTime

		if (cSubKeys && cchMaxSubKeyLen)
		{
			cchMaxSubKeyLen++; // For null terminator
			auto szBuf = std::wstring(cchMaxSubKeyLen, '\0');
			for (DWORD dwIndex = 0; dwIndex < cSubKeys; dwIndex++)
			{
				szBuf.clear();
				hRes = WC_W32(RegEnumKeyW(hRootKey, dwIndex, const_cast<wchar_t*>(szBuf.c_str()), cchMaxSubKeyLen));
				if (hRes == S_OK)
				{
					HKEY hSubKey = nullptr;
					hRes = WC_W32(RegOpenKeyExW(hRootKey, szBuf.c_str(), NULL, KEY_ALL_ACCESS, &hSubKey));
					const auto node = m_lpRegKeyTree.AddChildNode(szBuf.c_str(), hParent, hSubKey, nullptr);
					AddChildren(node, hSubKey);
				}
			}
		}
	}

	BEGIN_MESSAGE_MAP(RegistryDialog, CBaseDialog)
	END_MESSAGE_MAP()
} // namespace dialog
