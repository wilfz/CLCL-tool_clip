
/* Include Files */
#include "CLCLPlugin.h"
#ifndef IDD_REPLACE_WHITESPACE
#include "resource.h"
#endif
#include "memory.h"
#include "query\query.h"
#include "query\table.h"
#include "query\lvstring.h"
#include "query\odbcenvironment.h"


extern HINSTANCE hInst;
extern tstring ini_path;
extern tstring connection_string;

using namespace std;
using namespace linguversa;

class db_item
{
public:
	db_item() {
		sort = itemtype = itemformat = 0L;
		modified = { 0,0,0, 0,0,0, 0 };
	};

	tstring title;
	tstring parent;
	long sort;
	long itemtype;
	long itemformat;
	tstring formatname;
	tstring textcontent;
	TIMESTAMP_STRUCT modified;
};

class db_item_array : public vector<db_item*> {
public:
	~db_item_array() {
		for (size_t i = 0; i < size(); i++) {
			db_item* pdbi = at(i);
			if (pdbi)
				delete pdbi;
		}
	};
};

// forward declarations of local helper functions
bool init_dialog(HWND hDlg);
SQLRETURN build_table_clipitem(linguversa::Connection& con);
tstring create_ancestors(const HWND hWnd, DATA_INFO* di, linguversa::Query& create_folder);
bool FileTimeToTimestampStruct(const FILETIME ft, TIMESTAMP_STRUCT& ts);
bool TimestampStructToFileTime(const TIMESTAMP_STRUCT ts, FILETIME& ft);
int item_to_db(TOOL_DATA_INFO* tdi, tstring parent, linguversa::Query& statement);
SQLRETURN db_to_item(DATA_INFO* di, const tstring parent, linguversa::Query& select);
SQLRETURN load_subtree(const HWND hWnd, DATA_INFO* parent_item, const tstring parent_title, linguversa::Query& tree);
int sync_db_item(const db_item& dbi, DATA_INFO* item, DATA_INFO* data);

static BOOL CALLBACK dlg_odbc_properties(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * save_odbc
 */
__declspec(dllexport) int CALLBACK save_odbc(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	if (tdi == NULL) {
		return TOOL_SUCCEED;
	}

	if (connection_string.empty() 
	&& DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ODBC_SETTINGS), hWnd, dlg_odbc_properties, (LPARAM)tdi) == FALSE) {
		return TOOL_CANCEL;
	}

	linguversa::Connection con;
	bool b = con.Open(connection_string, hWnd);
	if (b == false)
		return TOOL_ERROR;

	SQLRETURN sqlret = build_table_clipitem(con);
	if (!SQL_SUCCEEDED(sqlret))
		return TOOL_ERROR;

	linguversa::Query insert_or_replace(&con);
	tstring drivername;
	sqlret = con.SqlGetDriverName(drivername);
	if (drivername == _T("SQLite3 ODBC Driver") || drivername == _T("sqlite3odbc.dll"))
		sqlret = insert_or_replace.Prepare(
			_T("replace into clipitem( title, parent, sort, itemtype, itemformat, formatname, textcontent, modified) values(?,?,?,?,?,?,?,?);"));
	else if (drivername == _T("SQL Server") || drivername == _T("SQLSRV32.DLL"))
		sqlret = insert_or_replace.Prepare(_T(
			"{call replace_clipitem (?,?,?,?,?,?,?,?)}"
			//"declare @title varchar(255), @parent varchar(255), @sort int, @itemtype int, @itemformat int, @formatname varchar(255), @textcontent varchar, @modified datetime;"
			//"select @title=?, @parent=?, @sort=?, @itemtype=?, @itemformat=?, @formatname=?, @textcontent=?, @modified=?;"
			//"insert into clipitem( title, parent, sort, itemtype, itemformat, formatname, textcontent, modified) "
			//"select @title, @parent, @sort, @itemtype, @itemformat, @formatname, @textcontent, @modified "
			//"where not exists( select * from clipitem where title = @title and parent = @parent);"
			//"update clipitem set title=@title, parent=@parent, sort=@sort, itemtype=@itemtype, itemformat=@itemformat, formatname=@formatname, textcontent=@textcontent; modified=@modified;"
		));
	else if (drivername.substr(0, 10) == _T("PostgreSQL") || drivername.substr(0, 8) == _T("PSQLODBC"))
		sqlret = insert_or_replace.Prepare(_T(
			"insert into clipitem(title, parent, sort, itemtype, itemformat, formatname, textcontent, modified) values(?,?,?,?,?,?,?,?)"
			" on conflict(title, parent) do update set"
			" sort = excluded.sort, itemtype = excluded.itemtype, itemformat = excluded.itemformat,"
			" formatname = excluded.formatname, textcontent = excluded.textcontent, modified = excluded.modified; "
		));

	// find out where in the tree we are:
	tstring parent = create_ancestors( hWnd, tdi->di, insert_or_replace);

	int ret = item_to_db(tdi, parent, insert_or_replace);

	insert_or_replace.Close();
	con.Close();
	return ret;
}

