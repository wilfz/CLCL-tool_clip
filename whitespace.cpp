

/* Include Files */
#include "CLCLPlugin.h"
#ifndef IDD_REPLACE_WHITESPACE
#include "resource.h"
#endif
// tstring is defined as string or wstring depending on UNICODE flag
#include "query\tstring.h"

using namespace std;

extern HINSTANCE hInst;
extern tstring ini_path;
extern TCHAR whiteSpaceReplacement[BUF_SIZE];


static BOOL CALLBACK set_WhiteSpaceReplacement(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_EDIT_REPLACEMENT, WM_SETTEXT, 0, (LPARAM)whiteSpaceReplacement);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			SendDlgItemMessage(hDlg, IDC_EDIT_REPLACEMENT, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)whiteSpaceReplacement);

			::WritePrivateProfileString(TEXT("ReplaceWhiteSpace"), TEXT("replacement"), whiteSpaceReplacement, ini_path.c_str());
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}


/*
 * item_replaceWhiteSpace
 * replace any amount of whitespace with the specified string
 */
static int item_replaceWhiteSpace(DATA_INFO* di, const TCHAR* str_replacement)
{
	HANDLE ret;
	TCHAR* from_mem, * to_mem;
	int size;

	if (str_replacement == nullptr)
		return TOOL_DATA_MODIFIED;

	// lock copy source
	if ((from_mem = (TCHAR*) GlobalLock(di->data)) == nullptr) {
		return TOOL_ERROR;
	}

	// calculate target size
	size = 1;
	for (int i = 0; from_mem[i] != TEXT('\0'); i++)
	{
		if (from_mem[i] != TEXT(' ') && from_mem[i] != TEXT('\t')) // blank or tab
		{
			size++;
			continue;
		}

		size += lstrlen(str_replacement);

		while (from_mem[i] == TEXT(' '))
			i++;
		if (from_mem[i] == TEXT('\t'))
			i++;
		while (from_mem[i] == TEXT(' '))
			i++;
		i--;
	}

	// reserve memory for copy target
	if ((ret = GlobalAlloc(GHND, sizeof(TCHAR) * size)) == NULL) {
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}
	if ((to_mem = (TCHAR*) GlobalLock(ret)) == NULL) {
		GlobalFree(ret);
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}

	// copying to target
	size_t j = 0;
	for (int i = 0; from_mem[i] != TEXT('\0'); i++)
	{
		if (from_mem[i] != TEXT(' ') && from_mem[i] != TEXT('\t')) // blank or tab
		{
			to_mem[j++] = from_mem[i];
			continue;
		}

		for (size_t k = 0; str_replacement[k] != TEXT('\0'); k++)
			to_mem[j++] = str_replacement[k];

		while (from_mem[i] == TEXT(' '))
			i++;
		if (from_mem[i] == TEXT('\t'))
			i++;
		while (from_mem[i] == TEXT(' '))
			i++;
		i--;
	}
	to_mem[j++] = TEXT('\0');

	GlobalUnlock(ret);
	GlobalUnlock(di->data);

	GlobalFree(di->data);
	di->data = ret;
	di->size = sizeof(TCHAR) * size;
	return TOOL_DATA_MODIFIED;
}

/*
 * replaceWhiteSpace_tool
 * Tool processing
 *
 *	argument:
 *		hWnd - the calling window
 *		tei - tool execution information
 *		tdi - item information for tools
 *
 *	Return value:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK replaceWhiteSpace(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	DATA_INFO* di;
	int ret = TOOL_SUCCEED;

	TCHAR localReplacement[BUF_SIZE];
	TCHAR* p, * r;
	size_t n = 0;

	// first step: use hardcoded strWhiteSpaceReplacement
	lstrcpy(localReplacement, TEXT(", "));

	// does tei->cmd_line contain data?
	if (tei->cmd_line != NULL && *tei->cmd_line != TEXT('\0'))
	{
		// copy content for localReplacement from tei->cmd_line
		for (p = tei->cmd_line, r = localReplacement; *p != TEXT('\0') && ++n < BUF_SIZE; p++)
		{
			*(r++) = *p;
		}
		*r = TEXT('\0');
	}
	else
	{
		// tei->cmd_line is empty then open the dialog to get the strings for str_open and str_close.
		if ((tei->cmd_line == NULL || *tei->cmd_line == TEXT('\0')) &&
			((tei->call_type & CALLTYPE_MENU) || (tei->call_type & CALLTYPE_VIEWER))) {
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_REPLACE_WHITESPACE), GetForegroundWindow(), set_WhiteSpaceReplacement, 0) == FALSE) {
				return TOOL_CANCEL;
			}
		}
		lstrcpy(localReplacement, whiteSpaceReplacement);
	}

	for (; tdi != NULL; tdi = tdi->next) {
		if (SendMessage(hWnd, WM_ITEM_CHECK, 0, (LPARAM)tdi->di) == -1) {
			continue;
		}
#ifdef UNICODE
		di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)tdi->di);
#else
		di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)tdi->di);
#endif
		if (di != NULL && di->data != NULL) {
			ret |= item_replaceWhiteSpace(di, localReplacement);
		}
	}
	return ret;
}

/*
 * replaceWhiteSpace_property
 * Show properties
 *
 *	argument:
 *		hWnd - handle of the options window
 *		tei - tool execution information
 *
 *	Return value:
 *		TRUE - with properties
 *		FALSE - no property
 */
__declspec(dllexport) BOOL CALLBACK replaceWhiteSpace_property(const HWND hWnd, TOOL_EXEC_INFO* tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_REPLACE_WHITESPACE), hWnd, set_WhiteSpaceReplacement, 0);
	return TRUE;
}

