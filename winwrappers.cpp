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
	}
	return s;
}

void wndctrl::set_window_text(const std::wstring& s)
{
	HRESULT result = SendDlgItemMessage(_hDlg, _id, WM_SETTEXT, 0, (LPARAM)s.c_str());
}