__declspec(dllexport) BOOL CALLBACK save_odbc_property(const HWND hWnd, TOOL_EXEC_INFO* tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ODBC_SETTINGS), hWnd, dlg_odbc_properties, 0);
	return TRUE;
}

/*
 * load_odbc
 */
__declspec(dllexport) int CALLBACK load_odbc(const HWND hWnd, TOOL_EXEC_INFO* tei, TOOL_DATA_INFO* tdi)
{
	DATA_INFO* regist_root;

	// Get regist/template
	if ((regist_root = (DATA_INFO*)SendMessage(hWnd, WM_REGIST_GET_ROOT, 0, 0)) == NULL) {
		return TOOL_SUCCEED;
	}

	if (connection_string.empty()
		&& DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ODBC_SETTINGS), hWnd, dlg_odbc_properties, (LPARAM)tdi) == FALSE) {
		return TOOL_CANCEL;
	}

	linguversa::Connection con;
	bool b = con.Open(connection_string, hWnd);
	if (b == false)
		return TOOL_ERROR;

	SQLRETURN sqlret = build_table_clipitem(con);
	if (!SQL_SUCCEEDED(sqlret))
		return TOOL_ERROR;

	linguversa::Query tree(con);
	sqlret = tree.Prepare(
		_T("select title, parent, sort, itemtype, itemformat, formatname, textcontent, modified "
			"from clipitem where parent = ? order by sort;"));
	if (!SQL_SUCCEEDED(sqlret))
		return TOOL_ERROR;

	sqlret = load_subtree(hWnd, regist_root, _T(""), tree);

	if (!SQL_SUCCEEDED(sqlret))
		return TOOL_ERROR;

	// Notify regist/template changes
	SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
	return TOOL_SUCCEED;
}

__declspec(dllexport) BOOL CALLBACK load_odbc_property(const HWND hWnd, TOOL_EXEC_INFO* tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ODBC_SETTINGS), hWnd, dlg_odbc_properties, 0);
	return TRUE;
}

bool init_dialog(HWND hDlg)
{
	ODBCEnvironment env;
	tstring dsn, drivername;
	HRESULT result = SendDlgItemMessage(hDlg, IDC_COMBO_DSN, CB_ADDSTRING, 0, (LPARAM)_T(""));
	while (SQL_SUCCEEDED(env.FetchDataSourceInfo(dsn, drivername))) {
		result = SendDlgItemMessage(hDlg, IDC_COMBO_DSN, CB_ADDSTRING, 0, (LPARAM)dsn.c_str());
	}
	result = SendDlgItemMessage(hDlg, IDC_EDIT_CONNECTIONSTRING, WM_SETTEXT, 0, (LPARAM)connection_string.c_str());

	return true;
}

