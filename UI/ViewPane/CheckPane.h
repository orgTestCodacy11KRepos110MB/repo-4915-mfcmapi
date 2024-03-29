#pragma once
#include <UI/ViewPane/ViewPane.h>

namespace viewpane
{
	class CheckPane : public ViewPane
	{
	public:
		static std::shared_ptr<CheckPane> Create(int paneID, UINT uidLabel, bool bVal, bool bReadOnly);
		bool GetCheck() const;
		static void Draw(_In_ HWND hWnd, _In_ HDC hDC, _In_ const RECT& rc, UINT itemState);
		bool containsWindow(HWND hWnd) const noexcept override;
		RECT GetWindowRect() const noexcept override
		{
			auto rcCheck = RECT{};
			::GetWindowRect(m_Check.GetSafeHwnd(), &rcCheck);
			const auto rcHeader = ViewPane::GetWindowRect();
			auto rcPane = RECT{};
			::UnionRect(&rcPane, &rcCheck, &rcHeader);
			return rcPane;
		}
		void EnableTopMargin() { m_bTopMargin = true; }

	private:
		void Initialize(_In_ CWnd* pParent, _In_ HDC hdc) override;
		HDWP DeferWindowPos(_In_ HDWP hWinPosInfo, _In_ int x, _In_ int y, _In_ int width, _In_ int height) override;
		void CommitUIValues() override;
		int GetMinWidth() override;
		int GetFixedHeight() override;

		CButton m_Check;
		std::wstring m_szLabel; // Check label
		int m_iLabelWidth{}; // The width of the label
		bool m_bCheckValue{false};
		bool m_bCommitted{false};
		bool m_bTopMargin{false};
	};
} // namespace viewpane