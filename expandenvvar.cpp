/* Include Files */
#include "CLCLPlugin.h"
#include <string>
#include <cassert>
#include <tchar.h>
#include "string.h"

using namespace std;

// forward declarations of local functions and classes
void expand_macros(tstring& s);

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

	if (tdi == nullptr)
		return TOOL_ERROR;

	di = tdi->di;

	if (di == nullptr || di->type != TYPE_ITEM && di->type != TYPE_DATA)
		return TOOL_ERROR;

	// If command line is given get the macro to be expanded from there.
	if (tei->cmd_line != nullptr && *tei->cmd_line != TEXT('\0')) {
		// TODO: Open the template folder and look for a folder named like the command line, 
		// if it exists, select a macro from there and expand it.
		tstring folder(tei->cmd_line);
		tstring macro;

		// If no such folder exists, use command line itself as macro.
		macro.assign(tei->cmd_line);

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

		// Replace %DATE%, %TIME%, and %CLIPBOARD% with apprpriate strings
		expand_macros(expanded);

		len = expanded.length() + 1; // include terminating null

		// Copy expanded.c_str() to to_mem
		HANDLE hnd = NULL;
		// reserve memory for copy target
		if ((hnd = GlobalAlloc(GHND, sizeof(TCHAR) * len)) == NULL) {
			return TOOL_ERROR;
		}

		TCHAR* to_mem = nullptr;
		if ((to_mem = (TCHAR*)GlobalLock(hnd)) == NULL) {
			GlobalFree(hnd);
			return TOOL_ERROR;
		}

		_tcscpy_s(to_mem, len, expanded.c_str());

		GlobalUnlock(hnd);

		di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("UNICODE TEXT"));

		if (di == nullptr) {
			GlobalUnlock(to_mem);
			GlobalFree(to_mem);
			return TOOL_ERROR;
		}

		di->data = to_mem;
		di->size = len * sizeof(TCHAR); // include terminating null

		SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)di);

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
		if (tei->call_type == CALLTYPE_HISTORY || tei->call_type == CALLTYPE_REGIST) {
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

		// Replace %DATE%, %TIME%, and %CLIPBOARD% with apprpriate strings
		expand_macros(expanded);

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

// Replace %DATE%, %TIME%, and %CLIPBOARD% with apprpriate strings
void expand_macros(tstring& s)
{
	SYSTEMTIME current;
	GetLocalTime(&current);
	TCHAR date[BUF_SIZE];
	// Get the default format for date.
	int lDate = GetDateFormatEx(
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
		if (lDate > 0) {
			s = s.substr(0, p_start) + tstring(date) + s.substr(p_start + 6);
			//s.replace(p_start, 6, buf);
		}
	}

	// Replace all occurences of %DATE:{format}% with the formatted date. {format} must comply with 
	// https://learn.microsoft.com/en-us/windows/win32/intl/day--month--year--and-era-format-pictures
	while ((p_start = s.find(TEXT("%DATE:"))) != tstring::npos 
			&& (p_end = s.find(TEXT("%"), p_start + 6)) != tstring::npos) {

		tstring format = s.substr(p_start + 6, p_end - p_start - 6);
		lDate = GetDateFormatEx(
			LOCALE_NAME_USER_DEFAULT,
			0,
			&current,
			format.c_str(),
			date,
			BUF_SIZE,
			nullptr
		);

		if (lDate > 0) {
			s = s.substr(0, p_start) + tstring(date) + s.substr(p_end + 1);
		}
	}

	// TODO: Add similar replacement for %TIME% and %CLIPBOARD%
	// Replace all occurences of %TIME:{format}% with the formatted time. {format} must comply with 
	// https://learn.microsoft.com/en-us/windows/win32/api/datetimeapi/nf-datetimeapi-gettimeformatex

	return;
}