// dialogue with ODBC settings
static BOOL CALLBACK dlg_odbc_properties(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		init_dialog(hDlg);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COMBO_DSN:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				int idx = SendDlgItemMessage(hDlg, IDC_COMBO_DSN, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				int len = SendDlgItemMessage(hDlg, IDC_COMBO_DSN, CB_GETLBTEXTLEN, (WPARAM)idx, (LPARAM)0);
				if (len == 0)
					return TRUE;
				TCHAR* dsn = new TCHAR[++len]; // increase for 0-terminaton
				len = SendDlgItemMessage(hDlg, IDC_COMBO_DSN, CB_GETLBTEXT, (WPARAM)idx, (LPARAM)dsn);
				tstring conio = string_format(_T("DSN=%s;"), dsn);
				delete[] dsn;
				Connection con;
				if (con.Open(conio, hDlg))
					SendDlgItemMessage(hDlg, IDC_EDIT_CONNECTIONSTRING, WM_SETTEXT, 0, (LPARAM)conio.c_str());
				con.Close();
			}
			break;

		case IDOK:
			{
				// get odbc properties from the controls and test the connection
				int len = SendDlgItemMessage(hDlg, IDC_EDIT_CONNECTIONSTRING, WM_GETTEXTLENGTH, (WPARAM)0, (LPARAM)0);
				TCHAR* conn = new TCHAR[++len];
				SendDlgItemMessage(hDlg, IDC_EDIT_CONNECTIONSTRING, WM_GETTEXT, (WPARAM)len, (LPARAM)conn);
				tstring conio;
				conio.assign(conn);
				delete[] conn;
				Connection con;
				if (con.Open(conio, hDlg)) {
					connection_string = conio;
					::WritePrivateProfileString(TEXT("ODBC"), TEXT("ConnectionString"), connection_string.c_str(), ini_path.c_str());
					EndDialog(hDlg, TRUE);
				}
				con.Close();
			}
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

SQLRETURN db_to_item(DATA_INFO* di, const tstring parent, linguversa::Query& select)
{
	bool b = false;
	long itemtype = -1;
	tstring nextparent;

	SQLRETURN sqlret = SQL_SUCCESS;
	while (SQL_SUCCEEDED(sqlret = select.Fetch()) && (b = select.GetFieldValue(_T("parent"), nextparent)) && parent == nextparent) {
		tstring title, parent, formatname, textcontent;
		long sort = 0, itemformat = 0;

		while (SQL_SUCCEEDED(sqlret) && (b = select.GetFieldValue(_T("itemtype"), itemtype)) && itemtype == TYPE_FOLDER) {
			b = select.GetFieldValue(_T("title"), title);
			// TODO: find or create child item of TYPE_FOLDER with with matching title
			// the function also tries to fetch the next sibling
			sqlret = db_to_item(di, title, select);
		}

		if (!b || !SQL_SUCCEEDED(sqlret)) // if there is no sibling sqlret is SQL_NODATA
			return sqlret;

		// now normal data items
		//load_item( select, regist_root);
		b = select.GetFieldValue(_T("title"), title);
		b = b && select.GetFieldValue(_T("parent"), parent);
		b = b && select.GetFieldValue(_T("sort"), sort);
		b = b && select.GetFieldValue(_T("itemtype"), itemtype);
		b = b && select.GetFieldValue(_T("itemformat"), itemformat);
		b = b && select.GetFieldValue(_T("formatname"), formatname);
		b = b && select.GetFieldValue(_T("textcontent"), textcontent);
	}

	return sqlret;
}

SQLRETURN load_subtree(const HWND hWnd, DATA_INFO* parent_item, const tstring parent_title, linguversa::Query& tree) {
	SQLRETURN sqlret = tree.SetParamValue(1, parent_title);
	if (!SQL_SUCCEEDED(sqlret))
		return sqlret;

	sqlret = tree.Execute();
	if (!SQL_SUCCEEDED(sqlret))
		return sqlret;

	db_item_array ia;
	// iterate the query and add all items into a db_item_array
	for (sqlret = tree.Fetch(); SQL_SUCCEEDED(sqlret); sqlret = tree.Fetch()) {
		db_item* dbi = new db_item();
		tree.GetFieldValue(_T("title"), dbi->title);
		tree.GetFieldValue(_T("parent"), dbi->parent);
		tree.GetFieldValue(_T("sort"), dbi->sort);
		tree.GetFieldValue(_T("itemtype"), dbi->itemtype);
		tree.GetFieldValue(_T("itemformat"), dbi->itemformat);
		tree.GetFieldValue(_T("formatname"), dbi->formatname);
		tree.GetFieldValue(_T("textcontent"), dbi->textcontent);
		tree.GetFieldValue(_T("modified"), dbi->modified);
		ia.push_back(dbi);
	}

	if (sqlret != SQL_NO_DATA)
		return sqlret;

	for (size_t i = 0; i < ia.size(); i++) { // iterate the db_items in the array
		db_item* dbi = ia[i];
		if (dbi == nullptr)
			continue;
		DATA_INFO* child_item = nullptr;
		DATA_INFO* last_child = nullptr;
		// here, starting from current parent_item search for a child_item matching dbi
		for (DATA_INFO* cdi = parent_item->child; cdi != nullptr; cdi = cdi->next) {
			DATA_INFO* unicode_data = nullptr;
			DATA_INFO* multibyte_data = nullptr;
			if (cdi->title != nullptr && _tcsicmp(cdi->title, dbi->title.c_str()) != 0
				|| cdi->menu_title != nullptr && _tcsicmp(cdi->menu_title, dbi->title.c_str()) != 0)
			{
				// not the one we are looking for
				child_item = nullptr;
				last_child = cdi;
				continue;
			}

			bool ItemTitleMatch = cdi->title != nullptr && _tcsicmp(cdi->title, dbi->title.c_str()) == 0
				|| cdi->menu_title != nullptr && _tcsicmp(cdi->menu_title, dbi->title.c_str()) == 0;

			if (ItemTitleMatch && cdi->type == TYPE_FOLDER) {
				child_item = cdi;
				break;
			}
			
			if (cdi->type == TYPE_ITEM) {
				if (ItemTitleMatch)
					child_item = cdi;
				// search for data_item with matching title and UNICODE format
				for (unicode_data = cdi->child; unicode_data != nullptr; unicode_data = unicode_data->next) {
					bool DataTitleMatch = unicode_data->title != nullptr && _tcsicmp(unicode_data->title, dbi->title.c_str()) == 0
						|| unicode_data->menu_title != nullptr && _tcsicmp(unicode_data->menu_title, dbi->title.c_str()) == 0;

					if ((ItemTitleMatch || DataTitleMatch) && unicode_data->format == CF_UNICODETEXT)
					{
						child_item = cdi;
						// sync cdi with dbi, but unicode_data->format must also fit
						int ret = sync_db_item(*dbi, child_item, unicode_data);
						break;
					}
				}

				// search for data_item with matching title and MULTIBYTE format
				for (multibyte_data = cdi->child; multibyte_data != nullptr; multibyte_data = multibyte_data->next) {
					bool DataTitleMatch = multibyte_data->title != nullptr && _tcsicmp(multibyte_data->title, dbi->title.c_str()) == 0
						|| multibyte_data->menu_title != nullptr && _tcsicmp(multibyte_data->menu_title, dbi->title.c_str()) == 0;

					if ((ItemTitleMatch || DataTitleMatch) && multibyte_data->format == CF_TEXT)
					{
						child_item = cdi;
						// sync child_item with dbi, but multibyte_data->format must also fit
						int ret = sync_db_item(*dbi, child_item, multibyte_data);
						break;
					}
				}

				// if there is a child_item without data_item of format CF_UNICODE or CF_TEXT
				if (child_item && unicode_data == nullptr && multibyte_data == nullptr && dbi->itemformat == CF_UNICODETEXT) {
					if ((unicode_data = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("UNICODE TEXT"))) == NULL) {
						SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)child_item);
						return SQL_NO_DATA;
					}
					unicode_data->next = child_item->child;
					child_item->child = unicode_data;
					int ret = sync_db_item(*dbi, child_item, unicode_data); // create the data_item
				}

				if (child_item)
					break;
			}
			
			child_item = nullptr;
			last_child = cdi;
		}

		if (child_item == nullptr) { // If we didn't find a match ...
			switch (dbi->itemtype) 
			{
			case TYPE_DATA:
				// create an item ...
				if ((child_item = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_ITEM, (LPARAM)dbi->title.c_str())) == NULL) {
					return SQL_NO_DATA;
				}
				// initialize item's modified time to 0 (otherwise it's current time)
				child_item->modified.dwHighDateTime = child_item->modified.dwLowDateTime = 0;
				if (dbi->itemformat == CF_TEXT && (child_item->child = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("TEXT"))) == NULL) {
					SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)child_item);
					return SQL_NO_DATA;
				}
				else if (dbi->itemformat == CF_UNICODETEXT && (child_item->child = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("UNICODE TEXT"))) == NULL) {
					SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)child_item);
					return SQL_NO_DATA;
				}
				//child_item->title = alloc_copy(dbi->title.c_str());
				break;
			case TYPE_FOLDER:
				// create a folder ...
				if ((child_item = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_FOLDER, (LPARAM)dbi->title.c_str())) == NULL) {
					return SQL_NO_DATA;
				}
				break;
			default:
				// we should never come here
				return SQL_NO_DATA;
			}

			if (last_child != nullptr) { // and append to the last child ...
				last_child->next = child_item;
				child_item->next = nullptr;
			}
			else if (parent_item->child == nullptr) { // attach it to parent_item as the only child ...
				parent_item->child = child_item;
				child_item->next = nullptr;
			}
			else {
				// we should never come here
				return SQL_NO_DATA;
			}

			// and finally sync it from the db_item dbi.
			int ret = sync_db_item(*dbi, child_item, child_item->child);
		}

		// If child_item is of TYPE_FOLDER go into recursion.
		if (child_item != nullptr && child_item->type == TYPE_FOLDER && dbi->itemtype == TYPE_FOLDER && dbi->title.length() > 0) {
			if(_tcsicmp(child_item->title ? child_item->title : _T(""), dbi->title.c_str()) != 0)
				continue;
			sqlret = load_subtree(hWnd, child_item, dbi->title, tree);
			if (!SQL_SUCCEEDED(sqlret))
				return sqlret;
		}
	}

	if (sqlret == SQL_NO_DATA)
		sqlret = SQL_SUCCESS;

	return sqlret;
}

