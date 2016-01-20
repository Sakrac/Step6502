#pragma once
#include <stdint.h>

class CMemory;


// CMemoryColumns

class CMemoryColumns : public CEdit
{
	DECLARE_DYNAMIC(CMemoryColumns)

public:
	CMemoryColumns();
	virtual ~CMemoryColumns();

	void SetMemory(CMemory *pMem) { m_memory = pMem; }

protected:
	CMemory *m_memory;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
};

// CMemoryAddress

class CMemoryAddress : public CEdit
{
	DECLARE_DYNAMIC(CMemoryAddress)

public:
	CMemoryAddress();
	virtual ~CMemoryAddress();

	void SetMemory(CMemory *pMem) { m_memory = pMem;  }

protected:
	CMemory *m_memory;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
};

struct SMemDelta {
	int m_actionId;
	uint8_t *m_mirror;
	uint16_t m_addr, m_end, m_size;

	SMemDelta() : m_actionId(0x12345678), m_mirror(nullptr), m_addr(0), m_end(0), m_size(0) {}
};

// CMemory

class CMemory : public CDockablePane
{
	DECLARE_DYNAMIC(CMemory)

public:
	CMemory();
	virtual ~CMemory();

	uint16_t currAddr;
	uint16_t prevAddr;
	uint16_t fitBytesWidth;
	uint16_t fitLinesHeight;
	int bytesWide;

	// if currently editing cursor_x != 0xff
	uint8_t cursor_x, cursor_y;

	CPoint selectPoint;
	bool selectAddr;
	int m_viewIndex;

protected:
	CMemoryAddress m_address;
	CMemoryColumns m_columns;
	uint16_t m_editLeft, m_editRight;
	uint16_t m_charWidth, m_lineHeight;

	SMemDelta m_currDelta;
	SMemDelta m_prevDelta;

	uint16_t m_selFirst;
	uint16_t m_selAddr;
	uint16_t m_selEnd;
	bool m_selecting;
	bool m_mouse_left_down;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	void OnEditCopy();
};


