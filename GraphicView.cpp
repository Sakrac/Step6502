// GraphicView.cpp : implementation file
//

#include "stdafx.h"
#include "Step6502.h"
#include "GraphicView.h"
#include "machine.h"


// CGraphicField

IMPLEMENT_DYNAMIC(CGraphicField, CEdit)

CGraphicField::CGraphicField() : m_view(nullptr)
{

}

CGraphicField::~CGraphicField()
{
}


BEGIN_MESSAGE_MAP(CGraphicField, CEdit)
ON_WM_KEYDOWN()
ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

void CGraphicField::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	if (nChar==VK_RETURN && m_view) {
		wchar_t address[64], *end;
		GetWindowText(address, sizeof(address)/sizeof(address[0]));
		m_view->SetField(wcstol(address, &end, (m_type==ADDRESS || m_type==FONT) ? 16 : 10), m_type);
		m_view->Invalidate();
	}
	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

UINT CGraphicField::OnGetDlgCode()
{
	// TODO: Add your message handler code here and/or call default

	return CEdit::OnGetDlgCode() | DLGC_WANTALLKEYS | DLGC_WANTARROWS;
}

// CGraphicStyle

IMPLEMENT_DYNAMIC(CGraphicStyle, CComboBox)

CGraphicStyle::CGraphicStyle() : m_view(nullptr)
{

}

CGraphicStyle::~CGraphicStyle()
{
}


BEGIN_MESSAGE_MAP(CGraphicStyle, CComboBox)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, &CGraphicStyle::OnCbnSelchange)
END_MESSAGE_MAP()



// CGraphicStyle message handlers




void CGraphicStyle::OnCbnSelchange()
{
	// TODO: Add your control notification handler code here
	if (m_view) {
		m_view->SetLayout((CGraphicView::GraphicLayout)GetCurSel());
		m_view->Invalidate();
	}
}


// CGraphicField message handlers


// CGraphicView

IMPLEMENT_DYNAMIC(CGraphicView, CDockablePane)

CGraphicView::CGraphicView()
{
	m_bytesWide = 40;
	m_linesHigh = 200;
	m_layout = GL_TEXTMODE;
	m_address = 0x0400;
	m_fontAddress = 0xf000;

	m_currBuffer = nullptr;
	m_currBufSize = 0;
	m_zoom = 0;
	m_x = 0;
	m_y = 0;
	m_drag = 0;
}

CGraphicView::~CGraphicView()
{
	if (m_currBuffer != nullptr) {
		free(m_currBuffer);
	}
}


BEGIN_MESSAGE_MAP(CGraphicView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


// CGraphicView message handlers


int CGraphicView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	CRect addrRect(0, 0, 128, 24);
	if (!m_editAddress.Create(ES_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_WANTRETURN, addrRect, this, ID_GRAPHIC_ADDRESS_FIELD)) {
		TRACE0("Failed to create address field\n");
		return FALSE; // failed to create
	}
	if (CWnd *pAddrItem = GetDlgItem(ID_GRAPHIC_ADDRESS_FIELD))
		pAddrItem->SetWindowText(L"400");

	CRect widthRect(180, 0, 210, 24);
	if (!m_editWidth.Create(ES_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_WANTRETURN, widthRect, this, ID_GRAPHIC_WIDTH_FIELD)) {
		TRACE0("Failed to create address field\n");
		return FALSE; // failed to create
	}
	if (CWnd *pAddrItem = GetDlgItem(ID_GRAPHIC_WIDTH_FIELD))
		pAddrItem->SetWindowText(L"40");

	CRect heightRect(225, 0, 265, 24);
	if (!m_editHeight.Create(ES_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_WANTRETURN, heightRect, this, ID_GRAPHIC_HEIGHT_FIELD)) {
		TRACE0("Failed to create address field\n");
		return FALSE; // failed to create
	}
	if (CWnd *pAddrItem = GetDlgItem(ID_GRAPHIC_HEIGHT_FIELD))
		pAddrItem->SetWindowText(L"200");
	CRect colRect(280, 0, 380, 128);
	if (!m_columns.Create(CBS_DROPDOWNLIST | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL, colRect, this, ID_GRAPHIC_STYLE_FIELD)) {
		TRACE0("Failed to create column field\n");
		return FALSE; // failed to create
	}
	CRect fontRect(390, 0, 518, 24);
	if (!m_editFontAddr.Create(ES_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER | ES_WANTRETURN, fontRect, this, ID_GRAPHIC_FONT_FIELD)) {
		TRACE0("Failed to create font address field\n");
		return FALSE; // failed to create
	}
	if (CWnd *pAddrItem = GetDlgItem(ID_GRAPHIC_FONT_FIELD))
		pAddrItem->SetWindowText(L"f000");
	m_columns.AddString(L"Lines");
	m_columns.AddString(L"Columns");
	m_columns.AddString(L"8X8,");
	m_columns.AddString(L"Sprites");
	m_columns.AddString(L"Text");
	m_columns.SetCurSel(GL_TEXTMODE);
	m_columns.SetGraphicView(this);
	m_editAddress.SetGraphicView(this, CGraphicField::FieldType::ADDRESS);
	m_editWidth.SetGraphicView(this, CGraphicField::FieldType::WIDTH);
	m_editHeight.SetGraphicView(this, CGraphicField::FieldType::HEIGHT);
	m_editFontAddr.SetGraphicView(this, CGraphicField::FieldType::FONT);

	return 0;
}

