// Memory.cpp : implementation file
//

#include "stdafx.h"
#include "Step6502.h"
#include "Memory.h"
#include "Sym.h"
#include "machine.h"


// Memory.cpp : implementation file
//

// CMemoryColumns

IMPLEMENT_DYNAMIC(CMemoryColumns, CEdit)

CMemoryColumns::CMemoryColumns() : m_memory(nullptr)
{

}

CMemoryColumns::~CMemoryColumns()
{
}


BEGIN_MESSAGE_MAP(CMemoryColumns, CEdit)
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



// CMemoryColumns message handlers


void CMemoryColumns::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_RETURN && m_memory) {
		wchar_t address[64], *end;
		GetWindowText(address, sizeof(address)/sizeof(address[0]));
		m_memory->bytesWide = (uint16_t)wcstol(address, &end, 10);
		GetParent()->Invalidate();
		return;
	}

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

UINT CMemoryColumns::OnGetDlgCode()
{
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS;
}


// CMemoryColumns message handlers

// CMemoryAddress

IMPLEMENT_DYNAMIC(CMemoryAddress, CEdit)

CMemoryAddress::CMemoryAddress() : m_memory(nullptr)
{

}

CMemoryAddress::~CMemoryAddress()
{
}


BEGIN_MESSAGE_MAP(CMemoryAddress, CEdit)
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



// CMemoryAddress message handlers




void CMemoryAddress::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar==VK_RETURN && m_memory) {
		wchar_t address[64], *end;
		GetWindowText(address, sizeof(address)/sizeof(address[0]));
		uint16_t addr;
		if (GetAddress(address, wcslen(address), addr))
			m_memory->currAddr = addr;
		else
			m_memory->currAddr = (uint16_t)wcstol(address, &end, 16);
		GetParent()->Invalidate();
		return;
	}
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}


UINT CMemoryAddress::OnGetDlgCode()
{
	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS;
}

// CMemory

IMPLEMENT_DYNAMIC(CMemory, CDockablePane)


CMemory::CMemory()
{
	currAddr = 0xe000;
	prevAddr = 0xe000;
	fitBytesWidth = 0;
	bytesWide = 0;
	selectAddr = false;
	cursor_x = cursor_y = 0xff;
	m_selecting = false;
}

CMemory::~CMemory()
{
	if (m_prevDelta.m_mirror != nullptr)
		free(m_prevDelta.m_mirror);

	if (m_currDelta.m_mirror != nullptr)
		free(m_currDelta.m_mirror);
}


BEGIN_MESSAGE_MAP(CMemory, CDockablePane)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONUP()
	ON_WM_GETDLGCODE()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()



// CMemory message handlers

#define MEMORY_BAR_HEIGHT 32

int CMemory::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	GetClientRect(&rect);

	CRect addrRect = rect;
	addrRect.bottom = addrRect.top + MEMORY_BAR_HEIGHT;
	addrRect.right = addrRect.left + 128;

	if (!m_address.Create(ES_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_WANTRETURN, addrRect, this, ID_MEMORY_ADDRESS_FIELD)) {
		TRACE0("Failed to create address field\n");
		return FALSE; // failed to create
	}
	if (CWnd *pAddrItem = GetDlgItem(ID_MEMORY_ADDRESS_FIELD)) {
		wchar_t wide_addr[32];
		wsprintf(wide_addr, L"%04x", currAddr);
		pAddrItem->SetWindowText(wide_addr);
	}
	m_address.SetMemory(this);

	addrRect.left = addrRect.right;
	addrRect.right = addrRect.left + 128;

	if (!m_columns.Create(ES_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_WANTRETURN, addrRect, this, ID_MEMORY_COLUMNS_FIELD)) {
		TRACE0("Failed to create column field\n");
		return FALSE; // failed to create
	}
	if (CWnd *pAddrItem = GetDlgItem(ID_MEMORY_COLUMNS_FIELD)) {
		pAddrItem->SetWindowText(L"0000");
	}
	m_columns.SetMemory(this);

	CRect colRect = rect;
	colRect.left += 128;

	return 0;
}

#define DBLBUF
void CMemory::OnPaint()
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

	//	HDC hDC = CreateCompatibleDC(pDC->GetSafeHdc());
	pDC = CDC::FromHandle(hdcMem);
