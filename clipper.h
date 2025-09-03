#pragma once

#include <string>
#include <list>
#include <codecvt>
#include "CLCLPlugin.h"
#include "clipitem.h"


// helper classes for export to and import from mobile clipper app
class clipper_item
{
public:
	clipper_item();
	clipper_item(DATA_INFO* di);

	std::string title;
	std::string contents;
	bool pinned;
	unsigned int position; // 0
	long long timestamp_ms; // 1748700892694,
	std::time_t timestamp;  // 1748700892

	friend std::ostream& operator<< (std::ostream& os, const clipper_item& ci);
};

class clipper_list
{
public:
	clipper_list() {
		position = 0;
		clipboard = false;
	};

	std::string name;
	unsigned int position;
	bool clipboard;

	std::list<std::shared_ptr<clipper_item>> clippings;

	friend std::ostream& operator<< (std::ostream& os, const clipper_list& cl);
};

class clipper_info {
public:
	// subject to change:
	clipper_info() {
		platform = "Windows";
		platform_version = 0;
		package = "org.rojekti.clipper";
		application_version_code = 112;
		export_version = 3;
	};

	std::string platform;
	unsigned int platform_version;
	std::string package;
	unsigned int application_version_code;
	unsigned int export_version;

	friend std::ostream& operator<< (std::ostream& os, const clipper_info& i);
};

class clipper_bkp
{
public:
	clipper_info info;
	std::list<std::shared_ptr<clipper_list>> lists;

	DATA_INFO* find_clipboard(DATA_INFO* di) const;
	void add_data_info(DATA_INFO* di, std::string parentname = "");
	void import_data_info(DATA_INFO* di);
	void to_item(clip_item& item) const;
	std::shared_ptr<clipper_list> get_default_list();

	friend std::ostream& operator<< (std::ostream& os, const clipper_bkp& cb);
};

void clipper_to_item(const clipper_item& ci, clip_item& item);
void list_to_item(const clipper_list& cl, clip_item& item);


std::time_t SystemTimeToEpochSeconds(const SYSTEMTIME st);
errno_t EpochSecondsToSystemTime(const std::time_t t, SYSTEMTIME& st);
bool FileTimeToEpochSeconds(const FILETIME ft, std::time_t& t);
bool EpochSecondsToFileTime(const std::time_t t, FILETIME& ft);
