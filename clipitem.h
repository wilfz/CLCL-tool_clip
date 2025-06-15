#pragma once
/* Include Files */
#include "CLCLPlugin.h"
#include <vector>
#include <string>
#include <sqltypes.h>


const wchar_t format_unicode[] = L"UNICODE TEXT";
const wchar_t format_multibyte[] = L"TEXT";

class clip_item
{
public:
	clip_item() {
		itemtype = itemformat = op_modifiers = op_virtkey = op_paste = 0;
		modified = { 0,0,0,0, 0,0,0, 0 };
	};

	std::wstring title;
	long itemtype;
	long itemformat;
	std::wstring formatname;
	std::wstring textcontent;
	// even if vector is not as performant as list, it is easier to use
	// and we do not have to care about destroying the child items
	std::vector<clip_item> children;
	SYSTEMTIME modified;
	std::wstring windowname;
	UINT op_modifiers;
	UINT op_virtkey;
	int op_paste;

	int init(TOOL_DATA_INFO* tdi);
	int to_data_info(DATA_INFO* item, HWND hWnd);
	int merge_into(DATA_INFO* item, HWND hWnd);
};

class cl_mem
{
public:
	cl_mem(wchar_t*& p) {
		_pp = (void**)&p;
	}

	const wchar_t* operator = (const std::wstring& s);
	operator const std::wstring() const;

private:
	void** _pp;
};

// wrapper class for DATA_INFO
class cl_item
{
public:
	cl_item(DATA_INFO* &di) {
		_pi = di;
	};

	operator DATA_INFO* () {
		return _pi;
	}

	DATA_INFO* get_child() {
		return _pi ? _pi->child : nullptr;
	};

	DATA_INFO* get_next() {
		return _pi ? _pi->next : nullptr;
	};

	void append_child(DATA_INFO* di);
	DATA_INFO* find_format(UINT format) const;
	DATA_INFO* find_format(const wchar_t* format) const;

	// TYPE_FOLDER or TYPE_ITEM:
	void set_title(std::wstring s);
	inline std::wstring get_title() const {
		return _pi && _pi->title ? std::wstring(_pi->title) : std::wstring();
	};

	// All Types
	inline void set_itemtype(int itemtype) {
		if (_pi != nullptr)
			_pi->type = itemtype;
	};
	inline int get_itemtype() const {
		return _pi ? _pi->type : -1;
	};

	// TYPE_ITEM:
	void set_modified(SYSTEMTIME st); 
	SYSTEMTIME get_modified() const;

	void set_windowname(std::wstring s);
	inline std::wstring get_windowname() const {
		return _pi && _pi->window_name ? std::wstring(_pi->window_name) : std::wstring();
	};

	inline void set_op_modifiers(UINT mod) {
		if (_pi != nullptr)
			_pi->op_modifiers = mod;
	};
	inline UINT get_op_modifiers() const {
		return _pi ? _pi->op_modifiers : 0;
	};

	inline void set_op_virtkey(UINT vkey) {
		if (_pi != nullptr)
			_pi->op_virtkey = vkey;
	};
	inline UINT get_op_virtkey() const {
		return _pi ? _pi->op_virtkey : 0;
	};

	inline void set_op_paste(int paste) {
		if (_pi != nullptr)
			_pi->op_paste = paste;
	};
	inline int get_op_paste() const {
		return _pi ? _pi->op_paste : 0;
	};

	// TYPE_DATA:
	void set_formatname(std::wstring s);
	inline std::wstring get_formatname() const {
		return _pi && _pi->format_name ? std::wstring(_pi->format_name) : std::wstring();
	};

	// TYPE_ITEM or TYPE_DATA:
	// Only, if a data item with format CF_UNICODETEXT or CF_TEXT already exists!
	// Lacking access to the proper window handle hWnd it cannot be created here.
	void set_textcontent(std::wstring s);
	std::wstring get_textcontent() const;

	// properties:
	__declspec(property(get = get_title, put = set_title)) std::wstring title;
	__declspec(property(get = get_itemtype, put = set_itemtype)) int itemtype;
	__declspec(property(get = get_modified, put = set_modified)) SYSTEMTIME modified;
	__declspec(property(get = get_windowname, put = set_windowname)) std::wstring windowname;
	__declspec(property(get = get_op_modifiers, put = set_op_modifiers)) UINT op_modifiers;
	__declspec(property(get = get_op_virtkey, put = set_op_virtkey)) UINT op_virtkey;
	__declspec(property(get = get_op_paste, put = set_op_paste)) int op_paste;
	__declspec(property(get = get_formatname, put = set_formatname)) std::wstring formatname;
	__declspec(property(get = get_textcontent, put = set_textcontent)) std::wstring textcontent;

private:
	DATA_INFO* _pi;
};


// helper functions
bool FileTimeToSystemTimeCL(const FILETIME& ft, SYSTEMTIME& st);
bool SystemTimeToFileTimeCL(const SYSTEMTIME& st, FILETIME& ft);
bool FileTimeToTimestampStruct(const FILETIME ft, TIMESTAMP_STRUCT& ts);
bool TimestampStructToFileTime(const TIMESTAMP_STRUCT ts, FILETIME& ft);

// string (utf8) -> u16string -> wstring
std::wstring utf8_to_utf16(const std::string& utf8);

// wstring -> u16string -> string (utf8)
std::string utf16_to_utf8(const std::wstring& utf16);
