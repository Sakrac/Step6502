#pragma once
#include <vector>

class CBreakpointList;

// CBPListCtr

class CBPListCtr : public CListCtrl
{
	DECLARE_DYNAMIC(CBPListCtr)

public:
	CBPListCtr();
	virtual ~CBPListCtr();

	void SetBreakpoints(CBreakpointList *pBP) {
		m_breakpoints = pBP;
	}
protected:
	CBreakpointList *m_breakpoints;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};


// CBreakpointEdit

class CBreakpointEdit : public CEdit
{
	DECLARE_DYNAMIC(CBreakpointEdit)

public:
	CBreakpointEdit();
	virtual ~CBreakpointEdit();
	void SetBreakpoints(CBreakpointList *pBP) {
		m_breakpoints = pBP;
	}
protected:
	CBreakpointList *m_breakpoints;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
};



// CBreakpointList

class CBreakpointList : public CDockablePane
{
	DECLARE_DYNAMIC(CBreakpointList)

public:
	CBreakpointList();
	virtual ~CBreakpointList();

	void OnEditField(wchar_t *text);
	void OnSelectField(int column, int row, CRect &rect);
	void OnClearEditField();
	void Rebuild();
	void RemoveBPByIndex(int idx);
	void GoToBPByIndex(int idx);
protected:
	CBPListCtr m_bp_listctrl;
	CImageList m_icons;
	CBitmap m_breakpoint_image;
	CBreakpointEdit m_edit_field;
	int m_field_col, m_field_row;
	std::vector<int> m_BP_ID;	// ID's of each line of breakpoints

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


