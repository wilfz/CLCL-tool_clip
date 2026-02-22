/* Include Files */
#include "CLCLPlugin.h"
#include <string>
#include <cassert>
#include <map>
#include <tchar.h>
#include "string.h"
#include "clipitem.h"


using namespace std;

// forward declarations of local functions and classes
tstring select_macro(const HWND hWnd, const tstring& folder);
void expand_macros(tstring& s);
void expand_from_history(const HWND hWnd, tstring& s, tstring clipsel = TEXT(""));

/*
 * expand_envvar_tool
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
__declspec(dllexport) int CALLBACK expand_envvar(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	DATA_INFO* di;
	int ret = TOOL_SUCCEED;

	if (tdi == nullptr || tei == nullptr)
		return TOOL_ERROR;

	if ((tei->call_type & CALLTYPE_VIEWER) != 0) {
		return TOOL_ERROR; // not allowed
	}

	di = tdi->di;

	if (di == nullptr || di->type != TYPE_ITEM && di->type != TYPE_DATA)
		return TOOL_ERROR;

	// If command line is given get the macro to be expanded from there.
	if (tei->cmd_line != nullptr && *tei->cmd_line != TEXT('\0')) {
		cl_item item(tdi->di);
		// get the currently selected text
		tstring clipsel = item.textcontent;

		// Open the template folder and look for a folder named like the command line, 
		// if it exists, select a macro from there and expand it.
		tstring folder(tei->cmd_line);
		tstring macro = select_macro(hWnd, folder);

		// If no such folder exists, use command line itself as macro.
		if (macro.empty())
			return TOOL_CANCEL;

		// Just find out the size of memory to be allocated in the next call.
		DWORD len = ::ExpandEnvironmentStrings(macro.c_str(), NULL, 0);
		if (len <= 0) {
			return TOOL_ERROR;
		}

		TCHAR* tmp = new TCHAR[len + 1];
		// Second call: now we have the target buffer alocated and can do the real expansion.
		len = ::ExpandEnvironmentStrings(macro.c_str(), tmp, len);
		if (len <= 0) {
			return TOOL_ERROR;
		}

		tstring expanded(tmp);
		assert(len == expanded.length() + 1);
		delete[] tmp;

		// Replace %DATE%, and %TIME% with appropriate strings
		expand_macros(expanded);

		// Replace %CLIPSEL% with current selection and
		// replace %CLIPHIST:n% with the n-th history item.
		expand_from_history( hWnd, expanded, clipsel);

		len = expanded.length() + 1; // include terminating null

		item.set_textcontent(expanded);
		//SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)di);

		// Notify that data was modified
		ret |= TOOL_DATA_MODIFIED;

		return ret;
	}

	// No command line is given; we get the expression to be expanded from the current item.
	for (; tdi != NULL; tdi = tdi->next) {
		if (SendMessage(hWnd, WM_ITEM_CHECK, 0, (LPARAM)tdi->di) == -1) {
			continue;
		}

#ifdef UNICODE
		di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)tdi->di);
#else
		di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)tdi->di);
#endif

		if (di == nullptr || di->data == nullptr)
			continue;

		// if item is a history oder registry item (CALLTYPE_HISTORY oder CALLTYPE_REGIST)
		// leave item as it is, modify copy of it
		// and insert as newest into history
		if ((tei->call_type & CALLTYPE_VIEWER) != 0) {
			di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_COPY, 0, (LPARAM)di);
		}

		if (di == nullptr || di->data == nullptr)
			continue;

		TCHAR* from_mem = NULL;
		// lock copy source
		if ((from_mem = (TCHAR*)GlobalLock(di->data)) == nullptr) {
			return TOOL_ERROR;
		}

		if (_tcslen(from_mem) == 0) {
			GlobalUnlock(di->data);
			return TOOL_ERROR;
		}

		// Just find out the size of memory to be allocated in the next call.
		DWORD len = ::ExpandEnvironmentStrings(from_mem, NULL, 0);
		if (len <= 0) {
			GlobalUnlock(di->data);
			return TOOL_ERROR;
		}

		TCHAR* tmp = new TCHAR[len + 1];
		// Second call: now we have the target buffer alocated and can do the real expansion.
		len = ::ExpandEnvironmentStrings(from_mem, tmp, len);
		if (len <= 0) {
			GlobalUnlock(di->data);
			return TOOL_ERROR;
		}

		tstring expanded(tmp);
		assert(len == expanded.length() + 1);
		delete[] tmp;

		// Replace %DATE%, and %TIME% with apprpriate strings
		expand_macros(expanded);

		// Replace %CLIPSEL% with current selection and
		// replace %CLIPHIST:n% with the n-th history item.
		expand_from_history(hWnd, expanded);

		len = expanded.length() + 1; // include terminating null

		// Copy expanded.c_str() to to_mem
		HANDLE hnd = NULL;
		// reserve memory for copy target
		if ((hnd = GlobalAlloc(GHND, sizeof(TCHAR) * len)) == NULL) {
			GlobalUnlock(di->data);
			return TOOL_ERROR;
		}

		TCHAR* to_mem = nullptr;
		if ((to_mem = (TCHAR*)GlobalLock(hnd)) == NULL) {
			GlobalFree(hnd);
			GlobalUnlock(di->data);
			return TOOL_ERROR;
		}

		_tcscpy_s(to_mem, len, expanded.c_str());

		GlobalUnlock(hnd);
		GlobalUnlock(di->data);

		GlobalFree(di->data);

		di->data = to_mem;
		di->size = len * sizeof(TCHAR); // include terminating null

		SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)di);

		// Notify that data was modified
		ret |= TOOL_DATA_MODIFIED;
	}

	return ret;
}

// フォーカス情報
// focus information
typedef struct _FOCUS_INFO {
	HWND active_wnd;
	HWND focus_wnd;
	POINT cpos;
	BOOL caret;
} FOCUS_INFO;

/*
 * get_focus_info - フォーカス情報を取得
 */