#endif
	if (pDC) {
#ifdef DBLBUF
		// Create an off-screen DC for double-buffering
		pDC->FillSolidRect(rect, pDC->GetBkColor());
#endif
		CFont *pPrevFont = pDC->SelectObject(&theApp.GetMainFrame()->Font());

		TEXTMETRIC metrics;
		pDC->GetTextMetrics(&metrics);

		int width = rect.Width();
		int height = rect.Height();
		int lineHeight = metrics.tmHeight + metrics.tmExternalLeading;
		int charWidth = metrics.tmAveCharWidth;

		int lines = (height-MEMORY_BAR_HEIGHT) / lineHeight;
		int columns = bytesWide == 0 ? (width-charWidth*5) / (charWidth * 3) : bytesWide;
		fitBytesWidth = columns;
		fitLinesHeight = lines;
		m_charWidth = charWidth;
		m_lineHeight = lineHeight;
		uint8_t v = uint8_t(rand());
		wchar_t byte_str[16];

		uint8_t *bytes = Get6502Mem(0);
		uint16_t addr = currAddr;

		m_editLeft = charWidth * 5;
		m_editRight = (columns * 3 + 4) * charWidth;

		if (selectAddr && selectPoint.y > MEMORY_BAR_HEIGHT && selectPoint.x>m_editLeft && selectPoint.x<m_editRight) {
			cursor_x = uint8_t((selectPoint.x-m_editLeft) / charWidth);
			cursor_y = uint8_t((selectPoint.y-MEMORY_BAR_HEIGHT) / lineHeight);
		}
		selectAddr = false;


		if (CMainFrame *pFrame = theApp.GetMainFrame()) {
			if (m_currDelta.m_actionId != pFrame->m_actionID) {
				uint16_t count = lines * columns;
				struct SMemDelta t = m_currDelta;
				m_currDelta = m_prevDelta;
				m_prevDelta = t;
				if (m_currDelta.m_size < count) {
					if (m_currDelta.m_mirror)
						free(m_currDelta.m_mirror);
					m_currDelta.m_size = 2*count;
					m_currDelta.m_mirror = (uint8_t*)malloc(m_currDelta.m_size);
				}
				m_currDelta.m_actionId = pFrame->m_actionID;
				m_currDelta.m_addr = currAddr;
				m_currDelta.m_end = addr + count;
				uint32_t end = (uint32_t)addr + count;
				if (end > 0x10000) {
					memcpy(m_currDelta.m_mirror + 0x10000-addr, Get6502Mem(0), end & 0xffff);
					end = 0x10000;
				}
				memcpy(m_currDelta.m_mirror, Get6502Mem(addr), end-addr);
			}
		}

		bool wasChanged = false;
		pDC->SetBkMode(TRANSPARENT);
		for (int l = 0; l<lines; l++) {
			CRect textRect = rect;
			textRect.top += l * lineHeight + MEMORY_BAR_HEIGHT;
			swprintf(byte_str, sizeof(byte_str), L"%04x", addr);
			pDC->DrawText(byte_str, &textRect, DT_TOP | DT_LEFT | DT_SINGLELINE);
			uint16_t addrEnd = addr + columns;

			if (m_selecting && m_selAddr<addrEnd && m_selEnd>=addr) {
				CRect selRect;
				selRect.left = ((m_selAddr<addr ? 0 : (m_selAddr-addr)) * 3 + 5) * charWidth;
				selRect.top = l * lineHeight + MEMORY_BAR_HEIGHT;
				selRect.right = ((m_selEnd>=addrEnd ? columns : (m_selEnd+1-addr)) * 3 + 4) * charWidth;
				selRect.bottom = selRect.top + lineHeight;
				pDC->FillSolidRect(&selRect, RGB(224, 224, 255));
			}
			if (cursor_x != 0xff && cursor_y == l) {
				COLORREF prevCol = pDC->GetBkColor();
				int left = charWidth * (5 + cursor_x);
				int top = MEMORY_BAR_HEIGHT + lineHeight * cursor_y;
				CRect cursor_rect(left, top, left + charWidth, top + lineHeight - 2);
				pDC->FillSolidRect(cursor_rect, RGB(255, 128, 128));
				pDC->SetBkColor(prevCol);
			}


			for (int c = 0; c<columns; c++) {
				textRect = rect;
				textRect.left += (c * 3 + 5) * charWidth;
				textRect.top += l * lineHeight + MEMORY_BAR_HEIGHT;

				v = bytes[addr];
				bool isChanged = false;
				if (m_prevDelta.m_end != m_prevDelta.m_addr) {
					if (m_prevDelta.m_end > m_prevDelta.m_addr || addr>=m_prevDelta.m_addr)
						isChanged = addr>=m_prevDelta.m_addr && addr<m_prevDelta.m_end ? m_prevDelta.m_mirror[addr-m_prevDelta.m_addr] != v : false;
					else if (addr<m_prevDelta.m_end)
						isChanged = m_prevDelta.m_mirror[0x1000-m_prevDelta.m_addr + addr] != v;
				}
				addr++;
				swprintf(byte_str, sizeof(byte_str), L"%02x", v);

				if (isChanged != wasChanged)
					pDC->SetTextColor(isChanged ? RGB(192, 0, 0) : RGB(0, 0, 0));
				pDC->DrawText(byte_str, &textRect, DT_TOP | DT_LEFT | DT_SINGLELINE);
				wasChanged = isChanged;
			}
		}
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

void CMemory::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	CRect rect;
	GetClientRect(&rect);
	rect.left += 128;
	rect.bottom = rect.top + MEMORY_BAR_HEIGHT;

//	m_columns.SetWindowPos(m_address.GetWindow(GW_OWNER), rect.left, rect.top, rect.Width(), MEMORY_BAR_HEIGHT, SWP_NOZORDER | SWP_NOMOVE);
	Invalidate();
}




