// CodeView.cpp : implementation file
//

#include "stdafx.h"
#include "Step6502.h"
#include "CodeView.h"
#include "machine.h"
#include "Sym.h"

// CCodeAddress

IMPLEMENT_DYNAMIC(CCodeAddress, CEdit)

CCodeAddress::CCodeAddress() : m_Code(nullptr)
{

}

CCodeAddress::~CCodeAddress()
{
}


BEGIN_MESSAGE_MAP(CCodeAddress, CEdit)
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

// CAsmEdit

IMPLEMENT_DYNAMIC(CAsmEdit, CEdit)

CAsmEdit::CAsmEdit() : m_Code(nullptr)
{

}

CAsmEdit::~CAsmEdit()
{
}


BEGIN_MESSAGE_MAP(CAsmEdit, CEdit)
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CAsmEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_RETURN && m_Code) {
		wchar_t wide_instruction[64];
		GetWindowText(wide_instruction, sizeof(wide_instruction)/sizeof(wide_instruction[0]));
		char instruction[64];
		size_t newlen = 0;
		wcstombs_s(&newlen, instruction, wide_instruction, sizeof(instruction)-1);
		int len = Assemble(instruction, m_addr);
		if (len)
			m_Code->OnAssembled(m_addr + len);
		return;
	} else if (nChar==VK_ESCAPE) {
		ShowWindow(SW_HIDE);
	}
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

UINT CAsmEdit::OnGetDlgCode()
{
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS;
}

void CAsmEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	ShowWindow(SW_HIDE);
}



void CCodeAddress::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_RETURN && m_Code) {
		wchar_t address[64], *end;
		GetWindowText(address, sizeof(address)/sizeof(address[0]));
		m_Code->currAddr = (uint16_t)wcstol(address, &end, 16);
		GetParent()->Invalidate();
		return;
	}

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


UINT CCodeAddress::OnGetDlgCode()
{
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS;
}



// CCodeView

IMPLEMENT_DYNAMIC(CCodeView, CDockablePane)

CCodeView::CCodeView() : currAddr(0xe000), m_cursor(0xffff),
m_nextAssembleAddr(-1), m_selecting(false), m_leftmouse(false)
{
}

CCodeView::~CCodeView()
{
}


BEGIN_MESSAGE_MAP(CCodeView, CDockablePane)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_MOUSEWHEEL()
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_SYSCOMMAND()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_BUTTON_STOP, &CCodeView::OnButtonStop)
	ON_COMMAND(ID_BUTTON_GO, &CCodeView::OnButtonGo)
	ON_COMMAND(ID_BUTTON_REVERSE, &CCodeView::OnButtonReverse)
	ON_COMMAND(ID_BUTTON_STEP, &CCodeView::OnButtonStep)
	ON_COMMAND(ID_BUTTON_STEP_BACK, &CCodeView::OnButtonStepBack)
	ON_COMMAND(ID_BUTTON_BREAK, &CCodeView::OnButtonBreak)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_DROPFILES()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()



// CCodeView message handlers



#define DBLBUF
#define CODE_BAR_HEIGHT 24
#define CODE_LEFT_MARGIN 24
#define CODE_MARGIN_LEFT 4
#define CODE_MARGIN_RIGHT 20

void CCodeView::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // Do not call CDockablePane::OnPaint() for painting messages
	CDC *pDC = &dc;

	CRect rect;
	GetClientRect(&rect);

#ifdef DBLBUF
	CDC *pPrevDC = pDC;
	HDC hdcMem = CreateCompatibleDC(pDC->GetSafeHdc());
	HBITMAP hbmMem = CreateCompatibleBitmap(pDC->GetSafeHdc(), rect.Width(), rect.Height());
	HANDLE hOld = SelectObject(hdcMem, hbmMem);
	pDC = CDC::FromHandle(hdcMem);