void get_focus_info(FOCUS_INFO* fi)
{
	// フォーカスを持つウィンドウの取得
	// get the window with focus
	fi->active_wnd = GetForegroundWindow();
	AttachThreadInput(GetWindowThreadProcessId(fi->active_wnd, NULL), GetCurrentThreadId(), TRUE);
	fi->focus_wnd = GetFocus();
	// キャレット位置取得
	// get caret position
	if (GetCaretPos(&fi->cpos) == TRUE && (fi->cpos.x > 0 || fi->cpos.y > 0)) {
		ClientToScreen(fi->focus_wnd, &fi->cpos);
		fi->caret = TRUE;
	}
	else {
		fi->caret = FALSE;
	}
	AttachThreadInput(GetWindowThreadProcessId(fi->active_wnd, NULL), GetCurrentThreadId(), FALSE);
}

tstring select_macro(const HWND hWnd, const tstring& folder)
{
	// Get registry_root
	DATA_INFO* registry_root = (DATA_INFO*)SendMessage(hWnd, WM_REGIST_GET_ROOT, 0, 0);
	if (registry_root == nullptr)
		return tstring();

	// Find the folder containing the macros
	DATA_INFO* fdi;
	for (fdi = registry_root->child; fdi != nullptr; fdi = fdi->next) {
		if (fdi->title == nullptr)
			continue;
		if (_tcsicmp(fdi->title, folder.c_str()) == 0)
			break;
	}

	if (fdi == nullptr)
		return tstring(); // nothing found

	// if it's an item return its text content
	if (fdi->type == TYPE_ITEM)
		return cl_item(fdi).get_textcontent();

	if (fdi->type != TYPE_FOLDER)
		return tstring(); // what else can it be?

	// メニューの作成
	// Creating a menu
	HMENU hMenu = CreatePopupMenu();
	// this map will contain macros which can be expanded
	map<unsigned int, tstring> macro;
	vector<tstring> content;
	content.reserve(64); // there will be rarely more than 64 macros
	content.push_back(tstring()); // dummy for 1-based indexing of menu items))
	unsigned int i = 0;

	// fill menu with the folder items
	for (DATA_INFO* di = fdi->child; di != nullptr; di = di->next) {
		cl_item mi(di);
		if (mi.itemtype == TYPE_FOLDER)
			continue;

		tstring macro_name = mi.get_title();
		if (mi.title.empty())
			macro_name = mi.get_textcontent().substr(0, 48);
		macro[++i] = mi.get_textcontent();
		content.push_back(mi.get_textcontent());
		AppendMenu(hMenu, MF_STRING, i, macro_name.c_str());
	}

	//  no need to display an empty menu
	if (i == 0) {
		DestroyMenu(hMenu);
		return tstring(); // nothing found
	}

	FOCUS_INFO fi;
	get_focus_info(&fi);

	// メニュー表示
	// Display menu
	HWND hCurrentWnd = GetForegroundWindow();
	SetForegroundWindow(hWnd);
	ShowWindow(hWnd, SW_HIDE);
	POINT apos;
	if (fi.caret == TRUE && fi.cpos.x >= 0 && fi.cpos.x < GetSystemMetrics(SM_CXSCREEN)
				&& fi.cpos.y >= 0 && fi.cpos.y < GetSystemMetrics(SM_CYSCREEN))
		apos = fi.cpos;
	else
	GetCursorPos((LPPOINT)&apos);

	i = TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD, apos.x, apos.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);
	SetForegroundWindow(hCurrentWnd);
	if (i <= 0) {
		return tstring();
	}

	// after all this is the text content to be expanded
	return macro[i];
	// return content[i];
}

