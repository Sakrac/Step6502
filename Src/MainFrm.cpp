
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "Step6502.h"
#include "MainFrm.h"
#include "machine.h"
#include "Sym.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainToolBar

IMPLEMENT_DYNAMIC(CMainToolBar, CMFCToolBar)

CMainToolBar::CMainToolBar()
{

}

CMainToolBar::~CMainToolBar()
{
}


BEGIN_MESSAGE_MAP(CMainToolBar, CMFCToolBar)
END_MESSAGE_MAP()



// CMainToolBar message handlers




// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
	ON_COMMAND(ID_VIEW_OUTPUTWND, &CMainFrame::OnViewOutputWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OUTPUTWND, &CMainFrame::OnUpdateViewOutputWindow)
	ON_WM_SETTINGCHANGE()
	ON_COMMAND(ID_BUTTON_STOP, &CMainFrame::OnButtonStop)
	ON_COMMAND(ID_BUTTON_STEP_BACK, &CMainFrame::OnButtonStepBack)
	ON_COMMAND(ID_BUTTON_STEP, &CMainFrame::OnButtonStep)
	ON_COMMAND(ID_BUTTON_REVERSE, &CMainFrame::OnButtonReverse)
	ON_COMMAND(ID_BUTTON_GO, &CMainFrame::OnButtonGo)
	ON_COMMAND(ID_BUTTON_BREAK, &CMainFrame::OnButtonBreak)
	ON_COMMAND(ID_BUTTON_IRQ, &CMainFrame::OnButtonIrq)
	ON_COMMAND(ID_BUTTON_NMI, &CMainFrame::OnButtonNmi)
	ON_COMMAND(ID_BUTTON_RELOAD, &CMainFrame::OnButtonReload)
	ON_COMMAND(ID_BUTTON_RESET, &CMainFrame::OnButtonReset)
	ON_COMMAND(ID_BUTTON_LOAD, &CMainFrame::OnButtonLoad)
	ON_COMMAND(ID_FONTSIZE_12, &CMainFrame::OnFontsize12)
	ON_COMMAND(ID_FONTSIZE_14, &CMainFrame::OnFontsize14)
	ON_COMMAND(ID_FONTSIZE_16, &CMainFrame::OnFontsize16)
	ON_COMMAND(ID_FONTSIZE_18, &CMainFrame::OnFontsize18)
	ON_COMMAND(ID_FONTSIZE_20, &CMainFrame::OnFontsize20)
	ON_COMMAND(ID_FONTSIZE_22, &CMainFrame::OnFontsize22)
	ON_COMMAND(ID_FONTSIZE_24, &CMainFrame::OnFontsize24)
	ON_COMMAND(ID_EDIT_COPY, &CMainFrame::OnEditCopy)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);

	m_currFontSize = theApp.GetProfileInt(L"Font", L"Size", 12);
	m_actionID = 0;

	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_currFontSize;
	wcscpy_s(lf.lfFaceName, L"Lucida Console");
	m_font.CreateFontIndirect(&lf);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;

	if (!m_wndMenuBar.Create(this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	// create a view to occupy the client area of the frame
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_wndToolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	// Allow user-defined toolbars operations:
	InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

	if (!m_wndStatusBar.Create(this)) {
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndMenuBar);
	DockPane(&m_wndToolBar);


	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// create docking windows
	if (!CreateDockingWindows())
	{
		TRACE0("Failed to create docking windows\n");
		return -1;
	}

	m_memory.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_memory);

	m_code.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_code);

	m_memory2.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_memory2);

	m_registers.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_registers);

	m_graphicView.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_graphicView);

	m_breakpoints.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_breakpoints);

	m_watch.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_watch);

	// set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == NULL)
	{
		// load user-defined toolbar images
		if (m_UserImages.Load(_T(".\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_UserImages);
		}
	}

	// enable menu personalization (most-recently used commands)
	// TODO: define your own basic commands, ensuring that each pulldown menu has at least one basic command.
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_BUTTON_STOP);
	lstBasicCommands.AddTail(ID_BUTTON_GO);
	lstBasicCommands.AddTail(ID_BUTTON_REVERSE);
	lstBasicCommands.AddTail(ID_BUTTON_STEP);
	lstBasicCommands.AddTail(ID_BUTTON_STEP_BACK);
	lstBasicCommands.AddTail(ID_BUTTON_BREAK);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);

	CMFCToolBar::SetBasicCommands(lstBasicCommands);

	return 0;
}

void CMainFrame::InvalidateRegs()
{
	if (m_registers)
		m_registers.Invalidate();
}

void CMainFrame::InvalidateMem()
{
	if (m_memory)
		m_memory.Invalidate();
	if (m_memory2)
		m_memory2.Invalidate();
}