#endif
	if (pDC) {

#ifdef DBLBUF
		pDC->FillSolidRect(rect, pDC->GetBkColor());
#endif


		CFont *pPrevFont = pDC->SelectObject(&theApp.GetMainFrame()->Font());

		TEXTMETRIC metrics;
		pDC->GetTextMetrics(&metrics);

		int width = rect.Width();
		int height = rect.Height();
		int lineHeight = metrics.tmHeight + metrics.tmExternalLeading;
		int charWidth = metrics.tmAveCharWidth;
		int lines = (height-CODE_BAR_HEIGHT) / lineHeight;

		m_lineHeight = lineHeight;
		m_charWidth = charWidth;
		m_numLines = lines;

		wchar_t wide_str[512];
		char disasm[512];
		uint16_t curr_pc = GetRegs().PC;

		uint16_t addr = currAddr;

		bool isRunning = IsCPURunning();

		uint16_t *pBP = GetPCBreakpoints(), nBP = GetNumPCBreakpoints();
		bool was_label = false;

		pDC->SetBkMode(TRANSPARENT);
		for (int l = 0; l<lines; l++) {
			int strlen = sprintf_s(disasm, sizeof(disasm), "%04x ", addr);
			int cmdlen = 0;
			size_t newlen = 0;
			uint16_t instr_len = Disassemble(addr, disasm+strlen, sizeof(disasm)-strlen, cmdlen);
			mbstowcs_s(&newlen, wide_str, disasm, sizeof(wide_str)/sizeof(wide_str[0]));

			CRect textRect = rect;
			textRect.top += l * lineHeight + CODE_BAR_HEIGHT;
			textRect.left += CODE_LEFT_MARGIN;

			const wchar_t *label = !was_label ? GetSymbol(addr) : nullptr;

			if (m_cursor != 0xffff && !isRunning) {
				COLORREF prevCol = pDC->GetBkColor();
				int left = CODE_LEFT_MARGIN;
				int top = m_cursor * lineHeight + CODE_BAR_HEIGHT;
				CRect cursor_rect(left, top, rect.right - charWidth, top + lineHeight - 2);
				CBrush brush;
				brush.CreateSolidBrush(RGB(192, 192, 192));
				pDC->FrameRect(&cursor_rect, &brush);
				pDC->SetBkColor(prevCol);
			}

			if (m_selecting) {
				if ((l>=m_selCursor && addr<=m_selFirst) || (addr>=m_selFirst && l<=m_selCursor)) {
					CRect selRect = textRect;
					selRect.bottom = selRect.top + lineHeight;
					pDC->FillSolidRect(&selRect, RGB(224, 224, 255));
				}
			}


			if (label) {
				wsprintf(wide_str, L"%s:", label);
				textRect.left = rect.left;
				pDC->SetTextColor(RGB(0, 0, 0));
				pDC->DrawText(wide_str, &textRect, DT_TOP | DT_LEFT | DT_SINGLELINE);
				was_label = true;
				if (l<MAX_CODE_LINES)
					m_aLinePC[l] = -1;
			} else {
				for (uint16_t b = 0; b<nBP; b++) {
					if (addr == pBP[b]) {
						CDC otherDC;
						otherDC.CreateCompatibleDC(pDC);
						CBitmap *pPrevBmp = otherDC.SelectObject(&m_breakpoint_image);
						pDC->BitBlt(6, (l*2+1) * lineHeight/2 - 6 + CODE_BAR_HEIGHT, 12, 12, &otherDC, 0, 0, SRCCOPY);
						otherDC.SelectObject(pPrevBmp);
						break;
					}
				}

				if (addr == curr_pc && !isRunning) {
					CDC otherDC;
					otherDC.CreateCompatibleDC(pDC);
					CBitmap *pPrevBmp = otherDC.SelectObject(&m_pc_arrow_image);
					pDC->BitBlt(6, (l*2+1) * lineHeight/2 - 6 + CODE_BAR_HEIGHT, 12, 12, &otherDC, 0, 0, SRCCOPY);
					otherDC.SelectObject(pPrevBmp);
				}

				if (addr == m_nextAssembleAddr) {
					CRect asmRect;
					asmRect.left = CODE_LEFT_MARGIN + (5+3*3) * m_charWidth - 4;
					asmRect.right = asmRect.left + 128;
					asmRect.top = CODE_BAR_HEIGHT + m_lineHeight * l - 2;
					asmRect.bottom = asmRect.top + m_lineHeight + 4;
					m_assemble.MoveWindow(&asmRect);
					m_assemble.SetCode(this, addr);
					m_assemble.SetWindowText(L"");
					m_assemble.SetFocus();
					m_assemble.Invalidate();
					m_nextAssembleAddr = -1;
				}



				pDC->SetTextColor(isRunning ? RGB(0,0,0) : (addr == curr_pc ? RGB(0, 192, 0) : (l==m_cursor ? RGB(128, 128, 0) : RGB(0, 0, 0))));
				pDC->DrawText(wide_str, &textRect, DT_TOP | DT_LEFT | DT_SINGLELINE);

				was_label = false;
				// keep track of which address each line represents for setting the PC
				if (l<MAX_CODE_LINES)
					m_aLinePC[l] = addr;

				addr += instr_len;
			}
		}

		if (m_nextAssembleAddr>=0) {
			m_assemble.ShowWindow(SW_HIDE);
			m_nextAssembleAddr = -1;
		}

		bottomAddr = addr;

		pDC->SelectObject(pPrevFont);

#ifdef DBLBUF
		// Transfer the off-screen DC to the screen
		BitBlt(pPrevDC->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), hdcMem, 0, 0, SRCCOPY);

		// Free-up the off-screen DC
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);
#endif
	}
}


