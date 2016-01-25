#pragma once
#include <vector>
#include <map>

class CWatchView;

// CWatchListCtrl

class CWatchListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CWatchListCtrl)

public:
	CWatchListCtrl();
	virtual ~CWatchListCtrl();

	void SetWatchs(CWatchView *pBP) {
		m_Watchs = pBP;
	}
protected:
	void StartEdit(int item);
	CWatchView *m_Watchs;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};


// CWatchEdit

class CWatchEdit : public CEdit
{
	DECLARE_DYNAMIC(CWatchEdit)

public:
	CWatchEdit();
	virtual ~CWatchEdit();
	void SetWatchs(CWatchView *pBP) {
		m_Watchs = pBP;
	}
protected:
	CWatchView *m_Watchs;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
};

enum WatchType {
	WT_NORMAL,
	WT_BYTES,
	WT_DISASM
};

struct sWatchPoint {
	uint8_t *pRPN;
	uint16_t length;
	uint8_t type;

};

// CWatchView

class CWatchView : public CDockablePane
{
	DECLARE_DYNAMIC(CWatchView)

	struct sIDInfo {
		int id;
		bool enabled;
		sIDInfo() : id(-1), enabled(false) {}
		sIDInfo(int _id) : id(_id), enabled(true) {}
		sIDInfo(int _id, bool _en) : id(_id), enabled(_en) {}
	};

public:
	CWatchView();
	virtual ~CWatchView();

	void OnEditField(wchar_t *text);
	void OnSelectField(int column, int row, CRect &rect);
	void OnClearEditField();
	void RemoveWatch(int index);
	void SetWatch(int index, const wchar_t *expression);
	void EvaluateItem(int item);
	void EvaluateAll();

protected:
	CWatchListCtrl m_listctrl;
	CImageList m_icons;
	CBitmap m_Watch_image;
	CWatchEdit m_edit_field;
	std::vector<struct sWatchPoint> m_rpn;
	int m_field_col, m_field_row;
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


#pragma once
