
/* Include Files */
#include "CLCLPlugin.h"
#ifndef IDD_REPLACE_WHITESPACE
#include "resource.h"
#endif
#include "memory.h"
#include "clipitem.h"
#include "query\lvstring.h"

#include <fstream>
#include "nlohmann/json.hpp"
using json = nlohmann::json;

using namespace std;

/* Global Variables */
extern HINSTANCE hInst;

void to_json(json& j, const clip_item& item) {
	// local UTF-8 interim variables
	string title, formatname, textcontent;
	wstring wts = linguversa::string_format(TEXT("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
		item.modified.wYear,
		item.modified.wMonth,
		item.modified.wDay,
		item.modified.wHour,
		item.modified.wMinute,
		item.modified.wSecond,
		item.modified.wMilliseconds);

	// the UTF-8 standard conversion facet
	title = utf16_to_utf8(item.title);
	formatname = utf16_to_utf8(item.formatname);
	textcontent = utf16_to_utf8(item.textcontent);
	string ts = utf16_to_utf8(wts);

	j = json{
		{"title", title},
		{"itemtype", item.itemtype },
		{"dataformat", item.itemformat},
		{"formatname", formatname},
		{"textcontent", textcontent },
		{"children", item.children },
		{"modified", ts },
		{"op_modifiers", item.op_modifiers},
		{"op_virtkey", item.op_virtkey},
		{"op_paste", item.op_paste}
	};
}

void from_json(const json& j, clip_item& item) {
	// Check if j is an array.
	// If so then create an empty item and load the array to children.
	if (j.is_array()) {
		item.itemtype = TYPE_ROOT;
		j.get_to(item.children);
		return;
	}

	// local UTF-8 interim variables
	string title;
	string formatname;
	string textcontent;
	string ts;

	j.at("title").get_to(title);
	j.at("itemtype").get_to(item.itemtype);
	j.at("dataformat").get_to(item.itemformat);
	j.at("formatname").get_to(formatname);
	j.at("textcontent").get_to(textcontent);
	j.at("children").get_to(item.children);
	j.at("modified").get_to(ts);
	j.at("op_modifiers").get_to(item.op_modifiers);
	j.at("op_virtkey").get_to(item.op_virtkey);
	j.at("op_paste").get_to(item.op_paste);

	item.title = utf8_to_utf16(title);
	item.formatname = utf8_to_utf16(formatname);
	item.textcontent = utf8_to_utf16(textcontent);

	item.modified.wYear = atoi(ts.substr(0, 4).c_str());
	item.modified.wMonth = atoi(ts.substr(5, 2).c_str());
	item.modified.wDay = atoi(ts.substr(8, 2).c_str());
	item.modified.wHour = atoi(ts.substr(11, 2).c_str());
	item.modified.wMinute = atoi(ts.substr(14, 2).c_str());
	item.modified.wSecond = atoi(ts.substr(17, 2).c_str());
	item.modified.wMilliseconds = atoi(ts.substr(20, 3).c_str());
}


void to_json(json& j, DATA_INFO* di) {
	cl_item item(di);
	// local UTF-8 interim variables
	string title, formatname, textcontent, windowname;
	UINT dataformat = 0;
	SYSTEMTIME st = item.get_modified();
	// convert SYSTEMTIME to a string
	string modify = utf16_to_utf8(
		linguversa::string_format(TEXT("%04d-%02d-%02d %02d:%02d:%02d.%03d"), 
			st.wYear,
			st.wMonth,
			st.wDay,
			st.wHour,
			st.wMinute,
			st.wSecond,
			st.wMilliseconds));

	// the UTF-8 standard conversion facet
	title = utf16_to_utf8(item.get_title());
	windowname = utf16_to_utf8(item.get_windowname());

	// if item is TYPE_ITEM, we try to get the text content
	if (di->type == TYPE_ITEM) {
		DATA_INFO* ti = item.find_format(CF_UNICODETEXT);
		if (ti == nullptr) {
			ti = item.find_format(CF_TEXT);
			// If format is CF_TEXT it is in locale multibyte encoded.
			// cl_item(ti).textcontent yields conversion to UTF-16,
			// which will in turn be converted to UTF-8.
			// Thus we have a transparent conversion from locale multi_byte to UTF-8.
		}
		if (ti != nullptr) {
			// convert UTF-16 to UTF-8
			textcontent = utf16_to_utf8(cl_item(ti).textcontent);
			dataformat = ti->format;
			formatname = utf16_to_utf8(cl_item(ti).formatname);
		}

		if (textcontent.empty()) {
			return;
		}
	}

	list<DATA_INFO*> children;
	if (di->type == TYPE_FOLDER || di->type == TYPE_ROOT) {
		for (DATA_INFO* cdi = item.get_child(); cdi != nullptr; cdi = cdi->next)
			children.push_back(cdi);
	}

	j = json{
		{"title", title },
		{"itemtype", di->type },
		{"dataformat", dataformat },
		{"formatname", formatname },
		{"windowname", windowname },
		{"textcontent", textcontent },
		{"children", children },
		{"modified", modify },
		//{"hkey_id", di->hkey_id },
		{"op_modifiers", di->op_modifiers },
		{"op_virtkey", di->op_virtkey },
		{"op_paste", di->op_paste }
	};
}

void to_json(json& j, TOOL_DATA_INFO* tdi) {
	// single item
	if (tdi->next == nullptr) {
		if (tdi->di != nullptr)
			to_json(j, tdi->di);
		return;
	}

	// several items
	list<DATA_INFO*> items;
	for (TOOL_DATA_INFO* t = tdi; t != NULL; t = t->next) {
		if (t->di == NULL)
			continue;

		items.push_back(t->di);
	}

	to_json(j, items);
}


/*
 * save_json
 */
__declspec(dllexport) int CALLBACK save_json(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	if (tdi == NULL) {
		return TOOL_SUCCEED;
	}

	if (tdi->di == NULL)
		return TOOL_ERROR;

	int ret = TOOL_SUCCEED;

	json jitem;
	to_json(jitem, tdi);

	OPENFILENAME of;
	wchar_t file_name[MAX_PATH];

	wstring fname = cl_item(tdi->di).title + L".json\0";
	if (fname.size() >= MAX_PATH)
		fname = L".json\0";
	lstrcpy(file_name, fname.c_str());

	try
	{
		ZeroMemory(&of, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hInstance = hInst;
		of.hwndOwner = hWnd;
		of.lpstrFilter = TEXT("*.json\0*.json\0\0");
		of.nFilterIndex = 1;
		of.lpstrFile = file_name;
		of.nMaxFile = MAX_PATH - 1;
		of.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

		if (GetSaveFileName(&of) == FALSE) {
			return TOOL_CANCEL;
		}

		// write prettified JSON to file
		std::ofstream o(of.lpstrFile);
		o << std::setw(2) << jitem << std::endl;
		o.close();

		return TOOL_SUCCEED;
	}
	catch (const std::exception& e)
	{
		string s = e.what();
		return TOOL_ERROR;
	}

	return ret;
}

/*
 * load_json
 */
__declspec(dllexport) int CALLBACK load_json(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	if (tdi == NULL) {
		return TOOL_SUCCEED;
	}

	if (tdi->di == NULL)
		return TOOL_ERROR;

	OPENFILENAME of;
	wchar_t file_name[MAX_PATH];
	int ret = TOOL_SUCCEED;

	lstrcpy(file_name, L".json\0");

	try
	{
		ZeroMemory(&of, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hInstance = hInst;
		of.hwndOwner = hWnd;
		of.lpstrFilter = L"*.json\0*.json\0\0";
		of.nFilterIndex = 1;
		of.lpstrFile = file_name;
		of.nMaxFile = MAX_PATH - 1;
		of.Flags = OFN_FILEMUSTEXIST;

		if (GetOpenFileName(&of) == FALSE) {
			return TOOL_CANCEL;
		}

		// load JSON from file
		std::ifstream f(of.lpstrFile);
		json jdata = json::parse(f);
		f.close();

		clip_item ci;

		from_json(jdata, ci);

		// Get registhistory_root
		DATA_INFO* history_root = nullptr;
		if (tdi->di->type == TYPE_ROOT 
			&& (history_root = (DATA_INFO*)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0))
			&& tdi->di == history_root)
		{
			if (ci.itemtype == TYPE_ITEM) {
				ret = ci.to_data_info(tdi->di, hWnd);
				SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
				return ret;
			}

			for (size_t i = 0; i < ci.children.size(); i++)
			{
				clip_item& child = ci.children[i];
				if (child.itemtype != TYPE_ITEM)
					continue;
				ret = child.to_data_info(tdi->di, hWnd);
				if (ret != TOOL_SUCCEED)
					break;
			}
			SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
			return ret;
		}

		ret = ci.to_data_info(tdi->di, hWnd);

		// Notify regist/template changes
		SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
		SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
		return ret;
	}
	catch (const std::exception& e)
	{
		string s = e.what();
		return TOOL_ERROR;
	}

	return TOOL_ERROR;
}
