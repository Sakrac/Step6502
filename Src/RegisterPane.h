#pragma once
#include <stdint.h>

// CRegisterPane

class CRegisterPane : public CDockablePane
{
	DECLARE_DYNAMIC(CRegisterPane)

public:
	CRegisterPane();
	virtual ~CRegisterPane();

protected:
	int		m_lineHeight;
	int		m_charWidth;

	uint8_t	m_cursor_x;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg UINT OnGetDlgCode();
};


