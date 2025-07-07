
#include <codecvt>
#include <cassert>
#include "clipitem.h"
#include "memory.h"


using namespace std;

int clip_item::init(TOOL_DATA_INFO* tdi)
{
	TOOL_DATA_INFO* wk_tdi = NULL;

	if (tdi == nullptr || tdi->di == nullptr)
		return TOOL_ERROR;
	// convert di->modified (FILETIME) to TIMESTAMP_STRUCT
	SYSTEMTIME ts = { 0,0,0,0, 0,0,0, 0 };
	bool withTS = ::FileTimeToSystemTimeCL(tdi->di->modified, modified);
	title.assign(tdi->di->title ? tdi->di->title : L"");
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
			if (wk_tdi->di && wk_tdi->di->format == CF_UNICODETEXT) {
				bFound = true;
				break;
			}
		if (!bFound) {
			for (wk_tdi = tdi->child; wk_tdi != NULL; wk_tdi = wk_tdi->next)
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
	formatname.assign(di.format_name ? di.format_name : L"");
	textcontent = wsdata;
	op_modifiers = di.op_modifiers;
	op_virtkey = di.op_virtkey;
	op_paste = di.op_paste;

	return TOOL_SUCCEED;
}

int clip_item::to_data_info(DATA_INFO* item, HWND hWnd)
{
	if (item == nullptr)
		return TOOL_ERROR;

	int ret = TOOL_SUCCEED;

	// both are folder and have the same name
	if (this->itemtype == TYPE_FOLDER && item->type == TYPE_FOLDER 
			&& item->title != nullptr && _wcsicmp(this->title.c_str(), item->title) == 0
		|| this->itemtype == TYPE_ROOT && (item->type == TYPE_FOLDER || item->type == TYPE_ROOT))
	{
		ret = merge_into( item, hWnd);
		return ret;
	}

	// item is folder with different name
	if (item->type == TYPE_FOLDER || item->type == TYPE_ROOT) {
		DATA_INFO* lastchild = nullptr;
		for (DATA_INFO* cdi = item->child; cdi != nullptr; cdi = cdi->next) {
			wstring title = cl_item(cdi).get_title();
			if (this->itemtype == TYPE_FOLDER && cdi->type == TYPE_FOLDER 
				&& title.size() > 0 && title == this->title) 
			{
				ret = merge_into(cdi, hWnd);
				return ret;
			}

			if (cdi->type ==TYPE_ITEM || (this->itemtype == TYPE_DATA || this->itemtype == TYPE_ITEM)) {
				// this already exists
				// with same name in folder?
				if (title.size() > 0 && title == this->title) {
					return to_data_info(cdi, hWnd);
				}
				else {
					wstring wsdata = cl_item(cdi).textcontent;
					// or with same content
					if (wsdata.size() > 0 && this->textcontent == wsdata)
						return to_data_info(cdi, hWnd);
				}
			}

			lastchild = cdi;
		}

		// nothing found? Then create an new child item and apend at the end:
		DATA_INFO* cdi = nullptr;
		switch (this->itemtype)
		{
		case TYPE_ITEM:
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

	if (item->type != TYPE_ITEM || (this->itemtype != TYPE_ITEM && this->itemtype != TYPE_DATA)) {
		return TOOL_ERROR;
	}

	FILETIME dbft;
	long cmp = 0;
	if (!::SystemTimeToFileTimeCL(this->modified, dbft)) {
		SYSTEMTIME ts = { 2000,01,0,01, 00,00,00, 0} ;
		if (!::SystemTimeToFileTimeCL(ts, dbft))
			return TOOL_ERROR;
	}
		
	// dbft older than item->modified ?
	if ((cmp = ::CompareFileTime(&dbft, &(item->modified))) <= 0)
	{
		// nothing to do, item is newer
		return TOOL_SUCCEED;
	}

	// this->modified is newer than item->modified
	if (cl_item(item).textcontent == this->textcontent)
		return TOOL_SUCCEED; // nothing to do
	cl_mem(item->title) = this->title;

	// TODO: check if the same combination of op_modifiers and op_virtkey already exists.
	DATA_INFO* hotkey_item = nullptr;
	if (this->op_virtkey) {
		hotkey_item = find_hotkey_item((DATA_INFO*) ::SendMessage(hWnd, WM_REGIST_GET_ROOT, 0, 0),
			this->op_virtkey, this->op_modifiers);
	}
	// If not found, set the hotkey to the current item
	if (hotkey_item == nullptr)
	{
		item->op_modifiers = this->op_modifiers;
		item->op_virtkey = this->op_virtkey;
	}
	item->op_paste = this->op_paste;

	DATA_INFO* pd = nullptr;
	// search for data_item with matching title and UNICODE or CF_TEXT format
	// delete all other formats
	for (DATA_INFO* p = item->child; p != nullptr; p = p->next) {
		if (pd == nullptr && p->type == TYPE_DATA && p->format == CF_UNICODETEXT) {
			pd = p;
		}
		else if (pd == nullptr && p->type == TYPE_DATA && p->format == CF_TEXT) {
			pd = p;
		}
		else {
			SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)p);
		}
	}

	item->child = pd;
	if (pd != nullptr)
		pd->next = nullptr;

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
			// NYI
			//if ((pd = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)(this->formatname.c_str()))) == NULL)
				return TOOL_ERROR;
		}

		pd->data = nullptr;
		pd->size = 0;
		pd->next = nullptr;
		item->child = pd;
	}

	cl_item(pd).set_textcontent(this->textcontent);

	::SystemTimeToFileTimeCL(this->modified, item->modified);

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

