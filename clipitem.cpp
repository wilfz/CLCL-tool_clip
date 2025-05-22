
#include "clipitem.h"
#include <codecvt>

using namespace std;

int clip_item::init(TOOL_DATA_INFO* tdi)
{
	TOOL_DATA_INFO* wk_tdi = NULL;

	if (tdi == nullptr || tdi->di == nullptr)
		return TOOL_ERROR;
	// convert di->modified (FILETIME) to TIMESTAMP_STRUCT
	TIMESTAMP_STRUCT ts = { 0,0,0, 0,0,0, 0 };
	bool withTS = FileTimeToTimestampStruct(tdi->di->modified, modified);
	title.assign(tdi->di->title ? tdi->di->title : _T(""));
	itemtype = tdi->di->type;

	if (tdi->di->type == TYPE_FOLDER || tdi->di->type == TYPE_ROOT) {
		for (wk_tdi = tdi->child; wk_tdi != NULL; wk_tdi = wk_tdi->next) {
			// init all child items of folder (recursion)
			size_t n = children.size();
			if (n == children.capacity())
				children.reserve(n + 10);
			children.resize(n + 1);
			children[n].init(wk_tdi);
		}
		children.shrink_to_fit();
		return TOOL_SUCCEED;
	}

	if (tdi->di->type == TYPE_ITEM) {
		bool bFound = false;
		for (wk_tdi = tdi->child; wk_tdi != NULL; wk_tdi = wk_tdi->next)
			// for the moment we save only odbc_save_unicode;
			if (wk_tdi->di && wk_tdi->di->format == CF_UNICODETEXT) {
				bFound = true;
				break;
			}
		if (!bFound) {
			for (wk_tdi = tdi->child; wk_tdi != NULL; wk_tdi = wk_tdi->next)
				// for the moment we save only odbc_save_unicode;
				if (wk_tdi->di && wk_tdi->di->format == CF_TEXT) {
					break;
				}
		}

		// neither CF_UNICODETEXT nor CF_TEXT found => nothing to do
		if (wk_tdi == NULL) {
			return TOOL_SUCCEED;
		}
	}

	if (!wk_tdi || !wk_tdi->di) {
		return TOOL_ERROR;
	}

	DATA_INFO& di = *wk_tdi->di;
	wstring wsdata;
	string mbdata;
	if (!di.format_name) {
		return TOOL_ERROR;
	}
	else if (di.format == CF_UNICODETEXT) {
		wchar_t* ws = nullptr;
		if ((ws = (wchar_t*)GlobalLock(di.data)) == NULL) {
			return TOOL_ERROR;
		}
		wsdata.assign(ws);
		GlobalUnlock(di.data);
	}
	else if (di.format == CF_TEXT) {
		char* s = nullptr;
		if ((s = (char*)GlobalLock(di.data)) == NULL) {
			return TOOL_ERROR;
		}
		mbdata.assign(s);
		int cnt;
		int len = strlen(s) + 1;
		wchar_t* ws = new wchar_t[len];
		cnt = ::MultiByteToWideChar(CP_THREAD_ACP, 0, s, len, ws, len);
		if (cnt <= 0) {	// error
			delete[] ws;
			return TOOL_ERROR;
		}

		wsdata.assign(ws);
		delete[] ws;
		GlobalUnlock(di.data);
	}

	itemtype = (long)di.type;
	itemformat = (long)di.format;
	formatname.assign(di.format_name ? di.format_name : _T(""));
	textcontent = wsdata;

	return TOOL_SUCCEED;
}

