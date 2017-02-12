// ViceMon.cpp : implementation file
//

#include "stdafx.h"
#include <stdlib.h>
#include "Step6502.h"
#include "ViceMon.h"


// CViceMon

IMPLEMENT_DYNAMIC(CViceMon, CRichEditCtrl)

CViceMon::CViceMon()
{
	m_enabled = false;
	m_prompt = -1;

}

CViceMon::~CViceMon()
{
}


BEGIN_MESSAGE_MAP(CViceMon, CRichEditCtrl)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()



// CViceMon message handlers


// ViceMon.cpp : implementation file
//

// CViceMonPane

IMPLEMENT_DYNAMIC(CViceMonPane, CDockablePane)

CViceMonPane::CViceMonPane()
{

}

CViceMonPane::~CViceMonPane()
{
}


BEGIN_MESSAGE_MAP(CViceMonPane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_INPUT()
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()



// CViceMonPane message handlers




int CViceMonPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct)==-1)
		return -1;

	// TODO:  Add your specialized creation code here
	CRect rect;
	GetClientRect(&rect);

	if (!m_edit.Create( WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL |
					    ES_AUTOHSCROLL | ES_AUTOHSCROLL | ES_MULTILINE,
				   		rect, this, ID_VICE_EDIT)) {
		TRACE0("Failed to create Vice monitor edit control\n");
		return -1;
	}

	m_edit.SetOptions(ECOOP_OR, ECO_READONLY);

	return 0;
}


void CViceMonPane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	CRect rect;
	GetClientRect(&rect);

	m_edit.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
}

#define CHARS_PER_PRINT 256
void CViceMonPane::AddText(const char *buf, int len)
{
	wchar_t wide_buf[CHARS_PER_PRINT];
	int rd = 0;

	while (rd<len) {
		m_edit.SetSel(-1, -1);

		int idx = 0, idx_max = (CHARS_PER_PRINT-1);

		while (rd<len && idx<idx_max) {
			mbtowc(wide_buf+(idx++), buf+(rd++), 1);
		}
		wide_buf[idx] = 0;

		m_edit.ReplaceSel(wide_buf);
	}

	CHARRANGE range;
	m_edit.GetSel(range);
	m_edit.m_prompt = range.cpMax;
}

void CViceMonPane::EnableInput(bool enable)
{
	m_edit.SetOptions(enable ? ECOOP_AND : ECOOP_OR, enable ? (~ECO_READONLY) : ECO_READONLY );
	m_edit.m_enabled = enable;
}

void ViceSend(const char *string, int length);
void CViceMon::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	if (m_enabled && nChar==VK_RETURN) {
		SetSel(-1, -1);
		ReplaceSel( L"\n" );
		SetOptions(ECOOP_OR, ECO_READONLY );
		m_enabled = false;
		SetSel(m_prompt, -1);
		CString text = GetSelText();
		SetSel(-1, -1);
		char buf[512];
		size_t len = 0;
		wcstombs_s(&len, buf, text, sizeof(buf)-1);
		ViceSend(buf, (int)len);
	}

	CRichEditCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CViceMonPane::OnRawInput(UINT nInputcode, HRAWINPUT hRawInput)
{
	// This feature requires Windows XP or greater.
	// The symbol _WIN32_WINNT must be >= 0x0501.
	// TODO: Add your message handler code here and/or call default

	CDockablePane::OnRawInput(nInputcode, hRawInput);
}


void CViceMonPane::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	CDockablePane::OnKeyDown(nChar, nRepCnt, nFlags);
}
