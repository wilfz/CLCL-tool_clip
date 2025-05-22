
/* Include Files */
#include "CLCLPlugin.h"
#include "memory.h"

#include <vector>
#include "query\lvstring.h"

const TCHAR format_unicode[] = _T("UNICODE TEXT");
const TCHAR format_multibyte[] = _T("TEXT");

class clip_item
{
public:
	clip_item() {
		itemtype = itemformat = 0L;
		modified = { 0,0,0, 0,0,0, 0 };
	};

	std::tstring title;
	long itemtype;
	long itemformat;
	std::tstring formatname;
	std::tstring textcontent;
	std::vector<clip_item> children;
	TIMESTAMP_STRUCT modified;

	int init(TOOL_DATA_INFO* tdi);
	int to_data_info(DATA_INFO* item, HWND hWnd);
	int merge_into(DATA_INFO* item, HWND hWnd);
};

class cl_mem
{
public:
	cl_mem(TCHAR*& p) {
		_pp = (void**)&p;
	}

	const TCHAR* operator = (const std::tstring& s);

private:
	void** _pp;
};

// helper functions
bool FileTimeToTimestampStruct(const FILETIME ft, TIMESTAMP_STRUCT& ts);
bool TimestampStructToFileTime(const TIMESTAMP_STRUCT ts, FILETIME& ft);

// string (utf8) -> u16string -> wstring
std::wstring utf8_to_utf16(const std::string& utf8);

// wstring -> u16string -> string (utf8)
std::string utf16_to_utf8(const std::wstring& utf16);