int clip_item::to_data_info(DATA_INFO* item, HWND hWnd)
{
	if (item == nullptr)
		return TOOL_ERROR;

	int ret = TOOL_SUCCEED;

	// both are folder and have the same name
	if (this->itemtype == TYPE_FOLDER && item->type == TYPE_FOLDER 
		&& _tcsicmp(this->title.c_str(), item->title) == 0)
	{
		ret = merge_into( item, hWnd);
		return ret;
	}

	// item is folder with different name
	if (item->type == TYPE_FOLDER) {
		DATA_INFO* lastchild = nullptr;
		for (DATA_INFO* cdi = item->child; cdi != nullptr; cdi = cdi->next) {
			// this already exists in folder?
			if (this->itemtype == TYPE_DATA && cdi->type ==TYPE_ITEM && _tcsicmp(this->title.c_str(), cdi->title) == 0) {
				return to_data_info(cdi, hWnd);
			}

			lastchild = cdi;
		}

		// nothing found? Then create an new child item and apend at the end:
		DATA_INFO* cdi = nullptr;
		switch (this->itemtype)
		{
		case TYPE_DATA:
			// create an item ...
			if ((cdi = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_ITEM, (LPARAM)this->title.c_str())) == NULL) {
				return TOOL_ERROR;
			}
			// initialize item's modified time to 0 (otherwise it's current time)
			cdi->modified.dwHighDateTime = cdi->modified.dwLowDateTime = 0;
			cdi->child = nullptr;
			if (this->itemformat == CF_TEXT && (cdi->child = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("TEXT"))) == NULL) {
				SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)cdi);
				return TOOL_ERROR;
			}
			else if (this->itemformat == CF_UNICODETEXT && (cdi->child = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("UNICODE TEXT"))) == NULL) {
				SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)cdi);
				return TOOL_ERROR;
			}

			if (cdi->child)
				cdi->child->next = nullptr;
			break;

		case TYPE_FOLDER:
			// create a folder ...
			if ((cdi = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_FOLDER, (LPARAM)this->title.c_str())) == NULL) {
				return TOOL_ERROR;
			}
			break;

		default:
			// we should never come here
			return TOOL_ERROR;
		}

		if (lastchild == nullptr)
			item->child = cdi; // first and only child
		else
			lastchild->next = cdi; // append at the end of the list

		cdi->next = nullptr;

		return to_data_info(cdi, hWnd);
	}

	if (this->itemtype != TYPE_DATA || item->type != TYPE_ITEM) {
		return TOOL_ERROR;
	}

	FILETIME dbft;
	long cmp = 0;
	if (!TimestampStructToFileTime(this->modified, dbft)) {
		return TOOL_ERROR;
	}
		
	// dbft older than item->modified ?
	if ((cmp = ::CompareFileTime(&dbft, &(item->modified))) <= 0)
	{
		// nothing to do, item is newer
		return TOOL_SUCCEED;
	}

	// this->modified is newer than item->modified
	cl_mem(item->title) = this->title;

	DATA_INFO* pd = nullptr;
	// search for data_item with matching title and UNICODE format
	// delete all other formats
	for (DATA_INFO* p = item->child; p != nullptr; p = p->next) {
		if (p->type == TYPE_DATA && p->format == CF_UNICODETEXT) {
			pd = p;
			cl_mem(pd->format_name) = this->formatname;
		}
		else {
			SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)p);
		}
	}

	item->child = pd;

	if (pd == nullptr) {
		// no data_item found, so we create a new one
		switch (this->itemformat) {
		case CF_UNICODETEXT:
			if ((pd = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("UNICODE TEXT"))) == NULL)
				return TOOL_ERROR;
			pd->format = CF_UNICODETEXT;
			break;
		case CF_TEXT:
			if ((pd = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("TEXT"))) == NULL)
				return TOOL_ERROR;
			pd->format = CF_TEXT;
			break;
		default:
			if ((pd = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)(this->formatname.c_str()))) == NULL)
				return TOOL_ERROR;
		}

		pd->data = nullptr;
		pd->size = 0;
		pd->next = nullptr;
		item->child = pd;
	}

	string mbdata;
	wstring wsdata;
	size_t len = 0;

	switch (this->itemformat) 
	{
		case CF_UNICODETEXT:
		{
			// allocate (this->textcontent.size() + 1) * sizeof(TCHAR) bytes in pd->data and
			// copy this->textcontent to the allocated memory
			HGLOBAL ret = NULL;
			TCHAR* to_mem = nullptr;
			size_t targetlen = this->textcontent.size();
			// reserve memory for copy target
			if ((ret = GlobalAlloc(GHND, (targetlen + 1) * sizeof(TCHAR))) == NULL) {
				return TOOL_ERROR;
			}
			// lock copy target
			if ((to_mem = (TCHAR*)GlobalLock(ret)) == NULL) {
				GlobalFree(ret);
				return TOOL_ERROR;
			}

			// copying to target
			for (size_t i = 0; i < targetlen; i++) {
				to_mem[i] = this->textcontent[i];
			}
			to_mem[targetlen] = L'\0';

			GlobalUnlock(ret);
			if (pd->data != nullptr)
				GlobalFree(pd->data);
			pd->data = ret;
			pd->size = (targetlen + 1) * sizeof(TCHAR);
		}
		break;

		case CF_TEXT:
		{
			// first convert to multibyte mb
			int len = this->textcontent.length() + 1;
			char* mb = new char[len * 4];
			BOOL bUsedDefaultChar = FALSE;
			int cnt = ::WideCharToMultiByte(CP_THREAD_ACP, 0, this->textcontent.c_str(), 
				len, mb, len * 4, NULL, &bUsedDefaultChar);
		
			if (cnt <= 0 || cnt != strlen(mb) + 1) {
				delete[] mb;
				return TOOL_ERROR;
			}

			// then allocate data at to_mem
			HGLOBAL ret = NULL;
			char* to_mem = nullptr;
			size_t targetlen = strlen(mb);
			// reserve memory for copy target
			if ((ret = GlobalAlloc(GHND, targetlen + 1)) == NULL) {
				delete[] mb;
				return TOOL_ERROR;
			}
			// lock copy target
			if ((to_mem = (char*)GlobalLock(ret)) == NULL) {
				delete[] mb;
				GlobalFree(ret);
				return TOOL_ERROR;
			}

			// copying to target
			for (size_t i = 0; i < targetlen; i++) {
				to_mem[i] = mb[i];
			}
			to_mem[targetlen] = '\0';

			delete[] mb;
			GlobalUnlock(ret);
			if (pd->data != nullptr)
				GlobalFree(pd->data);
			pd->data = ret;
			pd->size = targetlen + 1;
		}
		break;

	}

	TimestampStructToFileTime(this->modified, item->modified);

	return TOOL_SUCCEED;
}