int CCodeView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	GetClientRect(&rect);

	CRect addrRect = rect;
	addrRect.bottom = addrRect.top + CODE_BAR_HEIGHT;
	addrRect.right = addrRect.left + 128;

	if (!m_address.Create(ES_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_WANTRETURN, addrRect, this, ID_MEMORY_ADDRESS_FIELD)) {
		TRACE0("Failed to create address field for code view\n");
		return FALSE; // failed to create
	}
	m_address.SetCode(this);

	CRect asmRect = rect;
	asmRect.bottom = addrRect.top + CODE_BAR_HEIGHT;
	asmRect.left = addrRect.left + 128;

	if (!m_assemble.Create(ES_LEFT | WS_CHILD | WS_BORDER | ES_WANTRETURN, asmRect, this, ID_ASSEMBLE_INPUT)) {
		TRACE0("Failed to create assembler input for code view\n");
		return FALSE; // failed to create
	}
	
	// load the breakpoint image
	if (!m_breakpoint_image.LoadBitmap(IDB_BREAKPOINT)) {
		TRACE0("Failed to create breakpoint bitmap\n");
	}

	if (!m_pc_arrow_image.LoadBitmap(IDB_PC_ARROW)) {
		TRACE0("Failed to create PC marker bitmap\n");
	}

	DragAcceptFiles();

	FocusPC();

	return 0;
}


BOOL CCodeView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta<0) {
		currAddr += InstructionBytes(currAddr);
		GetParent()->Invalidate();
	} else if (zDelta>0) {
		int len = 1;
		uint16_t addr = currAddr-len;
		while (InstructionBytes(addr) > len) {
			--addr;
			++len;
		}
		currAddr = addr;
		GetParent()->Invalidate();
	}
	return CDockablePane::OnMouseWheel(nFlags, zDelta, pt);
}


void CCodeView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	bool wasCursor = m_cursor != 0xffff && m_aLinePC[m_cursor]>=0;
	uint16_t prevAddr = wasCursor ? m_aLinePC[m_cursor] : 0;

	if (nChar == VK_TAB) {
		if ((GetKeyState(VK_SHIFT) & 0x8000) && !IsCPURunning()) {
			if (m_cursor != 0xffff && m_aLinePC[m_cursor]>0) {
				CPUAddUndoRegs(GetRegs());
				GetRegs().PC = m_aLinePC[m_cursor];
			}
		} else
			currAddr = GetRegs().PC;
		Invalidate();
	} else if (nChar == VK_ESCAPE) {
		m_selecting = false;
		if (IsCPURunning()) {
			CPUStop();
		}
		if (m_cursor != 0xffff) {
			m_cursor = 0xffff;
			Invalidate();
		}
	} else if (nChar == VK_UP) {
		if (m_cursor == 0 || m_cursor == 0xffff) {
			int len = 1;
			uint16_t addr = currAddr-len;
			while (InstructionBytes(addr) > len) {
				--addr;
				++len;
			}
			currAddr = addr;
		} else
			m_cursor--;
		GetParent()->Invalidate();
	} else if (nChar == VK_DOWN) {
		if (m_cursor == 0xffff || m_cursor >= (m_numLines-1))
			currAddr += InstructionBytes(currAddr);
		else
			m_cursor++;
		GetParent()->Invalidate();
	} else if (nChar == VK_F11 && !IsCPURunning()) {
		if (GetKeyState(VK_SHIFT) & 0x8000) {
			CPUStepBack();
			if (CMainFrame *pFrame = theApp.GetMainFrame()) {
				pFrame->m_actionID++;
				pFrame->InvalidateRegs();
			}
		} else {
			CPUStep();
			if (CMainFrame *pFrame = theApp.GetMainFrame())
				pFrame->m_actionID++;
		}
		if (GetRegs().PC<currAddr || (currAddr<bottomAddr && GetRegs().PC>=bottomAddr))
			currAddr = GetRegs().PC;
		GetParent()->Invalidate();
	} else if (nChar == VK_F8 && !IsCPURunning()) {
		CPUStepOver();
		if (CMainFrame *pFrame = theApp.GetMainFrame())
			pFrame->m_actionID++;
		if (GetRegs().PC<currAddr || (currAddr<bottomAddr && GetRegs().PC>=bottomAddr))
			currAddr = GetRegs().PC;
		GetParent()->Invalidate();
	} else if (nChar == VK_F9 && m_cursor != 0xffff && m_aLinePC[m_cursor]>0) {
		if (m_cursor<MAX_CODE_LINES) {
			uint32_t id = TogglePCBreakpoint(m_aLinePC[m_cursor]);
			theApp.GetMainFrame()->BreakpointChanged(id);
		}
		Invalidate();
	} else if (nChar == VK_F5) {
		if (GetKeyState(VK_SHIFT) & 0x8000)
			CPUReverse();
		else
			CPUGo();
		Invalidate();
	}

	if (nChar == VK_UP || nChar == VK_DOWN || nChar == VK_LEFT || nChar == VK_RIGHT) {
		if (GetKeyState(VK_SHIFT) & 0x8000) {
			if (m_selecting) {
				if (m_cursor != 0xffff)
					m_selCursor = m_cursor;
			} else if (wasCursor) {
				m_selCursor = m_cursor;
				m_selFirst = prevAddr;
				m_selecting = true;
			}
		} else {
			m_selecting = false;
		}
	}

	CDockablePane::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CCodeView::FocusPC() // make sure PC is in view
{
	Regs &r = GetRegs();
	if (r.PC < currAddr || r.PC >= bottomAddr)
		currAddr = r.PC;
}

