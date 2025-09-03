
/* Include Files */
#include "CLCLPlugin.h"
#ifndef IDD_REPLACE_WHITESPACE
#include "resource.h"
#endif
#include <string>
#include <sstream>
#include <regex>
#include "winwrappers.h"
#include "clipitem.h"
#include "memory.h"

using namespace std;

// tei->cmdline can only hold one string.
// To separate search expression fom replacement
// we use special char 0x1f aka US as separator
// within that string.
#define RX_SEPARATOR L'\x1f'

extern HINSTANCE hInst;
extern wstring ini_path;

TOOL_EXEC_INFO* dlg_tei = nullptr;
wchar_t searchexpr[BUF_SIZE];
wchar_t replaceexpr[BUF_SIZE];

static BOOL CALLBACK dlg_replace_regex(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * convert regular expression
 */
__declspec(dllexport) int CALLBACK clipregex(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	if (tdi == nullptr || tdi->di == nullptr) {
		return TOOL_SUCCEED;
	}

	if (tei->cmd_line == nullptr || *tei->cmd_line == TEXT('\0')) {
		return TOOL_CANCEL;
	}

	cl_item item(tdi->di);
	wstring input = item.textcontent;
	wstring wout;

	if (input.empty())
		return TOOL_SUCCEED; // nothing more to do

	wstring s(tei->cmd_line);
	size_t nsep = s.find(RX_SEPARATOR);

	if (nsep == wstring::npos || nsep == 0)
		return TOOL_SUCCEED; // nothing more to do

	wstring search_expression = s.substr(0, nsep);
	wstring replacement = s.substr(nsep + 1);

	auto re = std::wregex(search_expression);
	wsmatch m;

	if (regex_search( input, m, re) == true && m.ready() == true) {
		wout = regex_replace(input, re, replacement); // , regex_constants::match_flag_type::format_sed);
		item.set_textcontent(wout);
	}

	return TOOL_SUCCEED;
}

__declspec(dllexport) BOOL CALLBACK clipregex_property(const HWND hWnd, TOOL_EXEC_INFO* tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_REPLACE), hWnd, dlg_replace_regex, (LPARAM)tei);
	return TRUE;
}

bool init_replace_regex(HWND hDlg, const TOOL_EXEC_INFO* tei = nullptr)
{
	wstring s;
	wstring searchexpr;
	wstring replaceexpr;
	if (tei && tei->cmd_line)
		s.assign(tei->cmd_line);

	size_t nsep = s.find(RX_SEPARATOR);
	if (nsep != wstring::npos && nsep > 0) {
		searchexpr = s.substr(0, nsep);
		replaceexpr = s.substr(nsep + 1);
	}

	// set the GUI controls from searchexpr and replaceexpr
	wndctrl(hDlg, IDC_SEARCHEXPRESSION).set_window_text(searchexpr);
	wndctrl(hDlg, IDC_REPLACEEXPRESSION).set_window_text(replaceexpr);

	return true;
}

// dialogue translation dictionaries
static BOOL CALLBACK dlg_replace_regex(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hDlg == NULL)
		return FALSE;

	switch (uMsg) {
	case WM_INITDIALOG:
		dlg_tei = (TOOL_EXEC_INFO*)lParam;
		return (BOOL)init_replace_regex(hDlg, dlg_tei);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		{
			// get searchexpr and replaceexpr from their GUI controls.
			wstring searchexpr = wndctrl(hDlg, IDC_SEARCHEXPRESSION).get_window_text();
			wstring replaceexpr = wndctrl(hDlg, IDC_REPLACEEXPRESSION).get_window_text();

			// Separate the two parts with the special char RX_SEPARATOR (defined above).
			wstring s = searchexpr + wstring({ RX_SEPARATOR }) + replaceexpr;

			// By returning the content to tei->cmdline we enable CLCLSet.exe 
			// to store searchexpr and replaceexpr to clcl.ini.
			// Thus there can be several tool items of clipregex with different
			// search and replace expressions.
			if (dlg_tei && dlg_tei->cmd_line)
				::mem_free((void**)&dlg_tei->cmd_line);

			if (dlg_tei)
				dlg_tei->cmd_line = ::alloc_copy(s.c_str());

			EndDialog(hDlg, TRUE);
			break;
		}

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
