#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "Step6502.h"
#include "MainFrm.h"
#include "machine.h"
#include "Sym.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CStep6502App

BEGIN_MESSAGE_MAP(CStep6502App, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CStep6502App::OnAppAbout)
	ON_COMMAND(ID_BUTTON_STOP, &CStep6502App::OnButtonStop)
END_MESSAGE_MAP()


// CStep6502App construction

CStep6502App::CStep6502App() : pMain(nullptr)
{
	m_bHiColorIcons = TRUE;

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("Step6502.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CStep6502App object

CStep6502App theApp;


// CStep6502App initialization

BOOL CStep6502App::InitInstance()
{
	Initialize6502();
	CWinAppEx::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("x65"));

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;

	pMain = pFrame;

	m_pMainWnd = pFrame;
	// create and load the frame with its resources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);





	// The one and only window has been initialized, so show and update it
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	return TRUE;
}

int CStep6502App::ExitInstance()
{
	//TODO: handle additional resources you may have added
	pMain = nullptr;
	Shutdown6502();
	ShutdownSymbols();
	return CWinAppEx::ExitInstance();
}

// CStep6502App message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	ON_WM_QUERYDRAGICON()
END_MESSAGE_MAP()

// App command to run the dialog
void CStep6502App::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CStep6502App customization load/save methods

void CStep6502App::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CStep6502App::LoadCustomState()
{
}

void CStep6502App::SaveCustomState()
{
}

// CStep6502App message handlers



void CStep6502App::OnButtonStop()
{
	// TODO: Add your command handler code here
}