void CMemory::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	UINT upchar = toupper(nChar);
	uint16_t prev_addr = currAddr + cursor_y * fitBytesWidth + cursor_x/3;

	if (nChar == VK_UP) {
		if (cursor_x != 0xff && cursor_y) {
			if (cursor_y>0)
				cursor_y--;
			else
				currAddr -= fitBytesWidth;
		} else
			currAddr -= fitBytesWidth;
		Invalidate();
	} else if (nChar == VK_DOWN) {
		if (cursor_x != 0xff) {
			cursor_y++;
			if (cursor_y>=fitLinesHeight) {
				cursor_y = fitLinesHeight-1;
				currAddr += fitBytesWidth;
			}
		} else
			currAddr += fitBytesWidth;
		Invalidate();
	} else if (nChar == VK_LEFT) {
		if (cursor_x != 0xff) {
			if (cursor_x==0) {
				cursor_x = 3 * fitBytesWidth-2;
				if (cursor_y>0)
					cursor_y--;
				else
					currAddr -= fitBytesWidth;
			} else
				cursor_x--;
			Invalidate();
		}
	} else if (nChar == VK_RIGHT) {
		if (cursor_x != 0xff) {
			cursor_x++;
			if (cursor_x >= (3*fitBytesWidth)) {
				cursor_x = 0;
				cursor_y++;
				if (cursor_y>=fitLinesHeight) {
					cursor_y = fitLinesHeight-1;
					currAddr += fitBytesWidth;
				}
			}
			Invalidate();
		}
	} else if (nChar == VK_PRIOR) {
		currAddr -= 8 * fitBytesWidth;
		Invalidate();
	} else if (nChar == VK_NEXT) {
		currAddr += 8 * fitBytesWidth;
		Invalidate();
	} else if (nChar == VK_ESCAPE) {
		cursor_x = cursor_y = 0xff;
		m_selecting = false;
		Invalidate();
	} else if (cursor_x != 0xff && ((nChar >= '0' && nChar <= '9') || (upchar >= 'A' && upchar <='F'))) {
		if ((cursor_x%3)!=2) {
			uint16_t addr = currAddr + cursor_y * fitBytesWidth + cursor_x/3;
			uint8_t shift = (cursor_x%3)==1 ? 0 : 4;
			uint8_t v = (upchar > '9' ? (upchar+10-'A') : (upchar-'0'));
			uint8_t o = Get6502Byte(addr);
			uint8_t w = (o & (0xf<<(4-shift))) | (v<<shift);
			Set6502Byte(addr, w);
			if ((cursor_x%3)==1)
				cursor_x++;
			cursor_x++;
			if (cursor_x >= (3*fitBytesWidth)) {
				cursor_x = 0;
				cursor_y++;
			}
			Invalidate();
		}
	}

	if (nChar == VK_LEFT || nChar == VK_UP || nChar == VK_RIGHT || nChar == VK_DOWN) {
		if (GetKeyState(VK_SHIFT) & 0x8000) {
			uint16_t new_addr = currAddr + cursor_y * fitBytesWidth + cursor_x/3;
			if (!m_selecting) {
				m_selAddr = prev_addr;
				m_selEnd = new_addr;
				m_selFirst = prev_addr;
			} else {
				if (new_addr <= m_selFirst) {
					m_selAddr = new_addr;
					m_selEnd = m_selFirst;
					Invalidate();
				} else if (new_addr > m_selFirst) {
					m_selAddr = m_selFirst;
					m_selEnd = new_addr;
					Invalidate();
				}
			}
			m_selecting = true;
		} else {
			m_selecting = false;
			Invalidate();
		}
	}

	CDockablePane::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMemory::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_mouse_left_down) {
		if (!m_selecting) {
			int x = (selectPoint.x-m_editLeft) / m_charWidth;
			int y = (selectPoint.y-MEMORY_BAR_HEIGHT) / m_lineHeight;
			if (y>=0 && x>=0) {
				uint16_t addr = currAddr + x/3 + y * fitBytesWidth;
				m_selFirst = addr;
				m_selAddr = addr;
				m_selEnd = addr;
				m_selecting = true;
				Invalidate();
			}
		} else {
			int x = (point.x-m_editLeft) / m_charWidth;
			int y = (point.y-MEMORY_BAR_HEIGHT) / m_lineHeight;
			uint16_t addr = currAddr + x/3 + y * fitBytesWidth;
			if (addr >= m_selFirst) {
				if (m_selAddr != m_selFirst || m_selEnd != addr) {
					m_selAddr = m_selFirst;
					m_selEnd = addr;
					Invalidate();
				}
			} else if (m_selAddr != addr || m_selEnd != m_selFirst) {
				m_selAddr = addr;
				m_selEnd = m_selFirst;
				Invalidate();
			}
			wchar_t buf[32];
			wsprintf(buf, L"%x", addr);

		}
	}
}


