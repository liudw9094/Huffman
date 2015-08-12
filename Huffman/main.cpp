/*
		Name: main.cpp
		All rights is reserved by Liu Dw.
		2010 Des.
*/
#include "stdafx.h"
#include "resource.h"
#include "Huffman.h"
#include "file.h"

using namespace std;

static wchar_t szInitPath[260] = _T("");
CCombinedHufFile	g_cmbHufFile;
CSection			g_cs;
volatile bool	g_bHaveFile = false;
volatile bool	g_bBusy = false;
volatile HWND	g_hwndBusy = NULL;
volatile HWND	g_hWnd = NULL;
int				g_nSelectedItem = -1;
#define CMB_HUF_FILTER L"Combined huffman file (*.chf)\0*.chf\0"
#define CMB_HUF_FILTER_ALL L"Combined huffman file (*.chf)\0*.chf\0All files (*.*)\0*.*"
#define CMB_ALL_FILTER L"All files (*.*)\0*.*"
static bool OpenFileDialog(wchar_t* szPath, const wchar_t* szFilter, HWND hwnd, DWORD dwFilterIndex = 1);
static bool SaveFileDialog(wchar_t* szPath, const wchar_t* szFilter, HWND hwnd, DWORD dwFilterIndex = 1);
static INT_PTR CALLBACK dlgBusyFunc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

struct THREAD_PARAM
{
	wchar_t szFilePath[MAX_PATH];
	HWND hWnd;
};
struct THREAD_PARAM_DECODE
{
	wchar_t szFileName[MAX_PATH];
	wchar_t szFilePathSave[MAX_PATH];
	HWND hWnd;
};
THREAD_PARAM threadParam;
THREAD_PARAM_DECODE threadParam1;