SQLRETURN build_table_clipitem(linguversa::Connection& con)
{
	tstring dbms_name, drivername;
	SQLRETURN sqlret = con.SqlGetInfo(SQL_DBMS_NAME, dbms_name);
	sqlret = con.SqlGetDriverName(drivername);

	linguversa::Table tbl(&con);
	sqlret = tbl.LoadTableInfo(_T("clipitem"));
	if (!SQL_SUCCEEDED(sqlret) && sqlret != SQL_NO_DATA)
		return sqlret;

	if (sqlret == SQL_NO_DATA || tbl.GetColumnCount() == 0) {
		// the table clipitem does not yet exist
		tbl.Close();
		// create the table
		linguversa::Query create(&con);
		sqlret = create.ExecDirect(
			_T("create table clipitem( title varchar(255) not null, parent varchar(255) not null, sort int null, itemtype int null, itemformat int null, formatname varchar(255) null, textcontent varchar null, primary key(title, parent));")
		);
		if (!SQL_SUCCEEDED(sqlret))
			return sqlret;
		create.Close();
		// now the table should exist
		sqlret = tbl.LoadTableInfo(_T("clipitem"));
		if (!SQL_SUCCEEDED(sqlret) && sqlret != SQL_NO_DATA)
			return sqlret; // otherwise exit
	}

	// alter table to add missing columns
	if (tbl.GetColumnIndex(_T("itemtype")) < 0) {
		linguversa::Query altertable(&con);
		sqlret = altertable.ExecDirect(
			_T("alter table clipitem add itemtype int null;"));
		if (!SQL_SUCCEEDED(sqlret))
			return sqlret;
	}
	if (tbl.GetColumnIndex(_T("itemformat")) < 0) {
		linguversa::Query altertable(&con);
		sqlret = altertable.ExecDirect(
			_T("alter table clipitem add itemformat int null;"));
		if (!SQL_SUCCEEDED(sqlret))
			return sqlret;
	}
	if (tbl.GetColumnIndex(_T("formatname")) < 0) {
		linguversa::Query altertable(&con);
		sqlret = altertable.ExecDirect(
			_T("alter table clipitem add formatname varchar(255) null;"));
		if (!SQL_SUCCEEDED(sqlret))
			return sqlret;
	}
	if (tbl.GetColumnIndex(_T("modified")) < 0) {
		linguversa::Query altertable(&con);
		// different SQL types for date/time in various DBMS
		if (dbms_name.substr(0,10) == _T("PostgreSQL") || drivername.substr(0,8) == _T("PSQLODBC"))
			sqlret = altertable.ExecDirect(
				_T("alter table clipitem add modified timestamp null;"));
		else
			sqlret = altertable.ExecDirect(
				_T("alter table clipitem add modified datetime null;"));
		if (!SQL_SUCCEEDED(sqlret))
			return sqlret;
	}

	return sqlret;
}

