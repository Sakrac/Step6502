#pragma once


// CLoadAddress dialog

class CLoadAddress : public CDialog
{
	DECLARE_DYNAMIC(CLoadAddress)

public:
	CLoadAddress(CWnd* pParent = NULL);   // standard constructor
	virtual ~CLoadAddress();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LOAD_ADDRESS };
#endif

	void Setup();

	int m_bForceLoadAddress;
	int m_bResetUndo;
	int m_bSetPCToLoadAddress;
	int m_bFileType;
	CString m_AddrStr;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
};
