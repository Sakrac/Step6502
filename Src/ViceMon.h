#pragma once


// CViceMon

class CViceMon : public CRichEditCtrl
{
	DECLARE_DYNAMIC(CViceMon)

public:
	CViceMon();
	virtual ~CViceMon();

	bool m_enabled;
	int m_prompt;


protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};


// CViceMonPane

class CViceMonPane : public CDockablePane
{
	DECLARE_DYNAMIC(CViceMonPane)

public:
	CViceMonPane();
	virtual ~CViceMonPane();

	CViceMon m_edit;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void AddText(const char * buf, int len);
	void Clear();
	void EnableInput(bool enable);
	afx_msg void OnRawInput(UINT nInputcode, HRAWINPUT hRawInput);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};


