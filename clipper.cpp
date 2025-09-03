#include <fstream>
#include "clipper.h"
#include "clipitem.h"
#include "nlohmann/json.hpp"
#include "streamindent.h"

using json = nlohmann::json;
using namespace wilfz;
using namespace std;

extern HINSTANCE hInst;
extern wstring ini_path;
extern wstring clipper_history;
extern string default_list_name;

template<typename T>
void to_json(json& j, const list<std::shared_ptr<T>>& l) {
	j = json::array();
	for (const std::shared_ptr<T>& it : l) 	{
		j.emplace_back(it);
	}
}

template<typename T>
void from_json(const json& j, list<std::shared_ptr<T>>& l) {
	l.clear();
	for (const json& it : j) {
		std::shared_ptr<T> item = std::make_shared<T>();
		from_json(it, item);
		l.push_back(item);
	}
}

static string json_escape(const string& src) {
	string target = src;
	if (src.empty())
		return target;
	string from = "\\";
	string to = "\\\\";
	size_t start_pos = 0;
	while ((start_pos = target.find(from, start_pos)) != string::npos)
	{
		target.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

	from = "\"";
	to = "\\\"";
	start_pos = 0;
	while ((start_pos = target.find(from, start_pos)) != string::npos)
	{
		target.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

	from = "\n";
	to = "\\n";
	start_pos = 0;
	while ((start_pos = target.find(from, start_pos)) != string::npos)
	{
		target.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

	from = "\r";
	to = "\\r";
	start_pos = 0;
	while ((start_pos = target.find(from, start_pos)) != string::npos)
	{
		target.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

	from = "\t";
	to = "\\t";
	start_pos = 0;
	while ((start_pos = target.find(from, start_pos)) != string::npos)
	{
		target.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}

	return target;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const list<std::shared_ptr<T>>& l) {
	if (l.empty()) {
		os << "[]";
		return os;
	}
	os << "[\n" << change_indent(+1);
	bool first = true;
	for (const shared_ptr<T>& item : l) {
		if (first)
			first = false;
		else
			os << ",\n";
		
		os << indent << *item;
	}
	os << "\n" << change_indent(-1) 
		<< indent << "]";
	return os;
}

/* Global Variables */
extern HINSTANCE hInst;

clipper_item::clipper_item()
{
	pinned = false;
	position = 0;
	timestamp_ms = 0;
	timestamp = (time_t)0;
}

clipper_item::clipper_item(DATA_INFO* di)
{
	cl_item cli(di);
	title = ::utf16_to_utf8(cli.title);
	contents = ::utf16_to_utf8(cli.textcontent);
	pinned = false;
	position = 0;
	timestamp = ::SystemTimeToEpochSeconds(cli.modified);
	timestamp_ms = (long long)timestamp > 0 ?
		((long long)timestamp * 1000 + cli.modified.wMilliseconds) : 0;
}

std::ostream& operator<<(std::ostream& os, const clipper_item& ci)
{
	os << "{\n" << change_indent(+1);
	if (!ci.title.empty())
		os << indent << "\"title\": \"" << json_escape(ci.title) << "\",\n";
	os << indent << "\"contents\": \"" << json_escape(ci.contents) << "\",\n"
		<< indent << "\"pinned\": " << (ci.pinned ? "true" : "false") << ",\n"
		<< indent << "\"position\": " << ci.position << ",\n"
		<< indent << "\"timestamp_ms\": " << ci.timestamp_ms << ",\n"
		<< indent << "\"timestamp\": " << ci.timestamp << "\n"
	    << change_indent(-1) 
		<< indent << "}";
	return os;
}

std::ostream& operator<<(std::ostream& os, const clipper_list& cl)
{
	os << "{\n" << change_indent(+1)
		<< indent << "\"name\": \"" << json_escape(cl.name) << "\",\n"
		<< indent << "\"position\": " << cl.position << ",\n"
		<< indent << "\"clipboard\": " << (cl.clipboard ? "true" : "false") << ",\n"
		<< indent << "\"clippings\": " << cl.clippings << "\n"
		<< change_indent(-1)
		<< indent << "}";
	return os;
}

std::ostream& operator<<(std::ostream& os, const clipper_info& i)
{
	os << "{\n" << change_indent(+1)
		<< indent << "\"platform\": \"" << i.platform << "\",\n"
		<< indent << "\"platform_version\": " << i.platform_version << ",\n"
		<< indent << "\"package\": \"" << i.package << "\",\n"
		<< indent << "\"application_version_code\": " << i.application_version_code << ",\n"
		<< indent << "\"export_version\": " << i.export_version << "\n"
		<< change_indent(-1)
		<< indent << "}";
	return os;
}

std::ostream& operator<<(std::ostream& os, const clipper_bkp& cb)
{
	os << "{\n" << change_indent(+1)
		<< indent << "\"info\": " << cb.info << ",\n"
		<< indent << "\"lists\": " << cb.lists << "\n"
		<< change_indent(-1)
		<< indent << "}";
	return os;
}

DATA_INFO* clipper_bkp::find_clipboard(DATA_INFO* di) const
{
	if (di == nullptr) {
		return nullptr; // no DATA_INFO provided
	}

	if (di->type == TYPE_FOLDER && clipper_history == di->title) {
		return di; // found the clipboard folder
	}

	for (DATA_INFO* child = di->child; child != nullptr; child = child->next) {
		if (child->type == TYPE_FOLDER && child->title == clipper_history) {
			return child; // found the clipboard folder in children
		}
	}

	return nullptr;
}

void clipper_bkp::add_data_info(DATA_INFO* di, string parentname)
{
	shared_ptr<clipper_list> cl = nullptr;
	unsigned int anzpos = lists.size();
	if (anzpos == 0) {
		// First list is always the clipboard list which is created on first call.
		// All following items of TYPE_ITEM will be added to this list
		cl = make_shared<clipper_list>();
		cl->name = "Clipboard";
		cl->position = anzpos++;
		cl->clipboard = true; // clipboard
		lists.push_back(cl);
	}

	if (di && di->type == TYPE_FOLDER && clipper_history == di->title) {
		cl = lists.front();
	}
	else if (di && di->type == TYPE_FOLDER) {
		// create a new list for the folder
		cl = make_shared<clipper_list>();
		cl->name = parentname.empty() 
			? ::utf16_to_utf8(cl_mem(di->title)) 
			: parentname + "/" + ::utf16_to_utf8(cl_mem(di->title));
		cl->position = anzpos++;
		cl->clipboard = false;
		lists.push_back(cl);
	}
	else if (di && di->type == TYPE_ITEM) {
		if (cl_item(di).textcontent == L"")
			return;
		cl = get_default_list();
		cl->clippings.push_back(make_shared<clipper_item>(di));
		return; // no children for TYPE_ITEM
	}

	if (di == nullptr || di->type < TYPE_FOLDER || di->type > TYPE_ROOT) {
		return; // no DATA_INFO, nothing more to do
	}

	if (cl == nullptr) {
		return; // no DATA_INFO, nothing more to do
	}

	// di is TYPE_FOLDER or TYPE_ROOT, add its children to the list
	unsigned int pos = 0;
	for (DATA_INFO* child = di->child; child != nullptr; child = child->next) {
		switch (child->type) {
			case TYPE_ITEM: {
				if (cl_item(child).textcontent == L"")
					continue;
				shared_ptr<clipper_item> item = make_shared<clipper_item>(child);
				if (cl->clipboard == false)
					item->position = pos++; // set position for items
				cl->clippings.push_back(item);
				break;
			}

			case TYPE_FOLDER: {
				string parent = cl->clipboard ? "" : cl->name;
				add_data_info(child, parent); // recursive call to add children
				break;
			}
		}
	}
}

void clipper_bkp::import_data_info(DATA_INFO* di)
{
	unsigned int anzpos = lists.size();
	if (anzpos == 0) {
		// first item, create a new list
		shared_ptr<clipper_list> cl = make_shared<clipper_list>();
		cl->name = "Clipboard";
		cl->position = anzpos++;
		cl->clipboard = true; // clipboard
		lists.push_back(cl);
	}
	
	shared_ptr<clipper_list> clipboard = lists.front();

	cl_item item(di);
	switch (item.itemtype) {
		case TYPE_ITEM: {
			clipboard->clippings.push_back(make_shared<clipper_item>(di));
			break;
		}
		case TYPE_FOLDER: {
			shared_ptr<clipper_list> cl;
			if (item.title == L"Clipper's clipboard" || item.title == L"") {
				cl = clipboard; // use existing clipboard
			}
			else {
				cl = make_shared<clipper_list>();
				cl->name = ::utf16_to_utf8(item.title);
				cl->clipboard = false; // not clipboard
				cl->position = anzpos++;
				lists.push_back(cl);
			}
			// append children to cl's clippings:
			for (DATA_INFO* child = di->child; child != nullptr; child = child->next) {
				if (child->type == TYPE_FOLDER) {
					import_data_info( child);
					continue; // only TYPE_ITEM children
				}
				// create a new clipper_item for each child
				if (child->type == TYPE_ITEM)
					cl->clippings.push_back(make_shared<clipper_item>(child));
			}
			break;
		}
		case TYPE_ROOT: {
			// root item, do nothing
			break;
		}
	}
}

void clipper_bkp::to_item(clip_item& item) const
{
	item.itemtype = TYPE_ROOT;
	item.title.clear();
	item.modified = { 0,0,0,0, 0,0,0, 0 };
	item.children.clear();

	for (const auto& cl : lists) {
		if (cl->clipboard == true) {
			clip_item folder;
			list_to_item(*cl, folder);
			folder.title = clipper_history;
			item.children.push_back(folder);
		}
	}

	for (const auto& cl : lists) {
		if (cl->clipboard == false) {
			clip_item folder;
			list_to_item(*cl, folder);
			item.children.push_back(folder);
		}
	}
}

std::shared_ptr<clipper_list> clipper_bkp::get_default_list()
{
	for (const auto& cl : lists) {
		if (cl->name == default_list_name) {
			return cl; // return the clipboard list
		}
	}
	// if no default list found, create a new one
	shared_ptr<clipper_list> default_list = make_shared<clipper_list>();
	lists.push_back(default_list);
	return default_list;
}


void to_json(json& j, std::shared_ptr<clipper_item> item) {
	if (!item->title.empty()) {
		j = json{
			{"title", item->title},
			{"contents", item->contents},
			{"pinned", item->pinned },
			{"position", item->position},
			{"timestamp_ms", item->timestamp_ms},
			{"timestamp", item->timestamp }
		};
	}
	else {
		j = json{
			{"contents", item->contents},
			{"pinned", item->pinned },
			{"position", item->position},
			{"timestamp_ms", item->timestamp_ms},
			{"timestamp", item->timestamp }
		};
	}
}

void from_json(const json& j, std::shared_ptr<clipper_item> item) {
	if (j.contains("title")) {
		j.at("title").get_to(item->title);
	} else {
		item->title.clear();
	}
	j.at("contents").get_to(item->contents);
	j.at("pinned").get_to(item->pinned);
	j.at("position").get_to(item->position);
	j.at("timestamp_ms").get_to(item->timestamp_ms);
	j.at("timestamp").get_to(item->timestamp);
}

void to_json(json& j, const std::shared_ptr<clipper_list> l) {
	j = json{
		{"name", l->name},
		{"position", l->position },
		{"clipboard", l->clipboard},
		{"clippings", l->clippings}
	};
}

void from_json(const json& j, std::shared_ptr<clipper_list> l) {
	j.at("name").get_to(l->name);
	j.at("position").get_to(l->position);
	j.at("clipboard").get_to(l->clipboard);
	j.at("clippings").get_to(l->clippings);
}

void to_json(json& j, const clipper_info& i) {
	j = json{
		{"platform", i.platform},
		{"platform_version", i.platform_version },
		{"package", i.package},
		{"application_version_code", i.application_version_code},
		{"export_version", i.export_version}
	};
}

void from_json(const json& j, clipper_info& i) {
	j.at("platform").get_to(i.platform);
	j.at("platform_version").get_to(i.platform_version);
	j.at("package").get_to(i.package);
	j.at("application_version_code").get_to(i.application_version_code);
	j.at("export_version").get_to(i.export_version);
}

void to_json(json& j, const clipper_bkp& cb) {
	j = json{
		{"info", cb.info},
		{"lists", cb.lists }
	};
}

void from_json(const json& j, clipper_bkp& cb) {
	j.at("info").get_to(cb.info);
	j.at("lists").get_to(cb.lists);
}


void clipper_to_item(const clipper_item& ci, clip_item& item)
{
	item.itemtype = TYPE_ITEM;
	item.title = utf8_to_utf16(ci.title);
	item.children.clear();
	item.itemformat = CF_UNICODETEXT;
	item.formatname = format_unicode;
	item.textcontent = ::utf8_to_utf16(ci.contents);
	item.windowname = L"Clipper mobile app";
	SYSTEMTIME st = { 0,0,0,0, 0,0,0, 0 };
	item.modified = st;
	if (EpochSecondsToSystemTime(ci.timestamp, st) == 0) {
		item.modified = st;
		item.modified.wMilliseconds = (WORD)(ci.timestamp_ms % 1000);
	}
}

void list_to_item(const clipper_list& cl, clip_item& item)
{
	item.itemtype = TYPE_FOLDER;
	item.title = ::utf8_to_utf16(cl.name);
	item.modified = { 0,0,0,0, 0,0,0, 0 };
	item.children.clear();
	for (const auto& ci : cl.clippings) {
		clip_item child;
		clipper_to_item(*ci, child);
		item.children.push_back(child);
	}
}


std::time_t SystemTimeToEpochSeconds(const SYSTEMTIME st)
{
	if (st.wYear >= 1900 && st.wMonth >= 1 && st.wMonth <= 12
		&& st.wDay >= 1 && st.wDay <= 31 && st.wHour <= 23
		&& st.wMinute <= 59 && st.wSecond <= 59 && st.wMilliseconds <= 999)
	{
		std::tm lt; // local time
		lt.tm_sec = st.wSecond;
		lt.tm_min = st.wMinute;
		lt.tm_hour = st.wHour;
		lt.tm_mday = st.wDay;
		lt.tm_mon = st.wMonth - 1;
		lt.tm_year = st.wYear - 1900;
		lt.tm_isdst = -1;	// calculate daylight saving

		std::time_t t = std::mktime(&lt);
		return t;
	}

	return (std::time_t)0;
}

errno_t EpochSecondsToSystemTime(const std::time_t t, SYSTEMTIME& st)
{
	std::tm lt;
	errno_t err = ::localtime_s(&lt, &t);
	if (err) {
		st = { 0,0,0,0, 0,0,0, 0 };
		return err;
	}

	st.wSecond = lt.tm_sec;
	st.wMinute = lt.tm_min;
	st.wHour = lt.tm_hour;
	st.wDay = lt.tm_mday;
	st.wMonth = lt.tm_mon + 1;
	st.wYear = lt.tm_year + 1900;

	return err;
}

bool FileTimeToEpochSeconds(const FILETIME ft, std::time_t& t)
{
	SYSTEMTIME st;
	if (ft.dwHighDateTime != 0 && ft.dwLowDateTime != 0
		&& ::FileTimeToSystemTimeCL(ft, st))
	{
		t = SystemTimeToEpochSeconds(st);
		return true;
	}

	t = (std::time_t)0;
	return false;
}

bool EpochSecondsToFileTime(const std::time_t t, FILETIME& ft)
{
	SYSTEMTIME st = { 2000,1,0,1, 0,0,0, 0 };
	errno_t err = ::EpochSecondsToSystemTime(t, st);
	if (err == 0 && ::SystemTimeToFileTimeCL(st, ft)) {
		return true;
	}

	ft.dwHighDateTime = ft.dwLowDateTime = 0;
	return false;
}

/*
 * save_clipper
 */
__declspec(dllexport) int CALLBACK save_clipper(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	if (tdi == NULL) 
		return TOOL_SUCCEED;

	if (tdi->di == NULL)
		return TOOL_ERROR;

	int ret = TOOL_SUCCEED;

	clipper_bkp cb;

	DATA_INFO* di = tdi->di;
	DATA_INFO* clipboard = nullptr;
	DATA_INFO* history_root = (DATA_INFO*)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0);
	DATA_INFO* template_root = (DATA_INFO*)SendMessage(hWnd, WM_REGIST_GET_ROOT, 0, 0);

	// Template root or one single folder
	if (di == template_root || di->type == TYPE_FOLDER && tdi->next == nullptr) {
		// take local clipboard subfolder
		clipboard = cb.find_clipboard(di);
		if (clipboard == nullptr) {
			cb.add_data_info(clipboard);
		}
		
		for (DATA_INFO* child = di->child; child != nullptr; child = child->next) {
			cb.add_data_info(child);
		}
	}
	else {
		// list of items:
		// take parent's clipboard folder
		DATA_INFO* pi = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_PARENT, 0, (LPARAM)di);
		clipboard = cb.find_clipboard(pi);
		if (clipboard == nullptr) {
			clipboard = history_root;
		}
		cb.add_data_info(clipboard);
		if (di == history_root)
			return TOOL_SUCCEED;

		for (TOOL_DATA_INFO* td = tdi; td != nullptr; td = td->next) {
			di = td->di;
			if (di == nullptr || di->type < TYPE_ITEM) continue;
			if (di == clipboard) continue; // already added clipboard folder

			cb.add_data_info(di);
		}
	}

	json jitem;
	to_json(jitem, cb);

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

		std::ofstream o(of.lpstrFile);
		// write prettified JSON to file
		//o << std::setw(2) << jitem << std::endl;
		// above does not work because Clipper expects the riight order of members in json objects
		o << std::setw(2) << cb << std::endl;
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