tstring create_ancestors(const HWND hWnd, DATA_INFO* pdi, linguversa::Query& create_folder)
{
	db_item_array ancestors;
	while (pdi != nullptr && pdi->type != TYPE_ROOT) {
		pdi = (DATA_INFO*)SendMessage(hWnd, WM_ITEM_GET_PARENT, 0, (LPARAM)pdi);
		if (pdi && pdi->type == TYPE_FOLDER) {
			db_item* dbi = new db_item();
			dbi->itemtype = TYPE_FOLDER;
			dbi->title = pdi->title ? pdi->title : pdi->menu_title;
			ancestors.push_back(dbi);
		}
	}

	tstring parent;
	size_t n = ancestors.size();
	while (n > 0) {
		db_item* dbi = ancestors[--n];
		if (dbi->title.empty())
			break;

		// create folder item in db
		SQLRETURN sqlret = SQL_SUCCESS;
		sqlret = create_folder.SetParamValue(1, dbi->title.substr(0, 255));
		sqlret = create_folder.SetParamValue(2, parent);
		sqlret = create_folder.SetParamValue(3, (long)9999); // append at the end
		sqlret = create_folder.SetParamValue(3, (long)3);
		sqlret = create_folder.SetParamNull(4);
		sqlret = create_folder.SetParamNull(5);
		sqlret = create_folder.SetParamNull(6);
		sqlret = create_folder.SetParamNull(7);
		sqlret = create_folder.SetParamNull(8);
		sqlret = create_folder.Execute();

		parent = dbi->title;
	}

	return parent;
}

