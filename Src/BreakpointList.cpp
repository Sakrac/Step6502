// BreakpointList.cpp : implementation file
//

#include "stdafx.h"
#include "Step6502.h"
#include "BreakpointList.h"
#include "machine.h"
#include "Expressions.h"

// CBreakpointEdit

IMPLEMENT_DYNAMIC(CBreakpointEdit, CEdit)

CBreakpointEdit::CBreakpointEdit()
{
	m_breakpoints = nullptr;
}

CBreakpointEdit::~CBreakpointEdit()
{
}


BEGIN_MESSAGE_MAP(CBreakpointEdit, CEdit)
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



// CBreakpointEdit message handlers

void CBreakpointEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	ShowWindow(SW_HIDE);
	if (m_breakpoints)
		m_breakpoints->OnClearEditField();
}


void CBreakpointEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_RETURN && m_breakpoints) {
		wchar_t wide_instruction[512];
		GetWindowText(wide_instruction, sizeof(wide_instruction)/sizeof(wide_instruction[0]));
		if (m_breakpoints)
			m_breakpoints->OnEditField(wide_instruction);
		ShowWindow(SW_HIDE);
		return;
	} else if (nChar==VK_ESCAPE) {
		ShowWindow(SW_HIDE);
	}
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


UINT CBreakpointEdit::OnGetDlgCode()
{
	// TODO: Add your message handler code here and/or call default

	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTTAB;
}


// CBPListCtr

IMPLEMENT_DYNAMIC(CBPListCtr, CListCtrl)

CBPListCtr::CBPListCtr()
{

}

CBPListCtr::~CBPListCtr()
{
}


BEGIN_MESSAGE_MAP(CBPListCtr, CListCtrl)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CBPListCtr::OnLvnItemchanged)
END_MESSAGE_MAP()



// CBPListCtr message handlers

void CBPListCtr::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!IsCPURunning()) {
		int x0 = GetColumnWidth(0);
		int x1 = GetColumnWidth(1);

		int c = GetItemCount();
		for (int n = 0; n<c; n++) {
			CRect rect;
			if (GetItemRect(n, &rect, LVIR_BOUNDS)) {
				if (point.y > rect.top && point.y < rect.bottom && point.x > 32) {
					if (point.x < x0) {
/*						rect.left = 32;
						rect.right = x0;
						m_breakpoints->OnSelectField(0, n, rect);
						return;
*/					} else {
						rect.left = x0;
						rect.right = x1;
						m_breakpoints->OnSelectField(1, n, rect);
						return;
					}
				}
			}
		}
	}
	CListCtrl::OnLButtonDown(nFlags, point);
}

// CBreakpointList

IMPLEMENT_DYNAMIC(CBreakpointList, CDockablePane)

CBreakpointList::CBreakpointList()
{
	m_field_col = m_field_row = -1;
}

CBreakpointList::~CBreakpointList()
{
}

BEGIN_MESSAGE_MAP(CBreakpointList, CDockablePane)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
END_MESSAGE_MAP()



// CBreakpointList message handlers




void CBreakpointList::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(&rect);

	m_bp_listctrl.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);

	LVCOLUMN column = { 0 };
	column.mask = LVCF_WIDTH;
	column.cx = 96;
	m_bp_listctrl.SetColumn(0, &column);// L"Column 1");
	column.mask = LVCF_WIDTH;
	column.cx = rect.Width() - 96;

	m_bp_listctrl.SetColumn(1, &column);// L"Column 1");
	Invalidate();
}


BOOL CBreakpointList::OnEraseBkgnd(CDC* pDC)
{
	CRect rt;
	GetClientRect(&rt);

	CBrush br;
	br.CreateStockObject(WHITE_BRUSH);
	pDC->SelectObject(&br);

	pDC->Rectangle(&rt); // I just fill all the BG with white brush  

	return CDockablePane::OnEraseBkgnd(pDC);
}

void CBreakpointList::OnSelectField(int column, int row, CRect &rect)
{
	m_field_col = column;
	m_field_row = row;
	m_edit_field.MoveWindow(&rect);
	if (CWnd *editItem = GetDlgItem(ID_BREAKPOINT_EDIT))
		editItem->SetWindowText(m_bp_listctrl.GetItemText(row, column));
	m_edit_field.ShowWindow(SW_SHOW);
	m_edit_field.SetFocus();
	m_edit_field.Invalidate();
}

void CBreakpointList::OnEditField(wchar_t *text)
{
	if (m_field_col>=0 && m_field_row>=0) {
		uint8_t ops[64];
		m_bp_listctrl.SetItemText(m_field_row, m_field_col, text);
		uint32_t len = BuildExpression(text, ops, sizeof(ops));
		SetBPCondition(m_BP_ID[m_field_row].id, ops, len);
		m_BP_Expressions[m_BP_ID[m_field_row].id] = text;
	}
	m_field_col = m_field_row = -1;
}

void CBreakpointList::OnClearEditField()
{
	m_field_col = m_field_row = -1;
}

struct sBPSort {
	uint16_t addr;
	bool enabled;
	uint32_t id;
};

static int _sortBP(const void *A, const void *B)
{
	uint32_t a = ((const struct sBPSort*)A)->id;
	uint32_t b = ((const struct sBPSort*)B)->id;

	return a<b ? -1 : (a==b ? 0 : 1);
}