// Replace %DATE%, and %TIME% with apprpriate strings
void expand_macros(tstring& s)
{
	SYSTEMTIME current;
	GetLocalTime(&current);
	TCHAR date[BUF_SIZE];
	// Get the default format for date.
	int len_dt = GetDateFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		DATE_SHORTDATE,
		&current,
		nullptr,
		date,
		BUF_SIZE,
		nullptr
	);

	size_t p_start, p_end;
	// Replace all occurences of %DATE% with the default format for dates.
	while ((p_start = s.find(TEXT("%DATE%"))) != tstring::npos) {
		if (len_dt > 0) {
			s = s.substr(0, p_start) + tstring(date) + s.substr(p_start + 6);
			//s.replace(p_start, 6, buf);
		}
	}

	// Replace all occurences of %DATE:{format}% with the formatted date. {format} must comply with 
	// https://learn.microsoft.com/en-us/windows/win32/intl/day--month--year--and-era-format-pictures
	while ((p_start = s.find(TEXT("%DATE:"))) != tstring::npos 
			&& (p_end = s.find(TEXT("%"), p_start + 6)) != tstring::npos) {

		tstring format = s.substr(p_start + 6, p_end - p_start - 6);
		len_dt = GetDateFormatEx(
			LOCALE_NAME_USER_DEFAULT,
			0,
			&current,
			format.c_str(),
			date,
			BUF_SIZE,
			nullptr
		);

		if (len_dt > 0) {
			s = s.substr(0, p_start) + tstring(date) + s.substr(p_end + 1);
		}
	}

	TCHAR time[BUF_SIZE];
	// Get the current time in default format.
	len_dt = GetTimeFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		TIME_FORCE24HOURFORMAT,
		&current,
		nullptr,
		time,
		BUF_SIZE
	);

	// Replace all occurences of %TIME% with the default format for time.
	while ((p_start = s.find(TEXT("%TIME%"))) != tstring::npos) {
		if (len_dt > 0) {
			s = s.substr(0, p_start) + tstring(time) + s.substr(p_start + 6);
			//s.replace(p_start, 6, buf);
		}
	}

	// Replace all occurences of %TIME:{format}% with the formatted time. {format} must comply with 
	// https://learn.microsoft.com/en-us/windows/win32/api/datetimeapi/nf-datetimeapi-gettimeformatex
	while ((p_start = s.find(TEXT("%TIME:"))) != tstring::npos
		&& (p_end = s.find(TEXT("%"), p_start + 6)) != tstring::npos) {

		tstring format = s.substr(p_start + 6, p_end - p_start - 6);
		len_dt = GetTimeFormatEx(
			LOCALE_NAME_USER_DEFAULT,
			0,
			&current,
			format.c_str(),
			time,
			BUF_SIZE
		);

		if (len_dt > 0) {
			s = s.substr(0, p_start) + tstring(time) + s.substr(p_end + 1);
		}
	}

	return;
}

