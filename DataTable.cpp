// DataTable.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "DataTable.h"
#include "DatabaseUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataTable

IMPLEMENT_DYNAMIC(CDataTable, CDaoRecordset)

CDataTable::CDataTable(CDaoDatabase* pdb)
:CDaoRecordset(pdb)
{
	//{{AFX_FIELD_INIT(CDataTable)
	m_lID = 0;
	m_lParentID = 0;
	m_strClipBoardFormat = _T("");
	m_nFields = 4;
	//}}AFX_FIELD_INIT
	m_nDefaultType = dbOpenDynaset;
}


CString CDataTable::GetDefaultDBName()
{
	return GetDBName();
}

CString CDataTable::GetDefaultSQL()
{
	return _T("[Data]");
}

void CDataTable::DoFieldExchange(CDaoFieldExchange* pFX)
{
	//{{AFX_FIELD_MAP(CDataTable)
	pFX->SetFieldType(CDaoFieldExchange::outputColumn);
	DFX_Long(pFX, _T("[lID]"), m_lID);
	DFX_Long(pFX, _T("[lParentID]"), m_lParentID);
	DFX_Text(pFX, _T("[strClipBoardFormat]"), m_strClipBoardFormat);
	DFX_LongBinary(pFX, _T("[ooData]"), m_ooData);
	//}}AFX_FIELD_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CDataTable diagnostics

#ifdef _DEBUG
void CDataTable::AssertValid() const
{
	CDaoRecordset::AssertValid();
}

void CDataTable::Dump(CDumpContext& dc) const
{
	CDaoRecordset::Dump(dc);
}
#endif //_DEBUG

// caller must free
// takes m_ooData's HGLOBAL (do not update recset after calling this)
// This should be faster than making a copy, but is this SAFE ?????
HGLOBAL CDataTable::TakeData()
{
	if( m_ooData.m_hData == 0 || m_ooData.m_dwDataLength == 0 )
		return 0;

	// we have to do a realloc in order to make the hGlobal m_dwDataLength
	HGLOBAL hGlobal = ::GlobalReAlloc(m_ooData.m_hData, m_ooData.m_dwDataLength, GMEM_MOVEABLE );
	if( !hGlobal || ::GlobalSize(hGlobal) == 0 )
	{
		TRACE0( GetErrorString(::GetLastError()) );
		//::_RPT0( _CRT_WARN, GetErrorString(::GetLastError()) );
		ASSERT(FALSE);
	}
	// no longer valid
	m_ooData.m_hData = 0;
	m_ooData.m_dwDataLength = 0;
	return hGlobal;
}

// this takes ownership of hgData, freeing m_ooData if necessary
// This should be faster than making a copy, but is this SAFE ?????
BOOL CDataTable::ReplaceData( HGLOBAL hgData, UINT len )
{
	if( m_ooData.m_hData && (m_ooData.m_hData = GlobalFree( m_ooData.m_hData )) )
	{
		ASSERT(FALSE);
		return FALSE;
	}

	m_ooData.m_hData = hgData;
	m_ooData.m_dwDataLength = len;

	//Set the fields dirty
	SetFieldDirty(&m_ooData);
	SetFieldNull(&m_ooData,FALSE);

	return TRUE;
}

// copies hgData into m_ooData using ::GlobalSize(hgData) for the size
BOOL CDataTable::SetData( HGLOBAL hgData )
{
UINT unSize = GlobalSize(hgData);

	//Reallocate m_ooData.m_hData
	if(m_ooData.m_hData)
		m_ooData.m_hData = GlobalReAlloc(m_ooData.m_hData, unSize, GMEM_MOVEABLE);
	else
		m_ooData.m_hData = GlobalAlloc(GHND, unSize);

	m_ooData.m_dwDataLength = unSize;

	::CopyToGlobalHH( m_ooData.m_hData, hgData, unSize );

	//Set the fields dirty
	SetFieldDirty(&m_ooData);
	SetFieldNull(&m_ooData,FALSE);

	return TRUE;
}

// allocates a new copy of the data
HGLOBAL CDataTable::LoadData()
{
HGLOBAL hGlobal;
ULONG ulBufLen = m_ooData.m_dwDataLength; //Retrieve size of array

	if(ulBufLen == 0)
		return 0;

	hGlobal = NewGlobalH( m_ooData.m_hData, ulBufLen );

	return hGlobal;
}

BOOL CDataTable::DeleteAll()
{
	BOOL bRet = FALSE;
	try
	{
		theApp.EnsureOpenDB();
		theApp.m_pDatabase->Execute("DELETE * FROM Data", dbFailOnError);
		bRet = TRUE;
	}
	catch(CDaoException* e)
	{
		AfxMessageBox(e->m_pErrorInfo->m_strDescription);
		e->Delete();
	}

	return bRet;
}

void CDataTable::Open(LPCTSTR lpszFormat,...) 
{
	m_pDatabase = theApp.EnsureOpenDB();

	CString csText;
	va_list vlist;

	ASSERT(AfxIsValidString(lpszFormat));
	va_start(vlist,lpszFormat);
	csText.FormatV(lpszFormat,vlist);
	va_end(vlist);
	
	CDaoRecordset::Open(AFX_DAO_USE_DEFAULT_TYPE, csText, 0);
}

void CDataTable::Open(int nOpenType, LPCTSTR lpszSql, int nOptions) 
{
	m_pDatabase = theApp.EnsureOpenDB();
	
	CDaoRecordset::Open(nOpenType, lpszSql, nOptions);
}

BOOL CDataTable::DataEqual(HGLOBAL hgData)
{
	return ::CompareGlobalHH( hgData, m_ooData.m_hData, m_ooData.m_dwDataLength ) == 0;
}