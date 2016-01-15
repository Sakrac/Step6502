// LoadAddress.cpp : implementation file
//

#include "stdafx.h"
#include "Step6502.h"
#include "LoadAddress.h"
#include "afxdialogex.h"


// CLoadAddress dialog

IMPLEMENT_DYNAMIC(CLoadAddress, CDialog)

CLoadAddress::CLoadAddress(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_LOAD_ADDRESS, pParent)
{
	m_bForceLoadAddress = true;
	m_bResetUndo = true;
	m_bSetPCToLoadAddress = true;
	m_bFileType = 0;
	m_AddrStr = "default";
}

CLoadAddress::~CLoadAddress()
{
}

void CLoadAddress::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Check(pDX, IDC_SET_PC_TO_LOAD_ADDRESS2, m_bSetPCToLoadAddress);
	DDX_Check(pDX, IDC_FORCE_ADDRESS_CHECK, m_bForceLoadAddress);
	DDX_Check(pDX, IDC_RESET_UNDO, m_bResetUndo);
	DDX_Radio(pDX, IDC_FILETYPE_PRG, m_bFileType);
	DDX_Text(pDX, IDC_LOAD_ADDRESS_EDIT_MANUAL, m_AddrStr);
}

void CLoadAddress::Setup()
{
}


BEGIN_MESSAGE_MAP(CLoadAddress, CDialog)
END_MESSAGE_MAP()


// CLoadAddress message handlers