// Helper function to check if a string is a number (integer)
bool is_number(const TCHAR* str) {
	if (str == nullptr || *str == TEXT('\0'))
		return false;
	const TCHAR* p = str;
	// Optional: allow negative numbers
	if (*p == TEXT('-')) ++p;
	while (*p) {
		if (*p < TEXT('0') || *p > TEXT('9'))
			return false;
		++p;
	}
	return true;
}

// Helper function: read the n newest tetxt items from clipboard history
bool read_from_history(const HWND hWnd, vector<tstring>& hist_content, size_t n) {
	// Get registry_root
	DATA_INFO* registry_root = (DATA_INFO*)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0);
	if (registry_root == nullptr)
		return false;

	// otherwise iterate to the n-th element of history and get its content
	unsigned int i = 0; // first history item is 0, is the current selection
	DATA_INFO* di;
	for (di = registry_root->child; di != nullptr; di = di->next, i++) {
		if (i < hist_content.size())
			continue; // we already have that one in hist_content

		if (di->type != TYPE_ITEM) {
			hist_content.push_back(tstring()); // not a text item, append empty string
			continue;
		}

		cl_item hi(di);
		hist_content.push_back(hi.get_textcontent());

		if (i == n) {
			// Usually only the most recent hisory items are of interest,
			// no need to iterate further if we have already found the requested n-th item
			return true;
		}
	}

	return false;
}

// Replace %CLIPBOARD% with the previous clipboard content
// %CLIPSEL% with the current selection and 
// all occurences of %CLIPHIST:n% with the content of the n-th newest item in history.
void expand_from_history(const HWND hWnd, tstring& s, tstring clipsel) {
	vector<tstring> hist_content;
	size_t p_start, p_end = 0;
	// Now replace all occurences of %CLIPBOARD% with the 2nd history item, the one with index 1.
	// The most recent history item with index 0 is the current selection.
	while ((p_start = s.find(TEXT("%CLIPBOARD%"), p_end)) != tstring::npos) {
		if (!read_from_history( hWnd, hist_content, 1))
			break; // could not read 1st history item, leave %CLIPBOARD% as it is
		tstring clipboard = hist_content[1];
		s = s.substr(0, p_start) + clipboard + s.substr(p_start + 11);
		p_end = p_start + clipboard.length() + 1;
	}

	p_start = p_end = 0;
	while ((p_start = s.find(TEXT("%CLIPSEL%"), p_end)) != tstring::npos) {
		s = s.substr(0, p_start) + clipsel + s.substr(p_start + 9);
		p_end = p_start + clipsel.length() + 1;
	}

	p_start = p_end = 0;
	while ((p_start = s.find(TEXT("%CLIPHIST:"), p_end)) != tstring::npos
		&& (p_end = s.find(TEXT("%"), p_start + 10)) != tstring::npos) {

		tstring n_str = s.substr(p_start + 10, p_end - p_start - 10);
		tstring repl;
		long n = 0;
		if (n_str.empty() || !is_number(n_str.c_str()) || (n = _tstol(n_str.c_str())) < 0) {
			p_end++;
			continue;
		}

		// is n not within hist_content?
		// Otherwise iterate to the n-th element of history and fill hist_content up to n, if possible.
		if (n >= (long)hist_content.size() && !read_from_history(hWnd, hist_content, n)) {
			p_end++;
			continue; // could not read n-th history item
		}

		repl = hist_content[n];
		s = s.substr(0, p_start) + repl + s.substr(p_end + 1);
		p_end = p_start + repl.length() + 1;
	}

	return;
}
