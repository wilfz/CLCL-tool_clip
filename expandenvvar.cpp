
/* Include Files */
#include "CLCLPlugin.h"
#include <string>
#include <cassert>
#include <tchar.h>
#include "string.h"

using namespace std;


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

		// just find out the size of memory to be allocated in the second call
		DWORD len = ::ExpandEnvironmentStrings(from_mem, NULL, 0);
		if (len <= 0) {
			GlobalUnlock(di->data);
			return TOOL_ERROR;
		}

		TCHAR* tmp = new TCHAR[len + 1];
		len = ::ExpandEnvironmentStrings(from_mem, tmp, len);
		if (len <= 0) {
			GlobalUnlock(di->data);
			return TOOL_ERROR;
		}

		tstring expanded(tmp);
		assert(len == expanded.length() + 1);
		delete[] tmp;

		// TODO: replace %DATE%, %TIME%, and %CLIPBOARD% with apprpriate strings
		// and copy expanded.c_str() to to_mem

		len = expanded.length() + 1; // include terminating null

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

