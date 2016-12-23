#pragma once
#include <stdint.h>

class CCodeView;

// CCodeAddress

class CCodeAddress : public CEdit
{
	DECLARE_DYNAMIC(CCodeAddress)

public:
	CCodeAddress();
	virtual ~CCodeAddress();

	void SetCode(CCodeView *pMem) { m_Code = pMem; }

protected:
	CCodeView *m_Code;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
};

// CAsmEdit

class CAsmEdit : public CEdit
{
	DECLARE_DYNAMIC(CAsmEdit)

public:
	CAsmEdit();
	virtual ~CAsmEdit();

	void SetCode(CCodeView *pMem, uint16_t addr) { m_Code = pMem; m_addr = addr; }
protected:
	CCodeView *m_Code;
	uint16_t m_addr;
	DECLARE_MESSAGE_MAP()
public:
	//	afx_msg void OnEnChange();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
};

// CCodeView

class CCodeView : public CDockablePane
{
	DECLARE_DYNAMIC(CCodeView)
	enum { MAX_CODE_LINES = 256 };
public:
	CCodeView();
	virtual ~CCodeView();

	uint16_t currAddr;
	uint16_t bottomAddr;

	void FocusPC();
	void FocusAddr(uint16_t addr); // make sure addr is in window
	void SetBP();
	void OnAssembled(uint16_t nextAddr);

protected:
	CCodeAddress m_address;
	CAsmEdit m_assemble;
	uint16_t m_lineHeight;
	uint16_t m_charWidth;
	uint16_t m_numLines;
	uint16_t m_cursor;
	int m_selCursor;
	uint16_t m_selFirst;
	int m_nextAssembleAddr;
	bool m_selecting;
	bool m_leftmouse;

	int m_aLinePC[MAX_CODE_LINES];

	CBitmap m_breakpoint_image;
	CBitmap m_pc_arrow_image;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnButtonStop();
	afx_msg void OnButtonGo();
	afx_msg void OnButtonReverse();
	afx_msg void OnButtonStep();
	afx_msg void OnButtonStepBack();
	afx_msg void OnButtonBreak();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnDropFiles(HDROP hDropInfo);

	void OnEditCopy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};


