#pragma once
#include <UI/ViewPane/ViewPane.h>
// List splitter control which hosts two controls either horizontally or vertically

namespace dialog
{
	class CBaseDialog;
}

namespace controls
{
	enum class splitType
	{
		vertical = 0,
		horizontal = 1
	};

	// Implementation of a lite Splitter class.
	// Liberal code sharing from the CSplitterWnd class
	class CFakeSplitter : public CWnd
	{
	public:
		CFakeSplitter() = default;
		~CFakeSplitter();

		void Init(HWND hWnd);
		void SetPaneOne(HWND paneOne) noexcept;
		void SetPaneTwo(HWND paneTwo) noexcept;
		void SetPaneOne(std::shared_ptr<viewpane::ViewPane> paneOne) noexcept
		{
			m_ViewPaneOne = paneOne;
			m_ViewPaneOne->SetTop();
			if (m_ViewPaneOne)
			{
				m_iSplitWidth = 7;
			}
			else
			{
				m_iSplitWidth = 0;
			}
		}

		void SetPaneTwo(std::shared_ptr<viewpane::ViewPane> paneTwo) noexcept
		{
			m_ViewPaneTwo = paneTwo;
			m_ViewPaneTwo->SetTop();
		}

		void SetPercent(FLOAT iNewPercent);
		void SetSplitType(splitType stSplitType) noexcept;
		void OnSize(UINT nType, int cx, int cy);
		HDWP DeferWindowPos(_In_ HDWP hWinPosInfo, _In_ int x, _In_ int y, _In_ int width, _In_ int height);
		int GetSplitWidth() const noexcept { return m_iSplitWidth; }

		// Callbacks
		std::function<int()> PaneOneMinSpanCallback = nullptr;
		std::function<int()> PaneTwoMinSpanCallback = nullptr;

	private:
		void OnPaint();
		void OnMouseMove(UINT nFlags, CPoint point);
		LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
		_Check_return_ int HitTest(LONG x, LONG y) const noexcept;

		// starting and stopping tracking
		void StartTracking(int ht);
		void StopTracking();
		void CalcSplitPos();

		bool m_bTracking{};
		FLOAT m_flSplitPercent{0.5};
		HWND m_PaneOne{};
		HWND m_PaneTwo{};
		HWND m_hwndParent{};
		std::shared_ptr<viewpane::ViewPane> m_ViewPaneOne{};
		std::shared_ptr<viewpane::ViewPane> m_ViewPaneTwo{};
		int m_iSplitWidth{};
		int m_iSplitPos{1};
		splitType m_SplitType{splitType::horizontal};
		HCURSOR m_hSplitCursorV{};
		HCURSOR m_hSplitCursorH{};

		DECLARE_MESSAGE_MAP()
	};
} // namespace controls