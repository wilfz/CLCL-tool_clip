
/* Include Files */
#include "CLCLPlugin.h"
#ifndef IDD_REPLACE_WHITESPACE
#include "resource.h"
#endif
#include "memory.h"
#include "clipitem.h"

#include <fstream>
#include "nlohmann/json.hpp"
using json = nlohmann::json;

using namespace std;

/* Global Variables */
extern HINSTANCE hInst;

void to_json(json& j, const clip_item& item) {
	// local UTF-8 interim variables
	string title, formatname, textcontent;
	tstring wts = linguversa::string_format(TEXT("%04d-%02d-%02d %02d:%02d:%02d.%09d"),
		item.modified.year,
		item.modified.month,
		item.modified.day,
		item.modified.hour,
		item.modified.minute,
		item.modified.second,
		item.modified.fraction);

	// the UTF-8 standard conversion facet
	title = utf16_to_utf8(item.title);
	formatname = utf16_to_utf8(item.formatname);
	textcontent = utf16_to_utf8(item.textcontent);
	string ts = utf16_to_utf8(wts);

	j = json{
		{"title", title},
		{"itemtype", item.itemtype },
		{"itemformat", item.itemformat},
		{"formatname", formatname},
		{"textcontent", textcontent },
		{"children", item.children },
		{"modified", ts }
	};
}

void from_json(const json& j, clip_item& item) {
	// local UTF-8 interim variables
	string title;
	string formatname;
	string textcontent;
	string ts;

	j.at("title").get_to(title);
	j.at("itemtype").get_to(item.itemtype);
	j.at("itemformat").get_to(item.itemformat);
	j.at("formatname").get_to(formatname);
	j.at("textcontent").get_to(textcontent);
	j.at("children").get_to(item.children);
	j.at("modified").get_to(ts);

	item.title = utf8_to_utf16(title);
	item.formatname = utf8_to_utf16(formatname);
	item.textcontent = utf8_to_utf16(textcontent);

	item.modified.year = atoi(ts.substr(0, 4).c_str());
	item.modified.month = atoi(ts.substr(5, 2).c_str());
	item.modified.day = atoi(ts.substr(8, 2).c_str());
	item.modified.hour = atoi(ts.substr(11, 2).c_str());
	item.modified.minute = atoi(ts.substr(14, 2).c_str());
	item.modified.second = atoi(ts.substr(17, 2).c_str());
	item.modified.fraction = atol(ts.substr(20, 9).c_str());
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

	clip_item ci;
	int ret = ci.init(tdi);
	if (ret != TOOL_SUCCEED)
		return ret;

	json jitem;
	to_json(jitem, ci);

	OPENFILENAME of;
	TCHAR file_name[MAX_PATH];

	try
	{
		ZeroMemory(&of, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hInstance = hInst;
		of.hwndOwner = hWnd;
		of.lpstrFilter = TEXT("*.json\0*.json\0\0");
		of.nFilterIndex = 1;
		lstrcpy(file_name, TEXT(".json\0"));
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
	TCHAR file_name[MAX_PATH];
	int ret = TOOL_SUCCEED;

	try
	{
		ZeroMemory(&of, sizeof(OPENFILENAME));
		of.lStructSize = sizeof(OPENFILENAME);
		of.hInstance = hInst;
		of.hwndOwner = hWnd;
		of.lpstrFilter = TEXT("*.json\0*.json\0\0");
		of.nFilterIndex = 1;
		lstrcpy(file_name, TEXT(".json\0"));
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

		ret = ci.to_data_info(tdi->di, hWnd);

		// Notify regist/template changes
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
