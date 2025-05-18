/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <shlobj.h>
#include <Shlwapi.h>

#include "CLCLPlugin.h"
#include "resource.h"
#include "query\lvstring.h"

using namespace std;

/* Define */
#define	INI_FILE_NAME	TEXT("tool_clip.ini")

/* Global Variables */
HINSTANCE hInst;
tstring ini_path;

TCHAR whiteSpaceReplacement[BUF_SIZE];
tstring connection_string;

/* Local Function Prototypes */
static BOOL dll_initialize(void);
static BOOL dll_uninitialize(void);

/*
 * DllMain 
 */
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hInst = hInstance;
		dll_initialize();
		break;

	case DLL_PROCESS_DETACH:
		dll_uninitialize();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

/*
 * get_tool_info_w
 * Get tool information
 *
 *	argument:
 *		hWnd - the calling window
 *		index - the index of the acquisition (from 0)
 *		tgi - tool retrieval information
 *
 *	Return value:
 *		TRUE - has tools to get next
 *		FALSE - end of acquisition
 */
__declspec(dllexport) BOOL CALLBACK get_tool_info_w(const HWND hWnd, const int index, TOOL_GET_INFO* tgi)
{
	switch (index) {
	case 0:
		//lstrcpy(tgi->title, TEXT("Replace WhiteSpace"));
		LoadString(hInst, IDS_REPLACE_WHITESPACE, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("replaceWhiteSpace"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_MENU_COPY_PASTE | CALLTYPE_VIEWER;
		return TRUE;

	case 1:
		//lstrcpy(tgi->title, TEXT("Tab-separated to HTML-table"));
		LoadString(hInst, IDS_TABSEPERATE_TO_HTML, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("tabSeparatedToHtmlTable"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 2:
		//lstrcpy(tgi->title, TEXT("Save to database"));
		LoadString(hInst, IDS_SAVE_TO_DB, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("save_odbc"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_VIEWER;
		return TRUE;

	case 3:
		//lstrcpy(tgi->title, TEXT("Load from database"));
		LoadString(hInst, IDS_LOAD_FROM_DB, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("load_odbc"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_VIEWER;
		return TRUE;

	case 4:
		lstrcpy(tgi->title, TEXT("Save to JSON"));
		lstrcpy(tgi->func_name, TEXT("save_json"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_VIEWER;
		return TRUE;

	case 5:
		lstrcpy(tgi->title, TEXT("Load from JSON"));
		lstrcpy(tgi->func_name, TEXT("load_json"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_VIEWER;
		return TRUE;

	}
	return FALSE;
}

/*
 * dll_initialize
 */
static BOOL dll_initialize(void)
{
	TCHAR app_path[MAX_PATH];
	TCHAR* p, * r;

	// Handle NULL instead of hInst indicates to look 
	// for the path of the exe instead of the current dll.
	GetModuleFileName(NULL, app_path, MAX_PATH - 1);
	for (p = r = app_path; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			continue;
		}
#endif	// UNICODE
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			r = p;
		}
	}
	*r = TEXT('\0');

	UINT portable = 0;
	tstring clcl_ini_path = linguversa::string_format(TEXT("%s\\%s"), app_path, TEXT("clcl.ini"));
	tstring ini_dir;
	if (::PathFileExists(clcl_ini_path.c_str()) == TRUE) {
		portable = ::GetPrivateProfileInt(TEXT("GENERAL"), TEXT("GENERAL"), 0, clcl_ini_path.c_str());
	}
	if (portable == 1) {
		// locate tool_clip.ini in the same folder as clcl.ini besides clcl.exe
		ini_path = linguversa::string_format(TEXT("%s\\%s"), app_path, INI_FILE_NAME);
		ini_dir.assign(app_path);
	}
	else {
		// For release mode (not portable app) set ini_path to the same folder as %LOCALAPPDATA%\CLCL\clcl.ini
		// so that we have write access.
		// See C:\home\wilf\Projects\Clcl.cvs\main.c line 2234 ff
		TCHAR local_app_data[MAX_PATH];
		if (SUCCEEDED(::SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, local_app_data))) {
			//lstrcat(work_path, TEXT("\\CLCL"));
			ini_dir = linguversa::string_format(TEXT("%s\\CLCL"), local_app_data);
			ini_path = linguversa::string_format(TEXT("%s\\%s"), ini_dir.c_str(), INI_FILE_NAME);
			::CreateDirectory(ini_dir.c_str(), NULL);
			if (::PathFileExists(ini_path.c_str()) == FALSE) {
				// TODO: copy from old location in the dll-folder
			}
		}
	}

	if (ini_path.empty())
		return FALSE;

	// Initialize global variable for whitespace
	::GetPrivateProfileString(TEXT("ReplaceWhiteSpace"), TEXT("replacement"), TEXT(", "), whiteSpaceReplacement, BUF_SIZE - 1, ini_path.c_str());

	// Initialize global variables for database
	TCHAR conn[BUF_SIZE];
	::GetPrivateProfileString(TEXT("ODBC"), TEXT("ConnectionString"), TEXT(""), conn, BUF_SIZE - 1, ini_path.c_str());
	connection_string.assign(conn);
	if (connection_string.empty())
		connection_string = linguversa::string_format(_T("Driver={SQLite3 ODBC Driver};Database=%s\\test.db3;"), ini_dir.c_str());

	return TRUE;
}

/*
 * dll_uninitialize
 */
static BOOL dll_uninitialize(void)
{
	return TRUE;
}


/* End of source */