void CBreakpointList::RemoveBPByIndex(int idx)
{
	if (idx>=0 && idx<(int)m_BP_ID.size()) {
		RemoveBreakpointByID(m_BP_ID[idx].id);
	}
}

void CBreakpointList::GoToBPByIndex(int idx)
{
	if (idx>=0 && idx<(int)m_BP_ID.size()) {
		uint16_t addr;
		if (GetBreakpointAddrByID(m_BP_ID[idx].id, addr)) {
			theApp.GetMainFrame()->FocusAddr(addr);
		}
	}
}

void CBreakpointList::CheckboxState(int idx, bool set) {
	if (idx>=0 && idx<(int)m_BP_ID.size()) {
		if (set != m_BP_ID[idx].enabled) {
			m_BP_ID[idx].enabled = set;
			EnableBPByID(m_BP_ID[idx].id, set);
		}
	}
}


void CBreakpointList::Rebuild(uint32_t id_remove)
{
	m_bp_listctrl.DeleteAllItems();
	m_BP_ID.clear();

	// remove condition string if removed
	std::map<uint32_t, CString>::iterator f = m_BP_Expressions.find(id_remove);
	if (f != m_BP_Expressions.end())
		m_BP_Expressions.erase(f);

	uint16_t *pAddrs;
	uint32_t *pIDs;
	uint16_t nBP_DS;
	if (uint16_t nBP = GetPCBreakpointsID(&pAddrs, &pIDs, nBP_DS)) {
		uint16_t nBP_T = nBP + nBP_DS;
		std::vector<struct sBPSort> bp_sorted;
		bp_sorted.reserve(nBP_T);
		for (int b = 0; b<nBP_T; b++) {
			struct sBPSort bp;
			bp.addr = pAddrs[b];
			bp.id = pIDs[b];
			bp.enabled = b<nBP;
			bp_sorted.push_back(bp);
		}
		qsort(&bp_sorted[0], nBP_T, sizeof(struct sBPSort), _sortBP);
		m_BP_ID.reserve(nBP_T);
		for (int b = 0; b<nBP_T; b++) {
			wchar_t addrStr[32];
			wsprintf(addrStr, L"%04x", bp_sorted[b].addr);
			m_bp_listctrl.InsertItem(b, addrStr, 0);
			f = m_BP_Expressions.find(bp_sorted[b].id);
			if (f != m_BP_Expressions.end())
				m_bp_listctrl.SetItemText(b, 1, f->second);
			m_BP_ID.push_back(struct sIDInfo(bp_sorted[b].id, bp_sorted[b].enabled));
			ListView_SetCheckState(m_bp_listctrl.GetSafeHwnd(), b, bp_sorted[b].enabled);
		}
	}
	Invalidate();
}

int CBreakpointList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	GetClientRect(&rect);

	if (!m_edit_field.Create(ES_LEFT | WS_CHILD | WS_BORDER | ES_WANTRETURN, rect, this, ID_BREAKPOINT_EDIT)) {
		TRACE0("Failed to create assembler input for code view\n");
		return FALSE; // failed to create
	}
	m_edit_field.SetBreakpoints(this);


	if (!m_bp_listctrl.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL
						   | LVS_REPORT, rect, this, ID_BREAKPOINT_LIST)) {
		TRACE0("Failed to create Nodes List\n");
		return -1;
	}
	m_bp_listctrl.SetBreakpoints(this);
	m_bp_listctrl.SetExtendedStyle(m_bp_listctrl.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	m_icons.Create(12, 12, ILC_COLOR, 1, 1);

	if (!m_breakpoint_image.LoadBitmap(IDB_BREAKPOINT)) {
		TRACE0("Failed to create breakpoint bitmap\n");
	}

	m_icons.Add(&m_breakpoint_image, RGB(0,0,0));
	m_bp_listctrl.SetImageList(&m_icons, LVSIL_SMALL);

	LVCOLUMN column = { 0 };

	column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_ORDER | LVCF_TEXT;
	column.fmt = LVCFMT_FILL | LVCF_TEXT | LVCFMT_FIXED_WIDTH;
	column.cx = 96;
	column.iOrder = 0;
	column.pszText = L"Condition";

	m_bp_listctrl.InsertColumn(0, &column);

	column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_IMAGE | LVCF_ORDER | LVCF_TEXT;
	column.fmt = LVCFMT_IMAGE | LVCFMT_FIXED_WIDTH;
	column.cx = 96;
	column.iImage = 0;
	column.iOrder = 0;
	column.pszText = L"Address";

	m_bp_listctrl.InsertColumn(0, &column);// L"Column 1");

	Rebuild();
	return 0;
}


void CBPListCtr::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	if (nChar == VK_DELETE && m_breakpoints) {
		POSITION p = GetFirstSelectedItemPosition();
		while (p) {
			int n = GetNextSelectedItem(p);
			m_breakpoints->RemoveBPByIndex(n);
		}
		m_breakpoints->Rebuild();
		theApp.GetMainFrame()->InvalidateAll();
	}

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CBPListCtr::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	UINT f;
	int item = HitTest(point, &f);
	if (f & LVHT_ONITEM)
		m_breakpoints->GoToBPByIndex(item);
}


void CBPListCtr::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	if (m_breakpoints && pNMHDR) {
		bool checked = !!ListView_GetCheckState(GetSafeHwnd(), pNMLV->iItem);
		m_breakpoints->CheckboxState(pNMLV->iItem, checked);
	}

}