void CCodeView::FocusAddr(uint16_t addr) // make sure addr is in window
{
	if (addr < currAddr || addr >= bottomAddr) {
		currAddr = addr;
		Invalidate();
	}
}

void CCodeView::SetBP() // make sure PC is in view
{
	if (m_cursor<MAX_CODE_LINES && m_cursor!=0xffff && m_aLinePC[m_cursor]>0) {
		TogglePCBreakpoint(m_aLinePC[m_cursor]);
		theApp.GetMainFrame()->BreakpointChanged();
	}
	Invalidate();
}

UINT CCodeView::OnGetDlgCode()
{
	return CDockablePane::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTTAB;
}


void CCodeView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_F10) {
		CPUStepOver();
		if (GetRegs().PC<currAddr || (currAddr<bottomAddr && GetRegs().PC>=bottomAddr))
			currAddr = GetRegs().PC;
		GetParent()->Invalidate();
	}

	CDockablePane::OnSysKeyDown(nChar, nRepCnt, nFlags);
}


void CCodeView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_F10) {
		return;
	}

	CDockablePane::OnSysKeyUp(nChar, nRepCnt, nFlags);
}


void CCodeView::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID == SC_KEYMENU) // F10 pressed
		::SendMessage(GetSafeHwnd(), WM_KEYDOWN, VK_F10, NULL);
	else
		CDockablePane::OnSysCommand(nID, lParam);
}

void CCodeView::OnKillFocus(CWnd* pNewWnd)
{
	if (m_selecting) {
		m_selecting = false;
		Invalidate();
	}
	CDockablePane::OnKillFocus(pNewWnd);
}


void CCodeView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (CMainFrame *pFrame = theApp.GetMainFrame())
		pFrame->m_currView = S6_CODE;

	if (point.y > CODE_BAR_HEIGHT && !IsCPURunning()) {

		if (point.x < CODE_LEFT_MARGIN) {
			int line = uint16_t((point.y-CODE_BAR_HEIGHT) / m_lineHeight);
			if (line<MAX_CODE_LINES && m_aLinePC[line]>=0) {
				TogglePCBreakpoint(m_aLinePC[line]);
				theApp.GetMainFrame()->BreakpointChanged();
			}
		} else {
			m_cursor = uint16_t((point.y-CODE_BAR_HEIGHT) / m_lineHeight);
			m_leftmouse = true;
			m_selecting = false;
		}
		Invalidate();
	}

	CDockablePane::OnLButtonDown(nFlags, point);
}

void CCodeView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	m_leftmouse = false;
	CDockablePane::OnLButtonUp(nFlags, point);
}

void CCodeView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (m_leftmouse) {
		if (point.y>=CODE_BAR_HEIGHT) {
			if (!m_selecting) {
				uint16_t line = uint16_t((point.y-CODE_BAR_HEIGHT) / m_lineHeight);
				if (line<MAX_CODE_LINES && m_aLinePC[line]>=0) {
					m_selFirst = m_aLinePC[line];
					m_selecting = true;
					m_selCursor = line;
					Invalidate();
				}
			} else {
				int line = uint16_t((point.y-CODE_BAR_HEIGHT) / m_lineHeight);
				if (line != m_selCursor) {
					m_selCursor = line;
					Invalidate();
				}
			}
		}
		return;
	}
	CDockablePane::OnMouseMove(nFlags, point);
}


