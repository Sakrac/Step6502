
// Step6502.h : main header file for the Step6502 application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "MainFrm.h"

// CStep6502App:
// See Step6502.cpp for the implementation of this class
//

class CStep6502App : public CWinAppEx
{
public:
	CStep6502App();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
protected:
	CMainFrame	*pMain;

public:

	CMainFrame* GetMainFrame() { return pMain;  }

	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnButtonStop();
};

extern CStep6502App theApp;