DATA_INFO* clip_item::find_hotkey_item(DATA_INFO* di, UINT virtkey, UINT modifier)
{
	if (di == nullptr || di->type == TYPE_DATA || virtkey == 0) {
		return nullptr; // no hotkey item in data
	}
	for (DATA_INFO* cdi = di->child; cdi != nullptr; cdi = cdi->next) {
		if (cdi->type == TYPE_ITEM && cdi->op_virtkey == virtkey && cdi->op_modifiers == modifier) {
			return cdi;
		}
		if (cdi->type == TYPE_FOLDER || cdi->type == TYPE_ROOT) {
			DATA_INFO* p = find_hotkey_item(cdi, virtkey, modifier);
			if (p != nullptr) {
				return p;
			}
		}
	}

	return nullptr;
}

const wchar_t* cl_mem::operator=(const std::wstring& s)
{
	if (s.size() == 0) {
		mem_free((void**)_pp);
		*_pp = nullptr;
		return (const wchar_t*)*_pp;
	}

	// if strings are equal, return the old pointer
	if (*_pp && wcscmp((wchar_t*)*_pp, s.c_str()) == 0)
		return (const wchar_t*)*_pp;

	mem_free((void**)_pp);
	*_pp = alloc_copy(s.c_str());
	return (const wchar_t*)*_pp;
}

cl_mem::operator const std::wstring() const
{
	wstring s;
	if (*_pp)
		s.assign((wchar_t*)*_pp);
	return s;
}

void cl_item::append_child(DATA_INFO* di) 
{
	assert(_pi);
	if (_pi->child == nullptr) {
		_pi->child = di;
		return;
	}

	DATA_INFO* wk_di = _pi->child; 
	while (wk_di->next != nullptr)
		wk_di = wk_di->next;

	wk_di->next = di;
}

DATA_INFO* cl_item::find_format(UINT format) const
{
	if (_pi && _pi->type == TYPE_DATA && _pi->format == format)
		return _pi;

	if (_pi && _pi->type == TYPE_ITEM) {
		for (DATA_INFO* di = _pi->child; di != nullptr; di = di->next)
			if (di->format == format)
				return di;
	}

	return nullptr;
}

DATA_INFO* cl_item::find_format(const wchar_t* format) const
{
	if (format == nullptr)
		return nullptr;

	if (_pi && _pi->type == TYPE_DATA && _pi->format_name && _wcsicmp(_pi->format_name, format) == 0)
		return _pi;

	if (_pi && _pi->type == TYPE_ITEM) {
		for (DATA_INFO* di = _pi->child; di != nullptr; di = di->next)
			if (di->format_name && _wcsicmp(di->format_name, format) == 0)
				return di;
	}

	return nullptr;
}

void cl_item::set_title(std::wstring s)
{
	assert(_pi);
	assert(_pi->type == TYPE_ITEM || _pi->type == TYPE_FOLDER || _pi->type == TYPE_ROOT);
	if (_pi != nullptr || _pi->type != TYPE_ITEM && _pi->type != TYPE_FOLDER && _pi->type != TYPE_ROOT)
		return;
	cl_mem(_pi->title) = s;
}

void cl_item::set_modified(SYSTEMTIME st)
{
	assert(_pi);
	assert(_pi->type == TYPE_ITEM);
	if (_pi) {
		::SystemTimeToFileTimeCL(st, _pi->modified);
	}
}

SYSTEMTIME cl_item::get_modified() const
{
	SYSTEMTIME st = { 0,0,0,0, 0,0,0, 0 };
	if (_pi && _pi->type == TYPE_ITEM && ::FileTimeToSystemTimeCL(_pi->modified, st))
		return st;

	st = { 0,0,0,0, 0,0,0, 0 };
	return st;
}