int clip_item::merge_into(DATA_INFO* item, HWND hWnd)
{
	for (size_t i = 0; i < children.size(); i++) {
		clip_item& child = children[i];
		int ret = child.to_data_info(item, hWnd);
		if (ret != TOOL_SUCCEED) {
			return ret;
		}
	}
	return TOOL_SUCCEED;
}

const TCHAR* cl_mem::operator=(const std::tstring& s)
{
	// if strings are equal, return the old pointer
	if (*_pp && _tcscmp((TCHAR*)*_pp, s.c_str()) == 0)
		return (TCHAR*)*_pp;

	mem_free((void**)_pp);
	*_pp = alloc_copy(s.c_str());
	return (TCHAR*)*_pp;
}

bool FileTimeToTimestampStruct(const FILETIME ft, TIMESTAMP_STRUCT& ts)
{
	SYSTEMTIME st;
	if (ft.dwHighDateTime != 0 && ft.dwLowDateTime != 0
		&& FileTimeToSystemTime(&ft, &st))
	{
		ts.year = st.wYear;
		ts.month = st.wMonth;
		ts.day = st.wDay;
		ts.hour = st.wHour;
		ts.minute = st.wMinute;
		ts.second = st.wSecond;
		ts.fraction = st.wMilliseconds * 1000000; // ts.fraction is nanoseconds
		return true;
	}

	ts = { 0,0,0, 0,0,0, 0 };
	return false;
}

bool TimestampStructToFileTime(const TIMESTAMP_STRUCT ts, FILETIME& ft)
{
	SYSTEMTIME st;
	if (ts.year > 0 && ts.month > 0 && ts.day > 0) {
		st.wYear = ts.year;
		st.wMonth = ts.month;
		st.wDay = ts.day;
		st.wHour = ts.hour;
		st.wMinute = ts.minute;
		st.wSecond = ts.second;
		st.wMilliseconds = (WORD)(ts.fraction / 1000000); // ts.fraction is nanoseconds
		if (SystemTimeToFileTime(&st, &ft))
			return true;
	}

	ft.dwHighDateTime = ft.dwLowDateTime = 0;
	return false;
}

// The following 2 functions are from
// https://gist.github.com/gchudnov/c1ba72d45e394180e22f

// string (utf8) -> u16string -> wstring
wstring utf8_to_utf16(const string& utf8)
{
	wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> convert;
	u16string utf16 = convert.from_bytes(utf8);
	wstring wstr(utf16.begin(), utf16.end());
	return wstr;
}

// wstring -> u16string -> string (utf8)
string utf16_to_utf8(const wstring& utf16) {
	u16string u16str(utf16.begin(), utf16.end());
	wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> convert;
	string utf8 = convert.to_bytes(u16str);
	return utf8;
}
