#include <StdAfx.h>
#include <UI/Dialogs/Editors/restriction/ResSizeEditor.h>
#include <core/mapi/extraPropTags.h>
#include <core/utility/output.h>
#include <core/interpret/flags.h>
#include <core/interpret/proptags.h>
#include <core/addin/mfcmapi.h>

namespace dialog::editor
{
	ResSizeEditor::ResSizeEditor(_In_ CWnd* pParentWnd, ULONG relop, ULONG ulPropTag, ULONG cb)
		: CEditor(pParentWnd, IDS_RESED, IDS_RESEDSIZEPROMPT, CEDITOR_BUTTON_OK | CEDITOR_BUTTON_CANCEL)
	{
		TRACE_CONSTRUCTOR(SIZECLASS);

		SetPromptPostFix(flags::AllFlagsToString(flagRelop, false));
		AddPane(viewpane::TextPane::CreateSingleLinePane(0, IDS_RELOP, false));
		SetHex(0, relop);
		const auto szFlags = flags::InterpretFlags(flagRelop, relop);
		AddPane(viewpane::TextPane::CreateSingleLinePane(1, IDS_RELOP, szFlags, true));

		AddPane(viewpane::TextPane::CreateSingleLinePane(2, IDS_ULPROPTAG, false));
		SetHex(2, ulPropTag);
		AddPane(viewpane::TextPane::CreateSingleLinePane(
			3, IDS_ULPROPTAG, proptags::TagToString(ulPropTag, nullptr, false, true), true));

		AddPane(viewpane::TextPane::CreateSingleLinePane(4, IDS_CB, false));
		SetHex(4, cb);
	}

	_Check_return_ ULONG ResSizeEditor::HandleChange(UINT nID)
	{
		const auto paneID = CEditor::HandleChange(nID);

		if (paneID == 0)
		{
			SetStringW(1, flags::InterpretFlags(flagRelop, GetHex(0)));
		}
		else if (paneID == 2)
		{
			SetStringW(3, proptags::TagToString(GetPropTag(2), nullptr, false, true));
		}

		return paneID;
	}
} // namespace dialog::editor