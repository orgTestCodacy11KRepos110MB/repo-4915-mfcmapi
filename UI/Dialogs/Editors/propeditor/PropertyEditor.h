#pragma once
#include <UI/Dialogs/Editors/propeditor/ipropeditor.h>
#include <core/mapi/cache/mapiObjects.h>

namespace dialog::editor
{
	class CPropertyEditor : public IPropEditor
	{
	public:
		CPropertyEditor(
			_In_ CWnd* pParentWnd,
			_In_ std::shared_ptr<cache::CMapiObjects> lpMapiObjects,
			UINT uidTitle,
			const std::wstring& name,
			bool bIsAB,
			bool bMVRow,
			_In_opt_ LPMAPIPROP lpMAPIProp,
			ULONG ulPropTag,
			_In_opt_ const _SPropValue* lpsPropValue);
		~CPropertyEditor();

		// Get values after we've done the DisplayDialog
		_Check_return_ LPSPropValue getValue() noexcept;

	private:
		BOOL OnInitDialog() override;
		void InitPropertyControls();
		void WriteStringsToSPropValue();
		_Check_return_ ULONG HandleChange(UINT nID) override;
		void OnOK() override;
		void OpenEntry(_In_ const SBinary& bin);

		// source variables
		LPMAPIPROP m_lpMAPIProp{}; // Used only for parsing
		ULONG m_ulPropTag{};
		bool m_bIsAB{}; // whether the tag is from the AB or not
		const _SPropValue* m_lpsInputValue{};
		bool m_bDirty{};
		bool m_bMVRow{}; // whether this row came from a multivalued property. Used for smart view parsing.
		const std::wstring m_name;

		SPropValue m_sOutputValue{};
		std::vector<BYTE> m_bin; // Temp storage for m_sOutputValue
		GUID m_guid{}; // Temp storage for m_sOutputValue
		std::shared_ptr<cache::CMapiObjects> m_lpMapiObjects{};
	};
} // namespace dialog::editor