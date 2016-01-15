// RegisterPane.cpp : implementation file
//

#include "stdafx.h"
#include "Step6502.h"
#include "RegisterPane.h"
#include "machine.h"


// CRegisterPane

IMPLEMENT_DYNAMIC(CRegisterPane, CDockablePane)

CRegisterPane::CRegisterPane()
{
	m_cursor_x = 0xff;
}

CRegisterPane::~CRegisterPane()
{
}


BEGIN_MESSAGE_MAP(CRegisterPane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_KEYDOWN()
	ON_WM_KILLFOCUS()
	ON_WM_LBUTTONDOWN()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()



// CRegisterPane message handlers




int CRegisterPane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}


void CRegisterPane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
}

#define DBLBUF

void CRegisterPane::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CDockablePane::OnPaint() for painting messages

	CRect rect;
	GetClientRect(&rect);
	CDC *pDC = &dc;

#ifdef DBLBUF
	CDC *pPrevDC = pDC;
	HDC hdcMem = CreateCompatibleDC(pDC->GetSafeHdc());
	HBITMAP hbmMem = CreateCompatibleBitmap(pDC->GetSafeHdc(), rect.Width(), rect.Height());
	HANDLE hOld = SelectObject(hdcMem, hbmMem);

	//	HDC hDC = CreateCompatibleDC(pDC->GetSafeHdc());
	pDC = CDC::FromHandle(hdcMem);
#endif

	if (pDC) {
//		pDC->SetBkColor(RGB(128, 128, 128));

#ifdef DBLBUF
		// Create an off-screen DC for double-buffering
		pDC->FillSolidRect(rect, pDC->GetBkColor());
#endif
//		CBrush frameBrush;
		
		
		CFont *pPrevFont = pDC->SelectObject(&theApp.GetMainFrame()->Font());

		CRect textRect = rect;

		TEXTMETRIC metrics;
		pDC->GetTextMetrics(&metrics);

		int lineHeight = metrics.tmHeight + metrics.tmExternalLeading;
		int charWidth = metrics.tmAveCharWidth;

		m_lineHeight = lineHeight;
		m_charWidth = charWidth;

		if (m_cursor_x != 0xff) {
			COLORREF prevCol = pDC->GetBkColor();
			int left = charWidth * m_cursor_x;
			int top = lineHeight ;
			CRect cursor_rect(left, top, left + charWidth, top + lineHeight - 2);
			pDC->FillSolidRect(cursor_rect, RGB(255, 128, 128));
			pDC->SetBkColor(prevCol);
			pDC->SetBkMode(TRANSPARENT);
		}

		pDC->DrawText(L"ADDR AC XR YR SP NV-BDIZC CY STOPWATCH", &textRect, DT_TOP | DT_LEFT | DT_SINGLELINE);
		textRect.top += lineHeight;

		Regs r = GetRegs();
		wchar_t reg_str[512];
		size_t o = swprintf(reg_str, sizeof(reg_str)/sizeof(reg_str[0]), L"%04x %02x %02x %02x %02x ", r.PC, r.A, r.X, r.Y, r.S);
		for (int b = 128; b; b >>= 1)
			reg_str[o++] = (r.P&b) ? (wchar_t)'1' : (wchar_t)'0';
		swprintf(reg_str+o, sizeof(reg_str)/sizeof(reg_str[0])-o, L" %02x %9d", r.T, GetCycles());

		pDC->DrawText(reg_str, &textRect, DT_TOP | DT_LEFT | DT_SINGLELINE);

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


void CRegisterPane::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CDockablePane::OnLButtonDblClk(nFlags, point);
}


void CRegisterPane::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	UINT uc = toupper(nChar);
	if (nChar == VK_LEFT) {
		if (m_cursor_x != 0xff && m_cursor_x !=0x00) {
			m_cursor_x--;
			Invalidate();
		}
	} else if (nChar == VK_RIGHT) {
		if (m_cursor_x != 0xff && m_cursor_x < 24) {
			m_cursor_x++;
			Invalidate();
		}
	} else if (m_cursor_x != 0xff && ((uc>='0' && uc<='9') || (uc>='A' && uc<='F'))) {
		// ADDR AC XR YR SP NV-BDIZC
		// 01234567890123456789901234
		Regs &r = GetRegs();
		uint16_t n = uc<='9' ? (uc-'0') : (uc-'A'+10);
		if (m_cursor_x<4) {
			uint16_t s = (3-m_cursor_x)<<2;
			r.PC = (r.PC&(~(0xf<<s))) | (n<<s);
			m_cursor_x++;
			if (m_cursor_x==4)
				m_cursor_x++;
			Invalidate();
		} else if (m_cursor_x<17) {
			uint16_t i = (m_cursor_x-4) / 3;
			uint16_t s = (m_cursor_x-4) % 3;
			if (s) {
				s--;
				s <<= 2;
				n <<= 4-s;
				switch (i) {
					case 0: r.A = (r.A & (0xf<<s)) | n; break;
					case 1: r.X = (r.X & (0xf<<s)) | n; break;
					case 2: r.Y = (r.Y & (0xf<<s)) | n; break;
					case 3: r.S = (r.S & (0xf<<s)) | n; break;
				}
				m_cursor_x++;
				if (s==4)
					m_cursor_x++;
				Invalidate();
			}
		} else if (n==0 || n==1) {
			uint8_t bit = 24-m_cursor_x;
			r.P &= ~(1<<bit);
			r.P |= n<<bit;
			if (m_cursor_x<24)
				m_cursor_x++;
			Invalidate();
		}
	}
	CDockablePane::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CRegisterPane::OnKillFocus(CWnd* pNewWnd)
{
	CDockablePane::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	if (m_cursor_x != 0xff) {
		m_cursor_x = 0xff;
		Invalidate();
	}
}


void CRegisterPane::OnLButtonDown(UINT nFlags, CPoint point)
{
	// ADDR AC XR YR SP NV-BDIZC
	// 01234567890123456789901234
	// TODO: Add your message handler code here and/or call default
	if (point.y >= (m_lineHeight) && point.y <= (2*m_lineHeight)) {
		if (point.x < (24 * m_charWidth)) {
			m_cursor_x = uint8_t(point.x / m_charWidth);
			Invalidate();
		}
	}
	CDockablePane::OnLButtonDown(nFlags, point);
}


UINT CRegisterPane::OnGetDlgCode()
{
	// TODO: Add your message handler code here and/or call default

	return CDockablePane::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS | DLGC_WANTTAB;
}