int item_to_db(TOOL_DATA_INFO* tdi, tstring parent, linguversa::Query& statement)
{
	TOOL_DATA_INFO* wk_tdi;
	tstring title;
	RETCODE retcode;
	const TCHAR odbc_save_unicode[] = _T("UNICODE TEXT");
	const TCHAR odbc_save_multibyte[] = _T("TEXT");

	for (int i = 1; tdi != NULL; tdi = tdi->next, i++) {
		if (tdi->di == nullptr)
			continue;
		// convert di->modified (FILETIME) to TIMESTAMP_STRUCT
		TIMESTAMP_STRUCT ts = { 0,0,0, 0,0,0, 0 };
		bool withTS = FileTimeToTimestampStruct(tdi->di->modified, ts);

		title.assign(tdi->di->title ? tdi->di->title : _T(""));
		if (tdi->di->type == TYPE_FOLDER || tdi->di->type == TYPE_ROOT) {
			if (!title.empty()) {
				// save folder item to db
				retcode = statement.SetParamValue(1, title.substr(0,255));
				retcode = statement.SetParamValue(2, parent);
				retcode = statement.SetParamValue(3, (long)i);
				retcode = statement.SetParamValue(4, (long)tdi->di->type);
				retcode = statement.SetParamValue(5, (long)tdi->di->format);
				retcode = statement.SetParamNull(6);
				retcode = statement.SetParamNull(7);
				retcode = withTS ? statement.SetParamValue(8, ts) : statement.SetParamNull(8);
				retcode = statement.Execute();
			}
			if (tdi->child) {
				// save all child items of folder (recursion)
				int ret = item_to_db(tdi->child, title.substr(0,255), statement);
			}
			continue;
		}
		else if (tdi->di->type == TYPE_ITEM) {
			bool bFound = false;
			for (wk_tdi = tdi->child; wk_tdi != NULL; wk_tdi = wk_tdi->next)
				// for the moment we save only odbc_save_unicode; TODO: save all formats within item?
				if (wk_tdi->di && wk_tdi->di->format == CF_UNICODETEXT) {
					bFound = true;
					break;
				}
			if (!bFound) {
				for (wk_tdi = tdi->child; wk_tdi != NULL; wk_tdi = wk_tdi->next)
					// for the moment we save only odbc_save_unicode; TODO: save all formats within item?
					if (wk_tdi->di && wk_tdi->di->format == CF_TEXT) {
						break;
					}
			}
			if (wk_tdi == NULL) {
				continue;
			}
		}
		else if (tdi->di->type == TYPE_DATA) {
			if (!tdi->di->format_name)
				continue;
			if (tdi->di->format != CF_UNICODETEXT && tdi->di->format != CF_TEXT)
				continue;
			if (lstrcmpi(odbc_save_unicode, tdi->di->format_name) != 0
				&& lstrcmpi(odbc_save_multibyte, tdi->di->format_name) != 0)
			{
				continue;
			}
			wk_tdi = tdi;
		}
		else {
			continue;
		}

		if (!wk_tdi || !wk_tdi->di) {
			return TOOL_ERROR;
		}

		DATA_INFO& di = *wk_tdi->di;
		wstring wsdata;
		string mbdata;
		if (!di.format_name) {
			continue;
		}
		else if (di.format == CF_UNICODETEXT) {
			wchar_t* ws = nullptr;
			if ((ws = (wchar_t*)GlobalLock(di.data)) == NULL) {
				return TOOL_ERROR;
			}
			wsdata.assign(ws);
			GlobalUnlock(di.data);
		}
		else if (di.format == CF_TEXT) {
			char* s = nullptr;
			if ((s = (char*)GlobalLock(di.data)) == NULL) {
				return TOOL_ERROR;
			}
			mbdata.assign(s);
			int cnt;
			int len = strlen(s) + 1;
			wchar_t* ws = new wchar_t[len];
			cnt = ::MultiByteToWideChar(CP_THREAD_ACP, 0, s, len, ws, len);
			if (cnt <= 0) {	// error
				delete[] ws;
				return TOOL_ERROR;
			}

			wsdata.assign(ws);
			delete[] ws;
			GlobalUnlock(di.data);
		}

		if (title.empty())
			title.assign(di.menu_title ? di.menu_title : wsdata.substr(0, 255));
		retcode = statement.SetParamValue(1, title.substr(0,255));
		retcode = statement.SetParamValue(2, parent);
		retcode = statement.SetParamValue(3, (long)i);
		retcode = statement.SetParamValue(4, (long)di.type);
		retcode = statement.SetParamValue(5, (long)di.format);
		retcode = statement.SetParamValue(6, di.format_name);
		retcode = statement.SetParamValue(7, wsdata);
		retcode = withTS ? statement.SetParamValue(8, ts) : statement.SetParamNull(8);
		retcode = statement.Execute();

		if (!SQL_SUCCEEDED(retcode))
		{
			// TODO:
			//throw statement.ThrowException(retcode);
			return TOOL_ERROR;
		}
	}

	return TOOL_SUCCEED;
}