void CMemory::OnLButtonDblClk(UINT nFlags, CPoint point)
{
}


void CMemory::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (CMainFrame *pFrame = theApp.GetMainFrame())
		pFrame->m_currView = m_viewIndex ? S6_MEM2 : S6_MEM1;

	m_mouse_left_down = true;
	selectPoint = point;
	if (m_selecting) {
		m_selecting = false;
		Invalidate();
	}

	CDockablePane::OnLButtonDown(nFlags, point);
}

BOOL CMemory::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta > 0) {
		currAddr -= fitBytesWidth;
		Invalidate();
	} else if (zDelta < 0) {
		currAddr += fitBytesWidth;
		Invalidate();
	}

	return CDockablePane::OnMouseWheel(nFlags, zDelta, pt);
}


void CMemory::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_mouse_left_down = false;
	if (IsCPURunning()) {
		Invalidate();
	} else if (point.y > MEMORY_BAR_HEIGHT && point.x>m_editLeft && point.x<m_editRight) {
		selectPoint = point;
		selectAddr = true;
		Invalidate();
	}


	CDockablePane::OnLButtonUp(nFlags, point);
}


UINT CMemory::OnGetDlgCode()
{
	return CDockablePane::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS;
}


void CMemory::OnKillFocus(CWnd* pNewWnd)
{
	CDockablePane::OnKillFocus(pNewWnd);
	bool invalidate = false;
	if (cursor_x != 0xff) {
		cursor_x = cursor_y = 0xff;
		invalidate = true;
	}
	if (m_selecting) {
		m_selecting = false;
		invalidate = true;
	}
	if (invalidate)
		Invalidate();
}

void CMemory::OnEditCopy()
{
	if (m_selecting) {
		uint16_t n = (m_selAddr - currAddr) % fitBytesWidth;
		uint16_t b = m_selEnd - m_selAddr + 1;
		int chars = 3 * b + 2 * (b / fitBytesWidth + 2);
		if (char *tmp = (char*)malloc(chars)) {
			int len = 0;
			for (int i = 0; i<b; i++) {
				if (++n>=fitBytesWidth)
					n = 0;
				len += sprintf_s(tmp+len, chars-len, "%02x%s",
								 Get6502Byte(currAddr+i), n ? " " : "\r\n");
			}
			len += sprintf_s(tmp+len, chars-len, "%s", "\r\n");
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