void CCodeView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (point.y > CODE_BAR_HEIGHT && !IsCPURunning()) {
		if (point.x >= CODE_LEFT_MARGIN) {
			int line = uint16_t((point.y-CODE_BAR_HEIGHT) / m_lineHeight);
			if (line < MAX_CODE_LINES && m_aLinePC[line]>=0) {
				CRect asmRect;
				asmRect.left = CODE_LEFT_MARGIN + (5+3*3) * m_charWidth - 4;
				asmRect.right = asmRect.left + 128;
				asmRect.top = CODE_BAR_HEIGHT + m_lineHeight * line - 2;
				asmRect.bottom = asmRect.top + m_lineHeight + 4;
				m_assemble.MoveWindow(&asmRect);
				m_assemble.SetCode(this, m_aLinePC[line]);
				m_assemble.ShowWindow(SW_SHOW);
				m_assemble.SetFocus();
				m_assemble.Invalidate();
			}
		}
	}
}

void CCodeView::OnAssembled(uint16_t nextAddr)
{
	m_nextAssembleAddr = nextAddr;
	if (bottomAddr>=nextAddr)
		currAddr += InstructionBytes(currAddr);
	Invalidate();
}


void CCodeView::OnButtonStop()
{
	CPUStop();
	Invalidate();
}


void CCodeView::OnButtonGo()
{
	if (!IsCPURunning())
		CPUGo();
}


void CCodeView::OnButtonReverse()
{
	CPUStepBack();
	Invalidate();
}

void CCodeView::OnButtonStep()
{
	CPUStop();
	Invalidate();
}

void CCodeView::OnButtonStepBack()
{
	CPUStepBack();
	Invalidate();
}

void CCodeView::OnButtonBreak()
{
	if (m_cursor != 0xffff && m_aLinePC[m_cursor]>=0) {
		TogglePCBreakpoint(m_aLinePC[m_cursor]);
		theApp.GetMainFrame()->BreakpointChanged();
		Invalidate();
	}
}

void CCodeView::OnDropFiles(HDROP hDropInfo)
{
	CString sFile;
	DWORD   nBuffer = 0;

	// Get the number of files dropped 
	int nFilesDropped = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	if (nFilesDropped) {
		wchar_t name[MAX_PATH];
		// Get path and name of the file 
		DragQueryFile(hDropInfo, 0, name, sizeof(name)/sizeof(name[0]));
		if (theApp.GetMainFrame())
			theApp.GetMainFrame()->TryLoadBinary(name);
	}
	CDockablePane::OnDropFiles(hDropInfo);
}


void CCodeView::OnEditCopy()
{
	if (m_selecting) {
		int l = m_selCursor;
		int selAddr = m_aLinePC[l];

		while (selAddr<0 && l<m_numLines)
			selAddr = m_aLinePC[--l];
		if (l==m_numLines && selAddr<0)
			selAddr = bottomAddr;

		uint16_t a0 = (uint16_t)selAddr>m_selFirst ? m_selFirst : (uint16_t)selAddr;
		uint16_t a1 = (uint16_t)selAddr>m_selFirst ? (uint16_t)selAddr : m_selFirst;

		int chars = (a1-a0) * 64;
		if (char *tmp = (char*)malloc(chars)) {
			int len = 0;
			while (a0<=a1) {
				int skp = 0;
				char instr[64];
				if (const wchar_t *label = GetSymbol(a0)) {
					size_t newlen = 0;
					char label8[256];
					wcstombs_s(&newlen, label8, label, sizeof(label8)-1);
					len += sprintf_s(tmp+len, chars-len, "             %s:\r\n", label8);
				}
				int bts = Disassemble(a0, instr, sizeof(instr), skp);
				len += sprintf_s(tmp+len, chars-len, "%04x %s\r\n", a0, instr);
				a0 += bts;
			}
			tmp[len] = 0;
			if (OpenClipboard()) {
				if (EmptyClipboard()) {
					HGLOBAL clipbuffer = GlobalAlloc(GMEM_DDESHARE, len+1);
					char* buffer = (char*)GlobalLock(clipbuffer);
					strcpy_s(buffer, len+1, tmp);
					GlobalUnlock(clipbuffer);
					SetClipboardData(CF_TEXT, clipbuffer);
				}
				CloseClipboard();
			}
			free(tmp);
		}
	}
}





