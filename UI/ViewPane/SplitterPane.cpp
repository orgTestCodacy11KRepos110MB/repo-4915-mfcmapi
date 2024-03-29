#include <StdAfx.h>
#include <UI/ViewPane/SplitterPane.h>
#include <core/utility/output.h>
#include <UI/UIFunctions.h>

namespace viewpane
{
	std::shared_ptr<SplitterPane> SplitterPane::CreateVerticalPane(const int paneID, const UINT uidLabel)
	{
		const auto pane = CreateHorizontalPane(paneID, uidLabel);
		pane->m_bVertical = true;

		return pane;
	}

	std::shared_ptr<SplitterPane> SplitterPane::CreateHorizontalPane(const int paneID, const UINT uidLabel)
	{
		const auto pane = std::make_shared<SplitterPane>();
		if (pane)
		{
			pane->SetLabel(uidLabel);
			if (uidLabel)
			{
				pane->makeCollapsible();
			}

			pane->m_paneID = paneID;
		}

		return pane;
	}

	int SplitterPane::GetMinWidth()
	{
		if (m_bVertical)
		{
			return max(m_PaneOne->GetMinWidth(), m_PaneTwo->GetMinWidth());
		}
		else
		{
			return m_PaneOne->GetMinWidth() + m_PaneTwo->GetMinWidth() +
				   (m_lpSplitter ? m_lpSplitter->GetSplitWidth() : 0);
		}
	}

	int SplitterPane::GetLines()
	{
		if (!collapsed())
		{
			if (m_bVertical)
			{
				return m_PaneOne->GetLines() + m_PaneTwo->GetLines();
			}
			else
			{
				return max(m_PaneOne->GetLines(), m_PaneTwo->GetLines());
			}
		}

		return 0;
	}

	ULONG SplitterPane::HandleChange(const UINT nID)
	{
		// See if the panes can handle the change first
		if (m_PaneOne)
		{
			auto paneID = m_PaneOne->HandleChange(nID);
			if (paneID != static_cast<ULONG>(-1)) return paneID;
		}

		if (m_PaneTwo)
		{
			auto paneID = m_PaneTwo->HandleChange(nID);
			if (paneID != static_cast<ULONG>(-1)) return paneID;
		}

		return ViewPane::HandleChange(nID);
	}

	void SplitterPane::SetMargins(
		const int iMargin,
		const int iSideMargin,
		const int iLabelHeight, // Height of the label
		const int iSmallHeightMargin,
		const int iLargeHeightMargin,
		const int iButtonHeight, // Height of buttons below the control
		const int iEditHeight) // height of an edit control
	{
		ViewPane::SetMargins(
			iMargin, iSideMargin, iLabelHeight, iSmallHeightMargin, iLargeHeightMargin, iButtonHeight, iEditHeight);

		if (m_PaneOne)
		{
			m_PaneOne->SetMargins(
				iMargin, iSideMargin, iLabelHeight, iSmallHeightMargin, iLargeHeightMargin, iButtonHeight, iEditHeight);
		}

		if (m_PaneTwo)
		{
			m_PaneTwo->SetMargins(
				iMargin, iSideMargin, iLabelHeight, iSmallHeightMargin, iLargeHeightMargin, iButtonHeight, iEditHeight);
		}
	}

	void SplitterPane::Initialize(_In_ CWnd* pParent, _In_ HDC hdc)
	{
		ViewPane::Initialize(pParent, hdc);

		m_lpSplitter = std::make_shared<controls::CFakeSplitter>();

		if (m_lpSplitter)
		{
			m_lpSplitter->Init(pParent->GetSafeHwnd());
			m_lpSplitter->SetSplitType(m_bVertical ? controls::splitType::vertical : controls::splitType::horizontal);
			m_PaneOne->Initialize(m_lpSplitter.get(), hdc);
			m_PaneOne->SetTop();
			m_PaneTwo->Initialize(m_lpSplitter.get(), hdc);
			m_PaneTwo->SetTop();
			m_lpSplitter->SetPaneOne(m_PaneOne);
			m_lpSplitter->SetPaneTwo(m_PaneTwo);
			if (m_bVertical)
			{
				m_lpSplitter->PaneOneMinSpanCallback = [&] { return m_PaneOne ? m_PaneOne->GetFixedHeight() : 0; };
				m_lpSplitter->PaneTwoMinSpanCallback = [&] { return m_PaneTwo ? m_PaneTwo->GetFixedHeight() : 0; };
			}
			else
			{
				m_lpSplitter->PaneOneMinSpanCallback = [&] { return m_PaneOne ? m_PaneOne->GetMinWidth() : 0; };
				m_lpSplitter->PaneTwoMinSpanCallback = [&] { return m_PaneTwo ? m_PaneTwo->GetMinWidth() : 0; };
			}
		}

		m_bInitialized = true;
	}

	// SplitterPaneLayout:
	// Header: GetHeaderHeight
	// Collapsible:
	//    margin: m_iSmallHeightMargin
	//    variable to fit panes
	int SplitterPane::GetFixedHeight()
	{
		auto iHeight = GetHeaderHeight();

		if (!collapsed())
		{
			if (m_bVertical)
			{
				iHeight += m_PaneOne->GetFixedHeight() + m_PaneTwo->GetFixedHeight() +
						   (m_lpSplitter ? m_lpSplitter->GetSplitWidth() : 0);
			}
			else
			{
				iHeight += max(m_PaneOne->GetFixedHeight(), m_PaneTwo->GetFixedHeight());
			}
		}

		return iHeight;
	}

	HDWP SplitterPane::DeferWindowPos(
		_In_ HDWP hWinPosInfo,
		_In_ const int x,
		_In_ const int y,
		_In_ const int width,
		_In_ const int height)
	{
		output::DebugPrint(
			output::dbgLevel::Draw,
			L"SplitterPane::DeferWindowPos x:%d y:%d width:%d height: %d\n",
			x,
			y,
			width,
			height);

		auto curY = y;

		// Layout our label
		hWinPosInfo = EC_D(HDWP, ViewPane::DeferWindowPos(hWinPosInfo, x, curY, width, height));
		curY += GetHeaderHeight();

		if (collapsed())
		{
			WC_B_S(m_lpSplitter->ShowWindow(SW_HIDE));
		}
		else
		{
			WC_B_S(m_lpSplitter->ShowWindow(SW_SHOW));
			hWinPosInfo = ui::DeferWindowPos(
				hWinPosInfo,
				m_lpSplitter->GetSafeHwnd(),
				x,
				curY,
				width,
				height - (curY - y),
				L"SplitterPane::DeferWindowPos::splitter");
			m_lpSplitter->OnSize(NULL, width, height - (curY - y));
		}

		return hWinPosInfo;
	}

	bool SplitterPane::containsWindow(HWND hWnd) const noexcept
	{
		if (m_lpSplitter && m_lpSplitter->GetSafeHwnd()) return true;
		if (m_PaneOne && m_PaneOne->containsWindow(hWnd)) return true;
		if (m_PaneTwo && m_PaneTwo->containsWindow(hWnd)) return true;
		return m_Header.containsWindow(hWnd);
	}
} // namespace viewpane