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