void ReleaseFile()
{
	g_cmbHufFile.Release();
	g_bHaveFile = false;
}
void UpdataView(HWND hwnd)
{
	CSafeLock lock(g_cs);
	::EnableWindow(::GetDlgItem(hwnd, IDC_ADD), g_bHaveFile);
	::EnableWindow(::GetDlgItem(hwnd, IDC_DELETE), g_bHaveFile);
	::EnableWindow(::GetDlgItem(hwnd, IDC_DECODE), g_bHaveFile);
	HWND hEdit = ::GetDlgItem(hwnd, IDC_E_READFILE);
	::SetWindowTextW(hEdit, L"");
	HWND hList = ::GetDlgItem(hwnd, IDC_LST_FILE);
	while(ListView_DeleteItem(hList, 0));
	g_nSelectedItem = -1;
	if(!g_bHaveFile)
		return;
	DWORD count = g_cmbHufFile.GetFileCount();
	for(DWORD i = 0; i < count; ++i)
	{
		const CHuffmanFile *pHufFile = g_cmbHufFile.GetFile(i);
		if(!pHufFile)
			return;
		SHuffman_File_Head head = *((SHuffman_File_Head*)pHufFile->GetData());
		LVITEMW lvi = {0};
		lvi.iSubItem = 0;
		lvi.pszText = (wchar_t*)pHufFile->GetFileName();
		lvi.iItem = i;
		int now=ListView_InsertItem(hList, &lvi);
		wchar_t tmp[256];;
		ListView_SetItemText(hList, now, 0, lvi.pszText);
		swprintf_s(tmp, L"%lu", head.dwSourceFileSize);
		ListView_SetItemText(hList, now, 1, tmp);
		swprintf_s(tmp, L"%lu", head.dwCodeSize);
		ListView_SetItemText(hList, now, 2, tmp);
	}
	g_nSelectedItem = -1;
	::SetWindowTextW(hEdit, g_cmbHufFile.GetFilePath());
}
void ShowBusy(HWND hwnd)
{
	if(!g_hwndBusy)
	{
		g_hwndBusy = CreateDialogW(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_BUSY), hwnd, dlgBusyFunc);
		// Move the window to the main window center
		RECT rect, rect1;
		GetWindowRect(hwnd, &rect);
		GetWindowRect(g_hwndBusy,&rect1);
		rect.top = ( rect.top + rect.bottom - (  rect1.bottom - rect1.top ) ) / 2;
		rect.bottom = rect.top + rect1.bottom - rect1.top;
		rect.left = ( rect.left + rect.right - ( rect1.right - rect1.left ) ) / 2;
		rect.right = rect.left + rect1.right - rect1.left;
		MoveWindow(g_hwndBusy, rect.left, rect.top,
					rect.right - rect.left,
					rect.bottom - rect.top, TRUE);

		ShowWindow(g_hwndBusy, SW_SHOW);      
		UpdateWindow(g_hwndBusy);
		g_bBusy = true;
	}
}
void DestroyBusy()
{
	g_bBusy = false;
}
void OnThreadSave(THREAD_PARAM *param)
{
	CSafeLock lock(g_cs);
	THREAD_PARAM Param;
	if(!param)
		goto fh;
	Param = *param;
	{
		if(!Param.szFilePath)
			goto fh;
		if(g_cmbHufFile.WriteFile(Param.szFilePath))
			g_bHaveFile = true;
		else
			MessageBoxW(Param.hWnd, L"Failed on saving package.", L"Error", MB_ICONERROR );
	}
fh:
	DestroyBusy();
}
void OnThreadOpen(THREAD_PARAM *param)
{
	CSafeLock lock(g_cs);
	THREAD_PARAM Param;
	if(!param)
		goto fh;
	Param = *param;
	{
		if(!Param.szFilePath)
			goto fh;
		ReleaseFile();
		if(g_cmbHufFile.ReadFile(Param.szFilePath))
			g_bHaveFile = true;
		else
			MessageBoxW(Param.hWnd, L"Failed on loading package.", L"Error", MB_ICONERROR );
	}
fh:
	DestroyBusy();
}
void OnThreadAddFile(THREAD_PARAM *param)
{
	CSafeLock lock(g_cs);
	THREAD_PARAM Param;
	if(!param)
		goto fh;
	Param = *param;
	{
		CSimpleFile file;
		if(!file.ReadFile(Param.szFilePath))
		{
			MessageBoxW(Param.hWnd, L"Failed on opening file.", L"Error", MB_ICONERROR );
			goto fh;
		}
		if(!g_cmbHufFile.AddFile(file))
		{
			MessageBoxW(Param.hWnd, L"Failed on encoding file.", L"Error", MB_ICONERROR );
			ReleaseFile();
			goto fh;
		}
		if(!g_cmbHufFile.UpdateFile())
		{
			MessageBoxW(Param.hWnd, L"Failed on saving package.", L"Error", MB_ICONERROR );
			ReleaseFile();
			goto fh;
		}
	}
fh:	
	DestroyBusy();
}
void OnThreadDelFile(THREAD_PARAM *param)
{
	CSafeLock lock(g_cs);
	THREAD_PARAM Param;
	if(!param)
		goto fh;
	Param = *param;
	
	DWORD index = g_cmbHufFile.GetFileIndex(Param.szFilePath);
	if( index == -1 )
		goto fh;
	if( !g_cmbHufFile.DeleteFile(index) )
	{
		MessageBoxW(Param.hWnd, L"Failed on deleting file.", L"Error", MB_ICONERROR );
		goto fh;
	}
	if(!g_cmbHufFile.UpdateFile())
	{
		MessageBoxW(Param.hWnd, L"Failed on saving package.", L"Error", MB_ICONERROR );
		ReleaseFile();
		goto fh;
	}
fh:
	DestroyBusy();
}
void OnThreadDecodeFile(THREAD_PARAM_DECODE *param)
{
	CSafeLock lock(g_cs);
	THREAD_PARAM_DECODE Param;
	CSimpleFile file;
	if(!param)
		goto fh;
	Param = *param;
	DWORD index = g_cmbHufFile.GetFileIndex(Param.szFileName);
	if( index == -1 )
		goto fh;
	if(!g_cmbHufFile.DecodeFile(index, file))
	{
		MessageBoxW(Param.hWnd, L"Failed on decoding file.", L"Error", MB_ICONERROR);
		goto fh;
	}
	if(!file.WriteFile(Param.szFilePathSave))
	{
		MessageBoxW(Param.hWnd, L"Failed on writing file.", L"Error", MB_ICONERROR);
		goto fh;
	}
fh:	
	DestroyBusy();
}
void OnBtnNew(HWND hwnd)
{
	if(g_bBusy)
		return;
	wchar_t szFilePath[MAX_PATH] = L"Untiled.chf";
	if( !SaveFileDialog(szFilePath, CMB_HUF_FILTER, hwnd) )
		return;
	ShowBusy(hwnd);
	ReleaseFile();
	UpdataView(hwnd);
	threadParam.hWnd = hwnd;
	wcscpy_s(threadParam.szFilePath, szFilePath);
	OnThreadSave(&threadParam);
	UpdataView(hwnd);
}
void OnBtnOpen(HWND hwnd)
{
	if(g_bBusy)
		return;
	wchar_t szFilePath[MAX_PATH] = L"";
	if( !OpenFileDialog(szFilePath, CMB_HUF_FILTER_ALL, hwnd) )
		return;
	ShowBusy(hwnd);
	threadParam.hWnd = hwnd;
	wcscpy_s(threadParam.szFilePath, szFilePath);
	_beginthread((void(__cdecl*)(void*))OnThreadOpen, 0, &threadParam);
}
void OnBtnAdd(HWND hwnd)
{
	if(g_bBusy || !g_bHaveFile)
		return;
	wchar_t szFilePath[MAX_PATH] = L"";
	if( !OpenFileDialog(szFilePath, CMB_ALL_FILTER, hwnd) )
		return;
	if(g_cmbHufFile.GetFileIndex(GetFileName(szFilePath)) != -1)
	{
		MessageBoxW(hwnd, L"File name has existed.", L"Error", MB_ICONERROR);
		return;
	}
	ShowBusy(hwnd);
	threadParam.hWnd = hwnd;
	wcscpy_s(threadParam.szFilePath, szFilePath);
	_beginthread((void(__cdecl*)(void*))OnThreadAddFile, 0, &threadParam);
}
void OnBtnDel(HWND hwnd)
{
	if(g_bBusy || !g_bHaveFile)
		return;
	if(g_nSelectedItem == -1)
	{
		MessageBoxW(hwnd, L"No file selected.", L"Error", MB_ICONERROR);
		return;
	}
	if( MessageBoxW(hwnd, L"Are you sure of deleting this file?",
				L"Caution", MB_ICONQUESTION | MB_YESNO ) != IDYES )
		return;

	ShowBusy(hwnd);
	HWND hList = ::GetDlgItem(hwnd, IDC_LST_FILE);
	wchar_t szFileName[MAX_PATH];
	ListView_GetItemText(hList, g_nSelectedItem, 0, szFileName, MAX_PATH);
	threadParam.hWnd = hwnd;
	wcscpy_s(threadParam.szFilePath, szFileName);
	_beginthread((void(__cdecl*)(void*))OnThreadDelFile, 0, &threadParam);
}
void OnBtnDecode(HWND hwnd)
{
	if(g_bBusy || !g_bHaveFile)
		return;
	if(g_nSelectedItem == -1)
	{
		MessageBoxW(hwnd, L"No file selected.", L"Error", MB_ICONERROR);
		return;
	}
	HWND hList = ::GetDlgItem(hwnd, IDC_LST_FILE);
	wchar_t szFileName[MAX_PATH];
	ListView_GetItemText(hList, g_nSelectedItem, 0, szFileName, MAX_PATH);
	wchar_t szFilePath[MAX_PATH];
	wcscpy_s(szFilePath, szFileName);
	if( !SaveFileDialog(szFilePath, CMB_ALL_FILTER, hwnd) )
		return;
	ShowBusy(hwnd);
	threadParam1.hWnd = hwnd;
	wcscpy_s(threadParam1.szFileName, szFileName);
	wcscpy_s(threadParam1.szFilePathSave, szFilePath);
	_beginthread((void(__cdecl*)(void*))OnThreadDecodeFile, 0, &threadParam1);
}
static bool SaveFileDialog(wchar_t* szPath, const wchar_t* szFilter, HWND hwnd, DWORD dwFilterIndex )
{
	OPENFILENAMEW ofn;       // common dialog box structure
	wchar_t szFile[260];       // buffer for file name

	_tcscpy_s(szFile, 260, szPath);
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = dwFilterIndex;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
				OFN_EXPLORER | OFN_NOLONGNAMES |
				OFN_OVERWRITEPROMPT;

	// Display the Save dialog box. 

	if (GetSaveFileNameW(&ofn)) 
	{
		wcscpy_s(szPath, 260, szFile);
		return true;
	}
	return false;
}
static bool OpenFileDialog(wchar_t* szPath, const wchar_t* szFilter, HWND hwnd, DWORD dwFilterIndex )
{
	OPENFILENAMEW ofn;       // common dialog box structure
	wchar_t szFile[260];       // buffer for file name

	_tcscpy_s(szFile, MAX_PATH, szPath);
	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = szFilter;
	ofn.nFilterIndex = dwFilterIndex;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
				OFN_EXPLORER | OFN_NOLONGNAMES;

	// Display the Open dialog box. 

	if (GetOpenFileNameW(&ofn)) 
	{
		wcscpy_s(szPath, MAX_PATH, szFile);
		return true;
	}
	return false;
}

