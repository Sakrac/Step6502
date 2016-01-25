// WatchList.cpp : implementation file
//

#include "stdafx.h"
#include "Step6502.h"
#include "WatchView.h"
#include "machine.h"
#include "Expressions.h"

// CWatchEdit

IMPLEMENT_DYNAMIC(CWatchEdit, CEdit)

CWatchEdit::CWatchEdit()
{
	m_Watchs = nullptr;
}

CWatchEdit::~CWatchEdit()
{
}


BEGIN_MESSAGE_MAP(CWatchEdit, CEdit)
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



// CWatchEdit message handlers

void CWatchEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	ShowWindow(SW_HIDE);
	if (m_Watchs)
		m_Watchs->OnClearEditField();
}


void CWatchEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_RETURN && m_Watchs) {
		wchar_t wide_instruction[512];
		GetWindowText(wide_instruction, sizeof(wide_instruction)/sizeof(wide_instruction[0]));
		if (m_Watchs)
			m_Watchs->OnEditField(wide_instruction);
		ShowWindow(SW_HIDE);
		return;
	} else if (nChar==VK_ESCAPE) {
		ShowWindow(SW_HIDE);
	}
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


UINT CWatchEdit::OnGetDlgCode()
{
	// TODO: Add your message handler code here and/or call default

	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTTAB;
}


// CWatchListCtrl

IMPLEMENT_DYNAMIC(CWatchListCtrl, CListCtrl)

CWatchListCtrl::CWatchListCtrl()
{

}

CWatchListCtrl::~CWatchListCtrl()
{
}


BEGIN_MESSAGE_MAP(CWatchListCtrl, CListCtrl)
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()



// CWatchListCtrl message handlers

void CWatchListCtrl::OnLButtonDown(UINT nFlags, CPoint point)
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
						m_Watchs->OnSelectField(0, n, rect);
						return;
						*/
					} else {
/*						rect.left = x0;
						rect.right = x1;
						m_Watchs->OnSelectField(1, n, rect);
						return;*/
					}
				}
			}
		}
	}
	CListCtrl::OnLButtonDown(nFlags, point);
}

void CWatchListCtrl::StartEdit(int item)
{
	if (item >= GetItemCount()) {
		InsertItem(GetItemCount(), L"", 0);
		item = GetItemCount()-1;
	}
	int x0 = GetColumnWidth(0);
	if (item >=0 && m_Watchs) {
		CRect rect;
		if (GetItemRect(item, &rect, LVIR_BOUNDS)) {
			rect.right = x0;
			m_Watchs->OnSelectField(0, item, rect);
		}
	}
}

#define MAX_DELETE_ITEMS 64
void CWatchListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	if (nChar == VK_DELETE && m_Watchs) {
		int selected[MAX_DELETE_ITEMS];
		int num_selected = 0;
		POSITION p = GetFirstSelectedItemPosition();
		while (p && num_selected < MAX_DELETE_ITEMS)
			selected[num_selected++] = GetNextSelectedItem(p);
		for (int n = 0; n<num_selected; n++) {
			DeleteItem(selected[n]);
			m_Watchs->RemoveWatch(selected[n]);
			for (int m = n+1; m<num_selected; m++) {
				if (selected[m]>selected[n])
					selected[m]--;
			}
		}
		Invalidate();
		theApp.GetMainFrame()->InvalidateAll();
	} else if (nChar == VK_RETURN) {
		if (GetItemCount()==0)
			StartEdit(0);
		else if (POSITION p = GetFirstSelectedItemPosition()) {
			int n = GetNextSelectedItem(p);
			if (p == nullptr)
				StartEdit(n);
		} else
			StartEdit(GetItemCount());
	}

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CWatchListCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (GetItemCount()==0)
		StartEdit(0);
	else {
		UINT f;
		int item = HitTest(point, &f);
		if (f & LVHT_ONITEM)
			StartEdit(item);
		else
			StartEdit(GetItemCount());
	}
}