bool FileTimeToTimestampStruct(const FILETIME ft, TIMESTAMP_STRUCT& ts)
{
	SYSTEMTIME st;
	if (ft.dwHighDateTime != 0 && ft.dwLowDateTime != 0
		&& FileTimeToSystemTime(&ft, &st))
	{
		ts.year = st.wYear;
		ts.month = st.wMonth;
		ts.day = st.wDay;
		ts.hour = st.wHour;
		ts.minute = st.wMinute;
		ts.second = st.wSecond;
		ts.fraction = st.wMilliseconds * 1000000; // ts.fraction is nanoseconds
		return true;
	}

	ts = { 0,0,0, 0,0,0, 0 };
	return false;
}

bool TimestampStructToFileTime(const TIMESTAMP_STRUCT ts, FILETIME& ft)
{
	SYSTEMTIME st;
	if (ts.year > 0 && ts.month > 0 && ts.day > 0) {
		st.wYear = ts.year;
		st.wMonth = ts.month;
		st.wDay = ts.day;
		st.wHour = ts.hour;
		st.wMinute = ts.minute;
		st.wSecond = ts.second;
		st.wMilliseconds = (WORD)(ts.fraction / 1000000); // ts.fraction is nanoseconds
		if (SystemTimeToFileTime(&st, &ft))
			return true;
	}

	ft.dwHighDateTime = ft.dwLowDateTime = 0;
	return false;
}

