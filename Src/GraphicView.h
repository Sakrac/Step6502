#pragma once
#include <inttypes.h>

class CGraphicView;

// CGraphicField

class CGraphicField : public CEdit
{
	DECLARE_DYNAMIC(CGraphicField)

public:
	enum FieldType {
		ADDRESS,
		WIDTH,
		HEIGHT,
		FONT
	};

	CGraphicField();
	virtual ~CGraphicField();
	void SetGraphicView(CGraphicView *pMem, FieldType _type) { m_view = pMem; m_type = _type; }

protected:
	CGraphicView *m_view;
	FieldType m_type;
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnEnChange();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg UINT OnGetDlgCode();
};


class CGraphicStyle : public CComboBox
{
	DECLARE_DYNAMIC(CGraphicStyle)

public:
	CGraphicStyle();
	virtual ~CGraphicStyle();

	void SetGraphicView(CGraphicView *pMem) { m_view = pMem; }

protected:
	CGraphicView *m_view;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnCbnSelchange();
};


// CGraphicView

class CGraphicView : public CDockablePane
{
	DECLARE_DYNAMIC(CGraphicView)
public:
	enum GraphicLayout {
		GL_BITPLANE,
		GL_COLUMNS,
		GL_8X8,
		GL_SPRITES,
		GL_TEXTMODE
	};

	CGraphicView();
	virtual ~CGraphicView();

	void SetLayout(GraphicLayout layout) { m_layout = layout;
		m_editFontAddr.ShowWindow(layout == GL_TEXTMODE ? SW_SHOW : SW_HIDE); }

	void SetField(int value, CGraphicField::FieldType type) {
		switch (type) {
			case CGraphicField::FieldType::ADDRESS:
				m_address = (uint16_t)value;
				break;
			case CGraphicField::FieldType::WIDTH:
				m_bytesWide = value;
				break;
			case CGraphicField::FieldType::HEIGHT:
				m_linesHigh = value;
				break;
			case CGraphicField::FieldType::FONT:
				m_fontAddress = value;
				break;
		}
	}

protected:
	uint16_t m_address;
	uint16_t m_fontAddress;
	int		m_bytesWide;
	int		m_linesHigh;
	int		m_zoom;
	bool	m_drag;
	CPoint	m_dragPrev;
	int		m_x, m_y;
	GraphicLayout m_layout;

	uint8_t *m_currBuffer;
	size_t m_currBufSize;


	HBITMAP Create8bppBitmap(HDC hdc);
	
	CGraphicField m_editAddress;
	CGraphicField m_editWidth;
	CGraphicField m_editHeight;
	CGraphicField m_editFontAddr;
	CGraphicStyle m_columns;


	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnEditCopy();
};


