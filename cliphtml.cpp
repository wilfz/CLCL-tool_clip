
#include "CLCLPlugin.h"
#include "memory.h"

// CopyHtml() - Copies given HTML to the data item.
// The HTML/BODY blanket is provided, so you only need to
// call it like CallHtml("<b>This is a test</b>");
int CopyHTML(DATA_INFO* di, TCHAR *thtml)
{
	char* html = nullptr;
#ifdef UNICODE
	// convert thtml to utf8:
	int len = (int)wcslen(thtml) + 1; // include trailing '\0'
	// each WCHAR may expand to maximal 4 byte in UTF-8
	int size = len * 4;
	html = new char[size];

	int cnt = ::WideCharToMultiByte(CP_UTF8, 0, thtml, len, html, size, NULL, FALSE);
	if (cnt <= 0) {
		delete[] html;
		return TOOL_ERROR;
	}
#else
	// TODO
	return TOOL_ERROR;
#endif
	// Create temporary buffer for HTML header...
	char *buf = new char[400 + strlen(html)];
	if (!buf) {
		delete html;
		return TOOL_ERROR;
	}
	
	// Get clipboard id for HTML format...
	static int cfid = 0;
	if (!cfid) cfid = RegisterClipboardFormat(TEXT("HTML Format"));

	#pragma warning( push )
	#pragma warning( disable : 4996 )
	// Create a template string for the HTML header...
	strcpy(buf,
		"Version:0.9\r\n"
		"StartHTML:00000000\r\n"
		"EndHTML:00000000\r\n"
		"StartFragment:00000000\r\n"
		"EndFragment:00000000\r\n"
		"<html><body>\r\n"
		"<!--StartFragment -->\r\n");

	// Append the HTML...
	strcat(buf, html);
	strcat(buf, "\r\n");
	// Finish up the HTML format...
	strcat(buf,
		"<!--EndFragment-->\r\n"
		"</body>\r\n"
		"</html>");

	// Now go back, calculate all the lengths, and write out the
	// necessary header information. Note, wsprintf() truncates the
	// string when you overwrite it so you follow up with code to replace
	// the 0 appended at the end with a '\r'...
	char *ptr = strstr(buf, "StartHTML");

	wsprintfA(ptr + 10, "%08u", strstr(buf, "<html>") - buf);
	*(ptr + 10 + 8) = '\r';

	ptr = strstr(buf, "EndHTML");
	wsprintfA(ptr + 8, "%08u", strlen(buf));
	*(ptr + 8 + 8) = '\r';

	ptr = strstr(buf, "StartFragment");
	wsprintfA(ptr + 14, "%08u", strstr(buf, "<!--StartFrag") - buf);
	*(ptr + 14 + 8) = '\r';

	ptr = strstr(buf, "EndFragment");
	wsprintfA(ptr + 12, "%08u", strstr(buf, "<!--EndFrag") - buf);
	*(ptr + 12 + 8) = '\r';

	// as buf is ready we don't need html anymore
	delete[] html;

	// Now you have everything in place ready to fill di->data.
	size_t targetlen = strlen(buf) + 1;
	HANDLE ret = NULL;
	char* to_mem = nullptr;

	if ((ret = GlobalAlloc(GHND, targetlen)) == NULL) {
		delete[] buf;
		return TOOL_ERROR;
	}
	if ((to_mem = (char*)GlobalLock(ret)) == nullptr) {
		delete[] buf;
		GlobalFree(ret);
		return TOOL_ERROR;
	}

	// copying to target
	for (size_t i = 0; i < targetlen; i++) {
		to_mem[i] = buf[i];
	}
	to_mem[targetlen] = '\0';
	delete[] buf;

	GlobalUnlock(ret);
	GlobalFree(di->data);
	di->data = ret;
	di->size = targetlen;
	return TOOL_DATA_MODIFIED;
}