BOOL CWatchListCtrl::PreTranslateMessage(MSG* pMsg)
{
	// VK_RETURN gets stolen by CListCtrl
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam==VK_RETURN) {
		if (GetItemCount()==0)
			StartEdit(0);
		else if (POSITION p = GetFirstSelectedItemPosition()) {
			int n = GetNextSelectedItem(p);
			if (p == nullptr)
				StartEdit(n);
		} else
			StartEdit(GetItemCount());
		return true;
	}
	return CListCtrl::PreTranslateMessage(pMsg);
}

// CWatchView

IMPLEMENT_DYNAMIC(CWatchView, CDockablePane)

CWatchView::CWatchView()
{
	m_field_col = m_field_row = -1;
}

CWatchView::~CWatchView()
{
	while (m_rpn.size())
		RemoveWatch((int)(m_rpn.size()-1));
}

BEGIN_MESSAGE_MAP(CWatchView, CDockablePane)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
END_MESSAGE_MAP()



// CWatchView message handlers




void CWatchView::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(&rect);

	m_listctrl.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_NOZORDER);

	LVCOLUMN column = { 0 };
	int orig[2] = { 0 };
	int total = 0;
	for (int c = 0; c<2; c++) {
		column.mask = LVCF_WIDTH;
		if (m_listctrl.GetColumn(c, &column))
			orig[c] = column.cx;
	}
	if (!orig[0] && !orig[1]) {
		orig[0] = 1;
		orig[1] = 1;
	} else if (!orig[0])
		orig[0] = orig[1];
	else if (!orig[1])
		orig[1] = orig[0];
	total = orig[0] + orig[1];
	column = { 0 };
	column.mask = LVCF_WIDTH;
	column.cx = orig[0] * rect.Width() / total;
	m_listctrl.SetColumn(0, &column);
	column.mask = LVCF_WIDTH;
	column.cx = rect.Width() - orig[0] * rect.Width() / total;
	m_listctrl.SetColumn(1, &column);

	Invalidate();
}


BOOL CWatchView::OnEraseBkgnd(CDC* pDC)
{
	CRect rt;
	GetClientRect(&rt);

	CBrush br;
	br.CreateStockObject(WHITE_BRUSH);
	pDC->SelectObject(&br);

	pDC->Rectangle(&rt); // I just fill all the BG with white brush  

	return CDockablePane::OnEraseBkgnd(pDC);
}

void CWatchView::OnSelectField(int column, int row, CRect &rect)
{
	m_field_col = column;
	m_field_row = row;
	m_edit_field.MoveWindow(&rect);
	if (CWnd *editItem = GetDlgItem(ID_WATCH_EDIT))
		editItem->SetWindowText(m_listctrl.GetItemText(row, column));
	m_edit_field.ShowWindow(SW_SHOW);
	m_edit_field.SetFocus();
	m_edit_field.Invalidate();
}

void CWatchView::OnEditField(wchar_t *text)
{
	if (m_field_col>=0 && m_field_row>=0) {
		m_listctrl.SetItemText(m_field_row, m_field_col, text);
		SetWatch(m_field_row, text);
	}
	m_field_col = m_field_row = -1;
}

void CWatchView::OnClearEditField()
{
	m_field_col = m_field_row = -1;
}

int CWatchView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	GetClientRect(&rect);

	if (!m_edit_field.Create(ES_LEFT | WS_CHILD | WS_BORDER | ES_WANTRETURN, rect, this, ID_WATCH_EDIT)) {
		TRACE0("Failed to create assembler input for code view\n");
		return FALSE; // failed to create
	}
	m_edit_field.SetWatchs(this);


	if (!m_listctrl.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL
							  | LVS_REPORT, rect, this, ID_WATCH_LIST)) {
		TRACE0("Failed to create Nodes List\n");
		return -1;
	}
	m_listctrl.SetWatchs(this);
	m_listctrl.SetExtendedStyle(m_listctrl.GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	LVCOLUMN column = { 0 };

	column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_ORDER | LVCF_TEXT;
	column.fmt = LVCFMT_FILL | LVCFMT_FIXED_WIDTH;
	column.cx = 96;
	column.iOrder = 1;
	column.pszText = L"Value";

	m_listctrl.InsertColumn(0, &column);

	column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_ORDER | LVCF_TEXT;
	column.fmt = LVCFMT_FIXED_WIDTH;
	column.cx = 96;
	column.iOrder = 0;
	column.pszText = L"Expression";

	m_listctrl.InsertColumn(0, &column);// L"Column 1");

	return 0;
}