HBITMAP CGraphicView::Create8bppBitmap(HDC hdc)
{
	BITMAPINFO *bmi = (BITMAPINFO *)malloc(sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256);
	BITMAPINFOHEADER &bih(bmi->bmiHeader);
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = m_bytesWide * 8;
	bih.biHeight = -((m_linesHigh+7) & (~7L));
	bih.biPlanes = 1;
	bih.biBitCount = 8;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = 0;
	bih.biXPelsPerMeter = 14173;
	bih.biYPelsPerMeter = 14173;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;
	for (int I = 0; I <= 255; I++) {
		bmi->bmiColors[I].rgbBlue = bmi->bmiColors[I].rgbGreen = bmi->bmiColors[I].rgbRed = (BYTE)I;
		bmi->bmiColors[I].rgbReserved = 0;
	}
	bmi->bmiColors[1].rgbBlue = 170;
	bmi->bmiColors[1].rgbGreen = 32;
	bmi->bmiColors[1].rgbRed = 32;
	bmi->bmiColors[2].rgbBlue = 255;
	bmi->bmiColors[2].rgbGreen = 136;
	bmi->bmiColors[2].rgbRed = 64;

	void *Pixels = NULL;
	HBITMAP hbmp = CreateDIBSection(hdc, bmi, DIB_RGB_COLORS, &Pixels, NULL, 0);
	uint8_t *d = (uint8_t*)Pixels;
	uint32_t w = m_bytesWide * 8;

	switch (m_layout) {
		case GL_BITPLANE: {
			uint16_t a = m_address;
			for (int y = 0; y<m_linesHigh; y++) {
				uint16_t xp = 0;
				for (int x = 0; x<m_bytesWide; x++) {
					uint8_t b = Get6502Byte(a++);
					uint8_t m = 0x80;
					for (int bit = 0; bit<8; bit++) {
						d[(y)*w + (xp++)] = (b&m) ? 2 : 1;
						m >>= 1;
					}
				}
			}
			break;
		}
		case GL_COLUMNS: {
			uint16_t a = m_address;
			for (int x = 0; x<m_bytesWide; x++) {
				for (int y = 0; y<m_linesHigh; y++) {
					int xp = x*8;
					uint8_t b = Get6502Byte(a++);
					uint8_t m = 0x80;
					for (int bit = 0; bit<8; bit++) {
						d[(y)*w + (xp++)] = (b&m) ? 2 : 1;
						m >>= 1;
					}
				}
			}
			break;
		}
		case GL_8X8: {
			uint16_t a = m_address;
			for (int y = 0; y<(m_linesHigh>>3); y++) {
				for (int x = 0; x<m_bytesWide; x++) {
					for (int h = 0; h<8; h++) {
						uint8_t b = Get6502Byte(a++);
						uint8_t m = 0x80;
						for (int bit = 0; bit<8; bit++) {
							d[(y*8+h)*w + (x*8+bit)] = (b&m) ? 2 : 1;
							m >>= 1;
						}
					}
				}
			}
			break;
		}
		case GL_TEXTMODE: {
			uint16_t a = m_address;
			for (int y = 0; y<(m_linesHigh>>3); y++) {
				for (int x = 0; x<m_bytesWide; x++) {
					uint8_t chr = Get6502Byte(a++);
					uint16_t cs = m_fontAddress + 8*chr;
					for (int h = 0; h<8; h++) {
						uint8_t b = Get6502Byte(cs++);
						uint8_t m = 0x80;
						for (int bit = 0; bit<8; bit++) {
							d[(y*8+h)*w + (x*8+bit)] = (b&m) ? 2 : 1;
							m >>= 1;
						}
					}
				}
			}
			break;
		}
		case GL_SPRITES: {
			uint16_t a = m_address;
			int sx = m_bytesWide/3;
			int sy = m_linesHigh/21;
			for (int y = 0; y<sy; y++) {
				for (int x = 0; x<sx; x++) {
					for (int l = 0; l<21; l++) {
						uint8_t *ds = d + (y*21+l)*w + x*24;
						for (int s = 0; s<3; s++) {
							uint8_t b = Get6502Byte(a++);
							uint8_t m = 0x80;
							for (int bit = 0; bit<8; bit++) {
								*ds++ = b&m ? 2 : 1;
								m >>= 1;
							}
						}
					}
					++a;
				}
			}
			break;
		}
	}

	free(bmi);

	return hbmp;
}