int build_html_table(const TCHAR* source, TCHAR* target)
{
	int tsize = 0;
	// html table
	// having style tag inside of body is not W3C-conform, but works.
	static const TCHAR table_open[] = TEXT("<style> th, td { border: thin solid; padding-right: 4px; padding-left: 4px; } </style><table style=\"border-collapse: collapse;\">");
	static const TCHAR table_close[] = TEXT("</table>");
	BOOL bTable = FALSE;
	// table header
	static const TCHAR thead_open[] = TEXT("<thead>");
	static const TCHAR thead_close[] = TEXT("</thead>");
	BOOL bTableHead = FALSE;
	// table body
	static const TCHAR tbody_open[] = TEXT("<tbody>");
	static const TCHAR tbody_close[] = TEXT("</tbody>");
	BOOL bTableBody = FALSE;
	// table row
	static const TCHAR tr_open[] = TEXT("<tr>");
	static const TCHAR tr_close[] = TEXT("</tr>");
	BOOL bTableRow = FALSE;
	// header and data item
	static const TCHAR th_open[] = TEXT("<th>");
	static const TCHAR th_close[] = TEXT("</th>");
	static const TCHAR td_open[] = TEXT("<td>");
	static const TCHAR td_close[] = TEXT("</td>");
	BOOL bTableItem = FALSE;

	TCHAR c, d;
	int i = 0;
	while ((c = source[i]) != '\0')
	{
		if (c != '\n' && c != '\r')
		{
			if (target)
				lstrcpy(target + tsize, table_open);
			tsize += lstrlen(table_open);
			bTable = TRUE;
			if (target)
				lstrcpy(target + tsize, tr_open);
			tsize += lstrlen(tr_open);
			bTableRow = TRUE;
			if (target)
				lstrcpy(target + tsize, th_open);
			tsize += lstrlen(th_open);
			bTableItem = TRUE;
			break;
		}
		i++;
	}

	// header row
	while ((c = source[i++]) != '\0')
	{
		if (c == '\n' || c == '\r')
		{
			if (c == '\r' && (d = source[i]) == '\n')
				c = source[i++];
			else if (c == '\n' && (d = source[i]) == '\r')
				c = source[i++];

			if (target)
				lstrcpy(target + tsize, th_close);
			tsize += lstrlen(th_close);
			bTableItem = FALSE;
			if (target)
				lstrcpy(target + tsize, tr_close);
			tsize += lstrlen(tr_close);
			bTableRow = FALSE;
			break;
		}

		switch (c)
		{
		case '\t':
			if (target)
				lstrcpy(target + tsize, th_close);
			tsize += lstrlen(th_close);
			if (target)
				lstrcpy(target + tsize, th_open);
			tsize += lstrlen(th_open);
			bTableItem = TRUE;
			break;
		default:
			if (target)
				target[tsize] = c;
			tsize++;
		}
	}

	if ((c = source[i]) == '\0')
	{
		if (bTableItem)
		{
			if (target)
				lstrcpy(target + tsize, th_close);
			tsize += lstrlen(th_close);
			bTableItem = FALSE;
		}
	}
	else
	{
		if (target)
			lstrcpy(target + tsize, tr_open);
		tsize += lstrlen(tr_open);
		bTableRow = TRUE;
		if (target)
			lstrcpy(target + tsize, td_open);
		tsize += lstrlen(td_open);
		bTableItem = TRUE;
	}

	// data rows
	while ((c = source[i++]) != '\0')
	{
		if (c == '\n' || c == '\r')
		{
			if (c == '\r' && (d = source[i]) == '\n')
				i++;
			else if (c == '\n' && (d = source[i]) == '\r')
				i++;

			if (bTableItem)
			{
				if (target)
					lstrcpy(target + tsize, td_close);
				tsize += lstrlen(td_close);
				bTableItem = FALSE;
			}
			if (bTableRow)
			{
				if (target)
					lstrcpy(target + tsize, tr_close);
				tsize += lstrlen(tr_close);
				bTableRow = FALSE;
			}
			continue;
		}

		if (!bTableRow)
		{
			if (target)
				lstrcpy(target + tsize, tr_open);
			tsize += lstrlen(tr_open);
			bTableRow = TRUE;
		}
		if (!bTableItem)
		{
			if (target)
				lstrcpy(target + tsize, td_open);
			tsize += lstrlen(td_open);
			bTableItem = TRUE;
		}

		switch (c)
		{
		case '\t':
			if (target)
				lstrcpy(target + tsize, td_close);
			tsize += lstrlen(td_close);
			if (target)
				lstrcpy(target + tsize, td_open);
			tsize += lstrlen(td_open);
			break;
		default:
			if (target)
				target[tsize] = c;
			tsize++;
		}
	}

	if (bTableItem)
	{
		if (target)
			lstrcpy(target + tsize, td_close);
		tsize += lstrlen(td_close);
		bTableItem = FALSE;
	}
	if (bTableRow)
	{
		if (target)
			lstrcpy(target + tsize, tr_close);
		tsize += lstrlen(tr_close);
		bTableRow = FALSE;
	}
	if (bTable)
	{
		if (target)
			lstrcpy(target + tsize, table_close);
		tsize += lstrlen(table_close);
		bTable = FALSE;
	}

	if (target)
		target[tsize] = '\0';
	tsize++;

	return tsize;
}