void CWatchView::RemoveWatch(int index)
{
	if (index<(int)m_rpn.size()) {
		free(m_rpn[index].pRPN);
		m_rpn.erase(m_rpn.begin() + index);
	}
}

void CWatchView::EvaluateItem(int item)
{
	if (item<0 || item>=(int)m_rpn.size())
		return;

	uint8_t *rpn = m_rpn[item].pRPN;
	wchar_t buf[128];
	if (rpn && rpn[0]) {
		if (m_rpn[item].type == WT_NORMAL) {
			int result = EvalExpression(rpn);
			const wchar_t *fmt = L"$%02x";
			if (result<0) {
				result = -result;
				if (result>256) {
					if (result > 65536)
						fmt = L"-$%06x";
					else
						fmt = L"-$%04x";
				} else
					fmt = L"-$%02x";
			} else {
				if (result>256) {
					if (result > 65536)
						fmt = L"$%06x";
					else
						fmt = L"$%04x";
				} else
					fmt = L"$%02x";
			}
			wsprintf(buf, fmt, result);
		} else if (m_rpn[item].type == WT_BYTES) {
			int addr = EvalExpression(rpn);
			int x1 = m_listctrl.GetColumnWidth(1);
			int num_bytes = x1 / 24; // rough estimate
			wchar_t *o = buf;
			o += swprintf_s(o, 128-(o-buf), L"%04x ", addr);
			for (int b = 0; b<num_bytes && (o-buf)<(sizeof(buf)/sizeof(buf[0])); b++)
				o += swprintf_s(o, 128-(o-buf), L"%02x ", Get6502Byte(addr + b));
		} else {
			int addr = EvalExpression(rpn);
			char disbuf[128];
			int chars = 0;
			char *o = disbuf;
			o += sprintf_s(o, 128-(o-disbuf), "%04x ", addr);
			Disassemble(addr, o, (int)(128-(o-disbuf)), chars, true);
			size_t newlen = 0;
			mbstowcs_s(&newlen, buf, disbuf, sizeof(buf)/sizeof(buf[0]));
		}
	}
	m_listctrl.SetItemText(item, 1, buf);
}

void CWatchView::EvaluateAll()
{
	for (size_t n = 0; n<m_rpn.size(); ++n)
		EvaluateItem((int)n);
}

void CWatchView::SetWatch(int index, const wchar_t *expression)
{
	uint8_t rpn[512];
	WatchType type = WT_NORMAL;
	if (expression[0]==L'*') {
		type = WT_BYTES;
		++expression;
	} else if (_wcsnicmp(L"dis", expression, 3) == 0) {
		wchar_t c = expression[3];
		if (!((c>=L'A' && c<='Z') || (c>=L'0' && c<='9') || (c>=L'a' && c<='z'))) {
			type = WT_DISASM;
			expression += 3;
		}
	}

	uint32_t len = BuildExpression(expression, rpn, sizeof(rpn)/sizeof(rpn[0]));
	if (index < (int)m_rpn.size()) {
		if (len > m_rpn[index].length) {
			if (m_rpn[index].pRPN)
				free(m_rpn[index].pRPN);
			m_rpn[index].pRPN = (uint8_t*)malloc(len);
			m_rpn[index].length = len;
		}
		if (len)
			memcpy(m_rpn[index].pRPN, rpn, len);
	} else {
		index = (int)m_rpn.size();
		struct sWatchPoint w;
		w.length = len;
		w.pRPN = len ? (uint8_t*)malloc(len) : 0;
		w.type = type;
		if (w.pRPN)
			memcpy(w.pRPN, rpn, len);
		m_rpn.push_back(w);
	}
	EvaluateItem(index);
}