void cl_item::set_windowname(std::wstring s)
{
	if (_pi && _pi->type == TYPE_ITEM) {
		cl_mem(_pi->window_name) = s;
	}
}

void cl_item::set_formatname(std::wstring s)
{
	if (_pi && _pi->type == TYPE_DATA) {
		cl_mem(_pi->format_name) = s;
	}
}

void cl_item::set_textcontent(std::wstring str)
{
	DATA_INFO* pd = find_format(CF_UNICODETEXT);
	if (pd == nullptr) {
		pd = find_format(CF_TEXT);
	}

	assert(pd);

	switch (pd->format)
	{
	case CF_UNICODETEXT:
	{
		// allocate (str.size() + 1) * sizeof(wchar_t) bytes in pd->data and
		// copy this->textcontent to the allocated memory
		HGLOBAL ret = NULL;
		wchar_t* to_mem = nullptr;
		size_t targetlen = str.size();
		// reserve memory for copy target
		if ((ret = GlobalAlloc(GHND, (targetlen + 1) * sizeof(wchar_t))) == NULL) {
			assert(false);
			return;
		}
		// lock copy target
		if ((to_mem = (wchar_t*)GlobalLock(ret)) == NULL) {
			GlobalFree(ret);
			assert(false);
			return;
		}

		// copying to target
		for (size_t i = 0; i < targetlen; i++) {
			to_mem[i] = str[i];
		}
		to_mem[targetlen] = L'\0';

		GlobalUnlock(ret);
		if (pd->data != nullptr)
			GlobalFree(pd->data);
		pd->data = ret;
		pd->size = (targetlen + 1) * sizeof(wchar_t);
		cl_mem(pd->format_name) = format_unicode;
	}
	break;

	case CF_TEXT:
	{
		// first convert to multibyte mb
		int len = str.length() + 1;
		char* mb = new char[len * 4];
		BOOL bUsedDefaultChar = FALSE;
		int cnt = ::WideCharToMultiByte(CP_THREAD_ACP, 0, str.c_str(),
			len, mb, len * 4, NULL, &bUsedDefaultChar);

		if (cnt <= 0 || cnt != strlen(mb) + 1) {
			delete[] mb;
			assert(false);
			return;
		}

		// then allocate data at to_mem
		HGLOBAL ret = NULL;
		char* to_mem = nullptr;
		size_t targetlen = strlen(mb);
		// reserve memory for copy target
		if ((ret = GlobalAlloc(GHND, targetlen + 1)) == NULL) {
			assert(false);
			return;
		}
		// lock copy target
		if ((to_mem = (char*)GlobalLock(ret)) == NULL) {
			delete[] mb;
			GlobalFree(ret);
			assert(false);
			return;
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
		cl_mem(pd->format_name) = format_multibyte;
	}
	break;

	}

	return;
}

std::wstring cl_item::get_textcontent() const
{
	wstring wsdata;
	DATA_INFO* di = find_format(CF_UNICODETEXT);
	if (di != nullptr) {
		wchar_t* ws = nullptr;
		if ((ws = (wchar_t*)GlobalLock(di->data)) == NULL) {
			return wsdata;
		}
		wsdata.assign(ws);
		GlobalUnlock(di->data);
		return wsdata;
	}

	di = find_format(CF_TEXT);
	if (di != nullptr) {
		char* s = nullptr;
		if ((s = (char*)GlobalLock(di->data)) == NULL) {
			return wsdata;
		}
		int cnt;
		int len = strlen(s) + 1;
		wchar_t* ws = new wchar_t[len];
		cnt = ::MultiByteToWideChar(CP_THREAD_ACP, 0, s, len, ws, len);
		if (cnt <= 0) {	// error
			delete[] ws;
			GlobalUnlock(di->data);
			return wsdata;
		}

		wsdata.assign(ws);
		delete[] ws;
		GlobalUnlock(di->data);
	}

	return wsdata;
}

bool FileTimeToSystemTimeCL(const FILETIME& ft, SYSTEMTIME& st)
{
	if (ft.dwHighDateTime != 0 && ft.dwLowDateTime != 0
		&& ::FileTimeToSystemTime(&ft, &st))
	{
		return true;
	}

	st = { 0,0,0,0, 0,0,0, 0 };
	return false;
}

bool SystemTimeToFileTimeCL(const SYSTEMTIME& st , FILETIME& ft)
{
	if (st.wYear >= 1900 && st.wMonth > 0 && st.wDay > 0) {
		if (SystemTimeToFileTime(&st, &ft))
			return true;
	}

	ft.dwHighDateTime = ft.dwLowDateTime = 0;
	return false;
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