static INT_PTR CALLBACK dlgBusyFunc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		::SetTimer(hDlg, NULL, 100, NULL);
		::EnableWindow(g_hWnd, FALSE);
		return (INT_PTR)TRUE;
	case WM_TIMER:
		if(!g_bBusy)
		{
			UpdataView(g_hWnd);
			::EnableWindow(g_hWnd, TRUE);
			::DestroyWindow(hDlg);
			g_hwndBusy = false;
		}
		return (INT_PTR)TRUE;
	}
	return (INT_PTR)FALSE;
}
static INT_PTR CALLBACK DialogFunc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	wchar_t szFilePath[260] = L"" ;
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		{
			g_hWnd = hDlg;
			_tcscpy_s(szFilePath, 260, szInitPath);
			HICON hIcon = LoadIcon(GetModuleHandle(NULL),
									MAKEINTRESOURCE(IDI_HC));
			SendMessageW(hDlg, WM_SETICON, TRUE, (LPARAM)hIcon);
			SendMessageW(hDlg, WM_SETICON, FALSE, (LPARAM)hIcon);
			if(szFilePath[0])
			{
				THREAD_PARAM param;
				param.hWnd = hDlg;
				wcscpy_s(param.szFilePath, szFilePath);
				OnThreadOpen(&param);
			}
			HWND hList = ::GetDlgItem(hDlg, IDC_LST_FILE);
			LVCOLUMNW column;
			column.mask = LVCF_TEXT|LVCF_FMT|LVCF_WIDTH;
			column.fmt = LVCFMT_LEFT;
			column.cchTextMax=100;
			column.cx=120;
			column.pszText = L"File Name";
			ListView_InsertColumn(hList, 0, &column);
			column.pszText = L"Original Size(bytes)";
			ListView_InsertColumn(hList, 1, &column);
			column.pszText = L"Coded Size(bytes)";
			ListView_InsertColumn(hList, 2, &column);
			UpdataView(hDlg);
			return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		case IDC_DECODE:
			{
				OnBtnDecode(hDlg);
				return (INT_PTR)TRUE;
			}
		case IDC_ADD:
			{
				OnBtnAdd(hDlg);
				return (INT_PTR)TRUE;
			}
		case IDC_OPEN:
			{
				OnBtnOpen(hDlg);
				return (INT_PTR)TRUE;
			}
		case IDC_NEW:
			{
				OnBtnNew(hDlg);
				return (INT_PTR)TRUE;
			}
		case IDC_DELETE:
			{
				OnBtnDel(hDlg);
				return (INT_PTR)TRUE;
			}
		default:
			break;
		}
		break;
	case WM_NOTIFY:
		switch(LOWORD(wParam))
		{
		case IDC_LST_FILE:
			{
				HWND hList = ::GetDlgItem(hDlg, IDC_LST_FILE);
				int nHot = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
				if(nHot != -1 )
					g_nSelectedItem = nHot;
				return (INT_PTR)TRUE;
			}
		default:
			break;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


int wmain(int cmdCount, wchar_t *cmdLine[])
{
	//::InitializeCriticalSection(&g_cs);
	if(cmdCount>= 2)
	{
		wcscpy_s(szInitPath, cmdLine[1]);
		for(int i = 2; i < cmdCount; ++i)
			::ShellExecuteW(NULL, L"open", cmdLine[0], cmdLine[i], L".\\", SW_SHOW);
	}
	DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_MAINUI), NULL, DialogFunc);
	//::DeleteCriticalSection(&g_cs);
	return 0;
}
int APIENTRY wWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance, 
                     LPWSTR     lpCmdLine,
                     int       nCmdShow)
{
	int cmdCount;
	wchar_t **cmdLine = CommandLineToArgvW(GetCommandLineW(), &cmdCount);
	return wmain( cmdCount, cmdLine );
}
