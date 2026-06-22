#include "winwrappers.h"
#include <WinUser.h>

using namespace std;

std::wstring wndctrl::get_window_text()
{
	wstring s;
	int len = SendDlgItemMessageW(_hDlg, _id, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0);
	if (len > 0) {
		wchar_t* lpwstr = new wchar_t[++len];
		wmemset(lpwstr, (wchar_t)0x00, len);
		SendDlgItemMessageW(_hDlg, _id, WM_GETTEXT, (WPARAM)len, (LPARAM)lpwstr);
		s.assign(lpwstr);
		delete[] lpwstr;
	}
	return s;
}

void wndctrl::set_window_text(const std::wstring& s)
{
	HRESULT result = SendDlgItemMessage(_hDlg, _id, WM_SETTEXT, 0, (LPARAM)s.c_str());
}

void wndcombobox::reset_content()
{
	SendDlgItemMessage(_hDlg, _id, CB_RESETCONTENT, 0, 0);
}

int wndcombobox::get_current_selection_index()
{
	return (int)SendDlgItemMessage(_hDlg, _id, CB_GETCURSEL, 0, 0);
}

int wndcombobox::select_index(const int index)
{
	if (index < 0)
		return CB_ERR;

	return (int)SendDlgItemMessage(_hDlg, _id, CB_SETCURSEL, index, 0);
}

int wndcombobox::select_string(const std::wstring& s)
{
	int cnt = (int)SendDlgItemMessage(_hDlg, _id, CB_GETCOUNT, 0, 0);
	for (int i = 0; i < cnt; i++) {
		int len = SendDlgItemMessageW(_hDlg, _id, CB_GETLBTEXTLEN, (WPARAM)i, (LPARAM)0);
		wchar_t* lpwstr = new wchar_t[++len];
		wmemset(lpwstr, (wchar_t)0x00, len);
		SendDlgItemMessageW(_hDlg, _id, CB_GETLBTEXT, (WPARAM)i, (LPARAM)lpwstr);
		wstring item_text;
		item_text.assign(lpwstr);
		delete[] lpwstr;
		if (item_text == s) {
			return select_index(i);
		}
	}
	return CB_ERR;
}

LPARAM wndcombobox::get_item_data(const int index)
{
	return SendDlgItemMessage(_hDlg, _id, CB_GETITEMDATA, index, 0);
}

void wndcombobox::set_item_data(const int index, const LPARAM data)
{
	if (index < 0)
		return;

	HRESULT result = SendDlgItemMessage(_hDlg, _id, CB_SETITEMDATA, index, data);
}

int wndcombobox::add_string(const std::wstring& s, const LPARAM data)
{
	int index = (int)SendDlgItemMessage(_hDlg, _id, CB_ADDSTRING, 0, (LPARAM)s.c_str());
	if (index == CB_ERR || index == CB_ERRSPACE) {
		return CB_ERR;
	}
	HRESULT result = SendDlgItemMessage(_hDlg, _id, CB_SETITEMDATA, index, data);
	return index;
}

std::wstring wndcombobox::get_current_selection_text()
{
	int index = (int)SendDlgItemMessage(_hDlg, _id, CB_GETCURSEL, 0, 0);
	if (index < 0) {
		return L"";
	}
	int len = SendDlgItemMessageW(_hDlg, _id, CB_GETLBTEXTLEN, (WPARAM)index, (LPARAM)0);
	wchar_t* lpwstr = new wchar_t[++len];
	wmemset(lpwstr, (wchar_t)0x00, len);
	SendDlgItemMessageW(_hDlg, _id, CB_GETLBTEXT, (WPARAM)index, (LPARAM)lpwstr);
	wstring s;
	s.assign(lpwstr);
	delete[] lpwstr;

	return s;
}

LPARAM wndcombobox::get_current_selection_data()
{
	int index = (int)SendDlgItemMessage(_hDlg, _id, CB_GETCURSEL, 0, 0);

	if (index < 0) {
		return (LPARAM)0;
	}

	return SendDlgItemMessage(_hDlg, _id, CB_GETITEMDATA, index, 0);
}