#define DBLBUF

#define EDIT_BAR_HEIGHT 32

void CGraphicView::OnPaint()
{
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
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
		// Create an off-screen DC for double-buffering
		pDC->FillSolidRect(rect, pDC->GetBkColor());
#endif

		CFont *pPrevFont = pDC->SelectObject(&theApp.GetMainFrame()->Font());

		CRect addrRect(128, 0, 180, EDIT_BAR_HEIGHT);
		pDC->DrawText(L"W/H:", &addrRect, DT_TOP | DT_RIGHT | DT_SINGLELINE);

		CRect widthRect(210, 0, 225, EDIT_BAR_HEIGHT);
		pDC->DrawText(L"X", &widthRect, DT_TOP | DT_CENTER | DT_SINGLELINE);
		

		CDC otherDC;
		otherDC.CreateCompatibleDC(pDC);
		HBITMAP currBitmap = Create8bppBitmap(otherDC);
		CBitmap *pPrevBmp = otherDC.SelectObject(CBitmap::FromHandle(currBitmap));
		int dw = rect.Width();
		int dh = rect.Height() - EDIT_BAR_HEIGHT;
		int sw = m_bytesWide * 8;
		int sh = m_linesHigh;

		if (!m_zoom) {
			if ((dh*sw) < (dw*sh))
				dw = sw * dh / sh;
			else
				dh = sh * dw / sw;
		} else {
			dw = sw * (1<<(m_zoom-1));
			dh = sh * (1<<(m_zoom-1));

			int rx = dw>rect.Width() ? dw - rect.Width() : 0;
			int ry = dh>(rect.Height()-EDIT_BAR_HEIGHT) ? dh - rect.Height() - EDIT_BAR_HEIGHT : 0;
			if (m_x>rx)
				m_x = rx;
			if (m_y>ry)
				m_y = ry;
		}

		pDC->StretchBlt(-m_x, EDIT_BAR_HEIGHT-m_y, dw, dh, &otherDC, 0, 0, sw, sh, SRCCOPY);
		otherDC.SelectObject(pPrevBmp);

		pDC->SelectObject(pPrevFont);

#ifdef DBLBUF
		// Transfer the off-screen DC to the screen
		BitBlt(pPrevDC->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), hdcMem, 0, 0, SRCCOPY);

		// Free-up the off-screen DC
		SelectObject(hdcMem, hOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);
#endif
		DeleteObject(currBitmap);
	}
}
// C:\code\Step6502\GraphicView.cpp : implementation file
//



void CGraphicView::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	Invalidate();
}

void CGraphicView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	m_zoom = (m_zoom+1)&3;
	Invalidate();
}


void CGraphicView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	if (point.y > EDIT_BAR_HEIGHT) {
		m_drag = true;
		m_dragPrev = point;
		Invalidate();
		return;
	}

	CDockablePane::OnLButtonDown(nFlags, point);
}


void CGraphicView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	m_drag = false;
	CDockablePane::OnLButtonUp(nFlags, point);
}


void CGraphicView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	if (m_drag) {
		if (m_dragPrev != point) {
			m_x -= point.x - m_dragPrev.x;
			m_y -= point.y - m_dragPrev.y;
			m_dragPrev = point;
			Invalidate();
		}
		return;
	}

	CDockablePane::OnMouseMove(nFlags, point);
}


void CGraphicView::OnMouseLeave()
{
	// TODO: Add your message handler code here and/or call default
	m_drag = false;
	CDockablePane::OnMouseLeave();
}
