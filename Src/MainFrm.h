
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "ChildView.h"
#include "RegisterPane.h"
#include "memory.h"
#include "CodeView.h"
#include "LoadAddress.h"
#include "GraphicView.h"
#include "BreakpointList.h"
#include "WatchView.h"
#include "ViceMon.h"

// CMainToolBar

class CMainToolBar : public CMFCToolBar
{
	DECLARE_DYNAMIC(CMainToolBar)

public:
	CMainToolBar();
	virtual ~CMainToolBar();
	void OnUpdateCmdUI(CFrameWnd *pFrame, BOOL bDisableIfNoHndler);

protected:
	void EnableToolbarButton(int id, bool enable);

	DECLARE_MESSAGE_MAP()
public:
};

enum S6Views {
	S6_REG,
	S6_CODE,
	S6_MEM1,
	S6_MEM2,
	S6_GFX
};

class CMainFrame : public CFrameWndEx
{
	
public:
	CMainFrame();
protected: 
	DECLARE_DYNAMIC(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	void InvalidateRegs();
	void InvalidateMem();
	void InvalidateAll();

	void FocusPC() { m_code.FocusPC(); }
	void FocusAddr(uint16_t addr) { m_code.FocusAddr(addr); }
	void TryLoadBinary(const wchar_t *name);
	void BreakpointChanged(uint32_t id=~0UL) { m_breakpoints.Rebuild(id); }
	void MachineUpdated();
	void VicePrint( const char* buf, int len );
	void ViceInput(bool enable);
	void StepOver();
	void Step(bool back);
	void ToggleBreakpoint();
	void Go(bool back);
	void Stop();
	CFont &Font() { return m_font; }

	int				  m_actionID;	// action ID increments when stepping or running
	S6Views           m_currView;

protected:  // control bar embedded members
	CMFCMenuBar       m_wndMenuBar;
	CMainToolBar      m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	CMFCToolBarImages m_UserImages;
	CRegisterPane     m_registers;
	CMemory		      m_memory;
	CMemory		      m_memory2;
	CCodeView         m_code;
	CChildView        m_wndView;
	CLoadAddress      m_loadAddressDialog;
	CGraphicView      m_graphicView;
	CBreakpointList   m_breakpoints;
	CWatchView        m_watch;
	CViceMonPane      m_vice;

	int               m_currFontSize;
	CFont             m_font;

	void LoadBinary(CString &file, int filetype, int address, int setPC, int forceAddress, int resetUndo);
	void UpdateFontSize(int newSize);

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	afx_msg void OnViewOutputWindow();
	afx_msg void OnUpdateViewOutputWindow(CCmdUI* pCmdUI);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	DECLARE_MESSAGE_MAP()

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
public:
	afx_msg void OnButtonStop();
	afx_msg void OnButtonStepBack();
	afx_msg void OnButtonStep();
	afx_msg void OnButtonReverse();
	afx_msg void OnButtonGo();
	afx_msg void OnButtonBreak();
	afx_msg void OnButtonIrq();
	afx_msg void OnButtonNmi();
	afx_msg void OnButtonReload();
	afx_msg void OnButtonReset();
	afx_msg void OnButtonLoad();
	afx_msg void OnButtonVice();
	afx_msg void OnFontsize12();
	afx_msg void OnFontsize14();
	afx_msg void OnFontsize16();
	afx_msg void OnFontsize18();
	afx_msg void OnFontsize20();
	afx_msg void OnFontsize22();
	afx_msg void OnFontsize24();
	afx_msg void OnEditCopy();
};


#pragma once