int item_html_table(DATA_INFO* di, TCHAR* src)
{
	char* utf8table = nullptr;
	int size;

	// first loop over from_mem to calculate size to be allocated
	size = build_html_table(src, (TCHAR*) nullptr);
	if (size <= 0)
		return TOOL_ERROR;

	TCHAR* wtable = new TCHAR[size]; // allocate that memory

	// second loop over from_mem fills the previuosly allocated memory
	size = build_html_table(src, wtable);

	int ret = CopyHTML(di, wtable);
	delete[] wtable;
	return ret;
}


/*
 * tabSeparatedToHtmlTable_tool
 * Tool processing
 *
 *	argument:
 *		hWnd - the calling window
 *		tei - tool execution information
 *		tdi - item information for tools
 *
 *	Return value:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK tabSeparatedToHtmlTable(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	DATA_INFO* src_di = nullptr;
	DATA_INFO* dst_di = nullptr;
	int ret = TOOL_SUCCEED;

	// walk the currently selected items
	for (; tdi != NULL; tdi = tdi->next) {
		DATA_INFO* di = tdi->di;
		if (SendMessage(hWnd, WM_ITEM_CHECK, 0, (LPARAM)tdi->di) == -1) {
			continue;
		}

		if (di->type == TYPE_DATA) {
			di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_PARENT, 0, (LPARAM)di);
		}
		if (di->type != TYPE_ITEM)
			return TOOL_ERROR;

#ifdef UNICODE
		src_di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)di);
#else
		src_di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)di);
#endif
		if (src_di == nullptr || src_di->data == nullptr)
			continue;

		TCHAR* source = nullptr;
		// lock copy source
		if ((source = (TCHAR*)GlobalLock(src_di->data)) == NULL) {
			return TOOL_ERROR;
		}

		TCHAR format_name[] = TEXT("HTML Format");
		dst_di = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("HTML Format"), (LPARAM)di);

		if (dst_di != nullptr && dst_di->data != nullptr) {
			ret |= item_html_table(dst_di, source);
		}
		else {
			// walk the data items (with various formats) belonging to di
			for (dst_di = di->child; dst_di != nullptr; dst_di = dst_di->next) {
				if (dst_di->format_name == TEXT("HTML Format")) {
					ret |= item_html_table(dst_di, source);
					break;
				}
				if (dst_di->next != nullptr)
					continue;

				// HTML format not found.
				// create a new data item and append it to 
				// append it at the end:
				// Get clipboard id for HTML format...
				UINT cf_html_format = RegisterClipboardFormat(TEXT("HTML Format"));
				if (cf_html_format == 0) {
					GlobalUnlock(src_di->data);
					return TOOL_ERROR;
				}

				DATA_INFO* new_item = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("HTML Format"));
				if (new_item == nullptr) {
					GlobalUnlock(src_di->data);
					return TOOL_ERROR;
				}

				new_item->data = nullptr;
				new_item->size = 0;
				if (item_html_table(new_item, source) == TOOL_DATA_MODIFIED) {
					dst_di = dst_di->next = new_item;
					dst_di->next = nullptr;
					ret = TOOL_DATA_MODIFIED;
				}
			}
		}

		GlobalUnlock(src_di->data);
	}

	return ret;
}

/*
 * tabSeparatedToHtmlTable_property
 * Show properties
 *
 *	argument:
 *		hWnd - handle of the options window
 *		tei - tool execution information
 *
 *	Return value:
 *		TRUE - with properties
 *		FALSE - no property
 */
__declspec(dllexport) BOOL CALLBACK tabSeparatedToHtmlTable_property(const HWND hWnd, TOOL_EXEC_INFO* tei)
{
	//DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_REPLACE_WHITESPACE), hWnd, set_WhiteSpaceReplacement, 0);
	//return TRUE;
	return FALSE;
}