int sync_db_item(const db_item& dbi, DATA_INFO* item, DATA_INFO* pd)
{
	if (item == nullptr)
		return TOOL_ERROR;

	if (item->title != nullptr && _tcscmp(item->title, dbi.title.c_str()) != 0) {
		mem_free((void**)&item->title);
		item->title = alloc_copy(dbi.title.c_str());
	}
	else if (item->title == nullptr) {
		item->title = alloc_copy(dbi.title.c_str());
	}

	if (dbi.itemtype == TYPE_FOLDER && item->type == TYPE_FOLDER) {
		return TOOL_SUCCEED;
	}

	if (pd && pd->type == TYPE_DATA)
	{
		// now copy dbi to struct members 
		pd->format = dbi.itemformat;

		if (pd->format_name && _tcscmp(pd->format_name, dbi.formatname.c_str()) != 0) {
			mem_free((void**)&pd->format_name);
			pd->format_name = alloc_copy(dbi.formatname.c_str());
		}
		else if (pd->format_name == nullptr) {
			pd->format_name = alloc_copy(dbi.formatname.c_str());
		}

		string mbdata;
		wstring wsdata;
		size_t len = 0;
		FILETIME dbft;
		long cmp = 0;
		switch (pd->format) {
		case CF_UNICODETEXT:
			if (pd->data != nullptr && (len = pd->size) > 0) {
				wchar_t* ws = nullptr;
				if ((ws = (wchar_t*)GlobalLock(pd->data)) == NULL) {
					return TOOL_ERROR;
				}
				wsdata.assign(ws);
				GlobalUnlock(pd->data);
			}
			// dbft newer than item->modified ?
			if (TimestampStructToFileTime(dbi.modified, dbft) && (cmp = CompareFileTime(&dbft, &(item->modified))) != 0 && cmp > 0
				|| dbft.dwHighDateTime == 0 && dbft.dwLowDateTime == 0) 
			{
				// allocate (dbi.textcontent.size() + 1) * sizeof(TCHAR) bytes in pd->data and
				// copy dbi.textcontent to the allocated memory
				HGLOBAL ret = NULL;
				TCHAR* to_mem = nullptr;
				size_t targetlen = dbi.textcontent.size();
				// reserve memory for copy target
				if ((ret = GlobalAlloc(GHND, (targetlen + 1) * sizeof(TCHAR))) == NULL) {
					return TOOL_ERROR;
				}
				// lock copy target
				if ((to_mem = (TCHAR*) GlobalLock(ret)) == NULL) {
					GlobalFree(ret);
					return TOOL_ERROR;
				}

				// copying to target
				for (size_t i = 0; i < targetlen; i++) {
					to_mem[i] = dbi.textcontent[i];
				}
				to_mem[targetlen] = L'\0';

				GlobalUnlock(ret);
				GlobalFree(pd->data);
				pd->data = ret;
				pd->size = (targetlen + 1) * sizeof(TCHAR);
				if (dbft.dwHighDateTime == 0 && dbft.dwLowDateTime == 0) {
					TIMESTAMP_STRUCT ts = { 2000, 01, 01, 00, 00, 00, 0 };
					TimestampStructToFileTime( ts, dbft);
				};
				
				item->modified = dbft;
			}
			break;
		case CF_TEXT:
			if (pd->data != nullptr && (len = pd->size) > 0) {
				char* s = nullptr;
				if ((s = (char*)GlobalLock(pd->data)) == NULL) {
					return TOOL_ERROR;
				}
				mbdata.assign(s);
				int len = strlen(s) + 1;
				wchar_t* ws = new wchar_t[len];
				int cnt = ::MultiByteToWideChar(CP_THREAD_ACP, 0, s, len, ws, len);
				if (cnt <= 0) {	// error
					delete[] ws;
					return TOOL_ERROR;
				}

				wsdata.assign(ws);
				delete[] ws;
				GlobalUnlock(pd->data);
			}

			// dbft newer than item->modified ?
			if (TimestampStructToFileTime(dbi.modified, dbft) && (cmp = CompareFileTime(&dbft, &(item->modified))) != 0 && cmp > 0) {
				// convert dbi.textcontent to multi_byte
				int len = dbi.textcontent.length() + 1;
				char* mb = new char[len * 4];
				BOOL bUsedDefaultChar = FALSE;
				int cnt = ::WideCharToMultiByte(CP_THREAD_ACP, 0, dbi.textcontent.c_str(), len, mb, len * 4, NULL, &bUsedDefaultChar);
				if (cnt <= 0 || cnt != strlen(mb) + 1) {
					delete[] mb;
					return TOOL_ERROR;
				}
				HGLOBAL ret = NULL;
				char* to_mem = nullptr;
				size_t targetlen = strlen(mb);
				// reserve memory for copy target
				if ((ret = GlobalAlloc(GHND, targetlen + 1)) == NULL) {
					delete[] mb;
					return TOOL_ERROR;
				}
				// lock copy target
				if ((to_mem = (char*)GlobalLock(ret)) == NULL) {
					delete[] mb;
					GlobalFree(ret);
					return TOOL_ERROR;
				}

				// copying to target
				for (size_t i = 0; i < targetlen; i++) {
					to_mem[i] = mb[i];
				}
				to_mem[targetlen] = '\0';

				delete[] mb;
				GlobalUnlock(ret);
				GlobalFree(pd->data);
				pd->data = ret;
				pd->size = targetlen + 1;
				item->modified = dbft;
			}
			break;
		}
		return TOOL_SUCCEED;
	}
	else {
		return TOOL_ERROR;
	}

	return TOOL_SUCCEED;
}