void CMainFrame::InvalidateAll()
{
	m_code.Invalidate();
	m_memory.Invalidate();
	m_memory2.Invalidate();
	m_registers.Invalidate();
	Invalidate();
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	return TRUE;
}

BOOL CMainFrame::CreateDockingWindows()
{
	BOOL bNameValid;

	// Create register pane window
	CString strRegsWnd;
	bNameValid = strRegsWnd.LoadString(IDS_REGISTERS_WND);
	ASSERT(bNameValid);
	if (!m_registers.Create(strRegsWnd, this, CRect(0, 0, 640, 100), TRUE, ID_VIEW_REGISTERS, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Registers window\n");
		return FALSE; // failed to create
	}

	// Create memory pane window
	CString strMemWnd;
	bNameValid = strMemWnd.LoadString(IDS_MEMORY_NAME);
	ASSERT(bNameValid);
	if (!m_memory.Create(strMemWnd, this, CRect(0, 0, 640, 480), TRUE, ID_VIEW_MEMORY, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Memory window\n");
		return FALSE; // failed to create
	}
	m_memory.m_viewIndex = 0;

	// Create memory pane window
	CString strMem2Wnd;
	bNameValid = strMem2Wnd.LoadString(IDS_MEMORY2_NAME);
	ASSERT(bNameValid);
	if (!m_memory2.Create(strMem2Wnd, this, CRect(0, 0, 640, 480), TRUE, ID_VIEW_MEMORY2, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Memory window\n");
		return FALSE; // failed to create
	}
	m_memory2.m_viewIndex = 1;

	// Create code pane window
	CString strCodeWnd;
	bNameValid = strCodeWnd.LoadString(IDS_CODE_VIEW);
	ASSERT(bNameValid);
	if (!m_code.Create(strCodeWnd, this, CRect(0, 0, 256, 512), TRUE, ID_VIEW_CODE, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Code window\n");
		return FALSE; // failed to create
	}

	CString strGraphicWnd;
	bNameValid = strGraphicWnd.LoadString(IDS_GRAPHIC_VIEW);
	ASSERT(bNameValid);
	if (!m_graphicView.Create(strGraphicWnd, this, CRect(0, 0, 640, 480), TRUE, ID_VIEW_GRAPHIC, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Graphic window\n");
		return FALSE; // failed to create
	}
	
	CString strBreakptsWnd;
	bNameValid = strBreakptsWnd.LoadString(IDS_BREAKPOINTS);
	ASSERT(bNameValid);
	if (!m_breakpoints.Create(strBreakptsWnd, this, CRect(0, 0, 640, 480), TRUE, ID_BREAKPOINTS, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Breakpoints window\n");
		return FALSE; // failed to create
	}

	CString strWatchWnd;
	bNameValid = strWatchWnd.LoadString(IDS_WATCH);
	ASSERT(bNameValid);
	if (!m_watch.Create(strWatchWnd, this, CRect(0, 0, 640, 480), TRUE, ID_WATCH_VIEW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_TOP | CBRS_FLOAT_MULTI)) {
		TRACE0("Failed to create Watch window\n");
		return FALSE; // failed to create
	}

	SetDockingWindowIcons(theApp.m_bHiColorIcons);
	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{
	HICON hOutputBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(bHiColorIcons ? IDI_OUTPUT_WND_HC : IDI_OUTPUT_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
	// forward focus to the view window
	m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWndEx::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

void CMainFrame::OnViewOutputWindow()
{
}

void CMainFrame::OnUpdateViewOutputWindow(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}


BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// base class does the real work

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}


	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != NULL)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}


void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CFrameWndEx::OnSettingChange(uFlags, lpszSection);
}


void CMainFrame::OnButtonStop()
{
	// TODO: Add your command handler code here
	CPUStop();

	m_code.FocusPC();

	m_registers.Invalidate();
	m_memory.Invalidate();
	m_code.Invalidate();
}

void CMainFrame::OnButtonStepBack()
{
	// TODO: Add your command handler code here
	CPUStepBack();
}


void CMainFrame::OnButtonStep()
{
	// TODO: Add your command handler code here
	CPUStep();
}


void CMainFrame::OnButtonReverse()
{
	// TODO: Add your command handler code here
	CPUReverse();
}

void CMainFrame::MachineUpdated()
{
	m_watch.EvaluateAll();
	m_watch.Invalidate();
	InvalidateAll();
}

void CMainToolBar::EnableToolbarButton(int id, bool enable)
{
	int index = CommandToIndex(id);
	if (index >=0) {
		UINT iStyle = GetButtonStyle(index);
		if (enable)
			SetButtonStyle(index, iStyle & (~TBBS_DISABLED));
		else
			SetButtonStyle(index, iStyle | TBBS_DISABLED);
	}
}

void CMainToolBar::OnUpdateCmdUI(CFrameWnd *pFrame, BOOL bDisableIfNoHndler)
{
	CMFCToolBar::OnUpdateCmdUI(pFrame, bDisableIfNoHndler);
	bool running = IsCPURunning();
	EnableToolbarButton(ID_BUTTON_STOP, running);
	EnableToolbarButton(ID_BUTTON_GO, !running);
	EnableToolbarButton(ID_BUTTON_REVERSE, !running);
	EnableToolbarButton(ID_BUTTON_STEP, !running);
	EnableToolbarButton(ID_BUTTON_STEP_BACK, !running);
	EnableToolbarButton(ID_BUTTON_FONTSIZE, true);
}


void CMainFrame::OnButtonGo()
{
	// TODO: Add your command handler code here
	CPUGo();
	InvalidateAll();
}


void CMainFrame::OnButtonBreak()
{
	// TODO: Add your command handler code here
	m_code.SetBP();
}


void CMainFrame::OnButtonIrq()
{
	// TODO: Add your command handler code here
	CPUIRQ();
	InvalidateAll();
}


void CMainFrame::OnButtonNmi()
{
	// TODO: Add your command handler code here
	CPUNMI();
	InvalidateAll();
}


void CMainFrame::OnButtonReload()
{
	// TODO: Add your command handler code here
	if (!IsCPURunning()) {
		int fileType = theApp.GetProfileInt(L"Last binary file", L"Filetype", 0);
		CString address = theApp.GetProfileString(L"Last binary file", L"LoadAddress", L"800");
		int forceLoadAddress = theApp.GetProfileInt(L"Last binary file", L"Force Load Address", 0);
		int setPC = theApp.GetProfileInt(L"Last binary file", L"Set PC to Load Address", 1);
		int resetUndo = theApp.GetProfileInt(L"Last binary file", L"Reset backtrace", 1);


		wchar_t *end;
		int addr = wcstol((const wchar_t*)address+(address[0]=='$' ? 1 : 0), &end, 16);

		CString file = theApp.GetProfileString(L"Last binary file", L"Filename");
		if (file)
			LoadBinary(file, fileType, addr, setPC, forceLoadAddress, resetUndo);
	}
}


void CMainFrame::OnButtonReset()
{
	// TODO: Add your command handler code here
	CPUReset();
	InvalidateAll();
}


void CMainFrame::LoadBinary(CString &file, int filetype, int address, int setPC, int forceAddress, int resetUndo)
{
	FILE *f = nullptr;

	if (_wfopen_s(&f, file, L"rb")==0) {

		// assume .prg for now
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (filetype == 0 || filetype == 1) {
			int8_t addr8[2];
			fread(addr8, 1, 2, f);
			size_t addr = addr8[0] + (uint16_t(addr8[1])<<8);
			size -= 2;
			if (filetype == 1) {
				fread(addr8, 1, 2, f);
				size = addr8[0] + (uint16_t(addr8[1])<<8);
			}
			if (!forceAddress)
				address = (int)addr;
		}

		size_t read = size < size_t(0x10000-address) ?
			size : size_t(0x10000-address);

		uint8_t *trg = Get6502Mem((uint16_t)address);
		fread(trg, read, 1, f);
		fclose(f);

		if (setPC) {
			GetRegs().PC = address;
			FocusPC();
		}

		if (resetUndo) {
			ResetUndoBuffer();
		}

		ReadSymbols(file);
		m_breakpoints.Rebuild();

		InvalidateAll();
	}
}

void CMainFrame::TryLoadBinary(const wchar_t *file)
{
	m_loadAddressDialog.m_bFileType = theApp.GetProfileInt(L"Last binary file", L"Filetype", 0);
	m_loadAddressDialog.m_AddrStr = theApp.GetProfileString(L"Last binary file", L"LoadAddress", L"800");
	m_loadAddressDialog.m_bForceLoadAddress = theApp.GetProfileInt(L"Last binary file", L"Force Load Address", 0);
	m_loadAddressDialog.m_bResetUndo = theApp.GetProfileInt(L"Last binary file", L"Reset backtrace", 1);
	m_loadAddressDialog.m_bSetPCToLoadAddress = theApp.GetProfileInt(L"Last binary file", L"Set PC to Load Address", 1);

	if (m_loadAddressDialog.DoModal() == IDCANCEL)
		return;

	theApp.WriteProfileInt(L"Last binary file", L"Filetype", m_loadAddressDialog.m_bFileType);
	theApp.WriteProfileString(L"Last binary file", L"LoadAddress", m_loadAddressDialog.m_AddrStr);
	theApp.WriteProfileInt(L"Last binary file", L"Force Load Address", m_loadAddressDialog.m_bForceLoadAddress);
	theApp.WriteProfileInt(L"Last binary file", L"Set PC to Load Address", m_loadAddressDialog.m_bSetPCToLoadAddress);
	theApp.WriteProfileInt(L"Last binary file", L"Reset backtrace", m_loadAddressDialog.m_bResetUndo);

	// remember the file for reloading

	theApp.WriteProfileString(L"Last binary file", L"Filename", file);

	wchar_t *end;
	int addr = wcstol((const wchar_t*)m_loadAddressDialog.m_AddrStr+(m_loadAddressDialog.m_AddrStr[0]=='$' ? 1 : 0), &end, 16);

	LoadBinary(CString(file), m_loadAddressDialog.m_bFileType, addr, m_loadAddressDialog.m_bSetPCToLoadAddress,
			   m_loadAddressDialog.m_bForceLoadAddress, m_loadAddressDialog.m_bResetUndo);
}

void CMainFrame::OnButtonLoad()
{
	// can't load code while CPU is running
	if (IsCPURunning())
		return;

	// TODO: Add your command handler code here
	CFileDialog fileDialog(true);
	fileDialog.AddText(ID_LOAD_ADDRESS_TEXT, L"Address:");

	if (fileDialog.DoModal() == IDCANCEL)
		return;

	m_loadAddressDialog.m_bFileType = theApp.GetProfileInt(L"Last binary file", L"Filetype", 0);
	m_loadAddressDialog.m_AddrStr = theApp.GetProfileString(L"Last binary file", L"LoadAddress", L"800");
	m_loadAddressDialog.m_bForceLoadAddress = theApp.GetProfileInt(L"Last binary file", L"Force Load Address", 0);
	m_loadAddressDialog.m_bResetUndo = theApp.GetProfileInt(L"Last binary file", L"Reset backtrace", 1);
	m_loadAddressDialog.m_bSetPCToLoadAddress = theApp.GetProfileInt(L"Last binary file", L"Set PC to Load Address", 1);

	m_loadAddressDialog.DoModal();

	theApp.WriteProfileInt(L"Last binary file", L"Filetype", m_loadAddressDialog.m_bFileType);
	theApp.WriteProfileString(L"Last binary file", L"LoadAddress", m_loadAddressDialog.m_AddrStr);
	theApp.WriteProfileInt(L"Last binary file", L"Force Load Address", m_loadAddressDialog.m_bForceLoadAddress);
	theApp.WriteProfileInt(L"Last binary file", L"Set PC to Load Address", m_loadAddressDialog.m_bSetPCToLoadAddress);
	theApp.WriteProfileInt(L"Last binary file", L"Reset backtrace", m_loadAddressDialog.m_bResetUndo);

	CString file = fileDialog.GetPathName();

	// remember the file for reloading

	theApp.WriteProfileString(L"Last binary file", L"Filename", file);

	wchar_t *end;
	int addr = wcstol((const wchar_t*)m_loadAddressDialog.m_AddrStr+(m_loadAddressDialog.m_AddrStr[0]=='$' ? 1 : 0), &end, 16);

	LoadBinary(file, m_loadAddressDialog.m_bFileType, addr, m_loadAddressDialog.m_bSetPCToLoadAddress,
			   m_loadAddressDialog.m_bForceLoadAddress, m_loadAddressDialog.m_bResetUndo);
}


// D:\test\step6502\MainFrm.cpp : implementation file
//

void CMainFrame::UpdateFontSize(int newSize)
{
	if (m_currFontSize != newSize) {
		theApp.WriteProfileInt(L"Font", L"Size", newSize);
		m_currFontSize = newSize;

		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));
		lf.lfHeight = m_currFontSize;
		wcscpy_s(lf.lfFaceName, L"Lucida Console");
		m_font.DeleteObject();
		m_font.CreateFontIndirect(&lf);
		InvalidateAll();
	}
}

void CMainFrame::OnFontsize12()
{
	UpdateFontSize(12);
}

void CMainFrame::OnFontsize14()
{
	UpdateFontSize(14);
}

void CMainFrame::OnFontsize16()
{
	UpdateFontSize(16);
}

void CMainFrame::OnFontsize18()
{
	UpdateFontSize(18);
}

void CMainFrame::OnFontsize20()
{
	UpdateFontSize(20);
}

void CMainFrame::OnFontsize22()
{
	UpdateFontSize(22);
}

void CMainFrame::OnFontsize24()
{
	UpdateFontSize(24);
}


void CMainFrame::OnEditCopy()
{
	switch (m_currView) {
		case S6_REG:
			m_registers.OnEditCopy();
			break;
		case S6_GFX:
			m_graphicView.OnEditCopy();
			break;
		case S6_MEM1:
			m_memory.OnEditCopy();
			break;
		case S6_MEM2:
			m_memory2.OnEditCopy();
			break;
		case S6_CODE:
			m_code.OnEditCopy();
			break;
	}
}
