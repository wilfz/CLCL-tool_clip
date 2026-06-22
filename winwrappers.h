#pragma once

#include <string>
#include <wtypes.h>

class wndctrl
{
public:
	wndctrl(const HWND hDlg, const int id) {
		_hDlg = hDlg;
		_id = id;
	};

	std::wstring get_window_text();
	void set_window_text(const std::wstring& s);

protected:
	HWND _hDlg;
	int _id;
};

class wndcombobox : public wndctrl
{
public:
	wndcombobox(const HWND hDlg, const int id) : wndctrl(hDlg, id) {};
	void reset_content();
	int get_current_selection_index();
	int select_index(const int index);
	int select_string(const std::wstring& s);
	LPARAM get_item_data(const int index);
	void set_item_data(const int index, const LPARAM data);
	int add_string(const std::wstring& s, const LPARAM data = (LPARAM)0);
	std::wstring get_current_selection_text();
	LPARAM get_current_selection_data();
};
