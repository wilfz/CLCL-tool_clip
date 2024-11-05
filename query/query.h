#pragma once

#include "connection.h"
#include "dbitem.h"
#include "fieldinfo.h"
#include "resultinfo.h"
#include "paraminfo.h"

#define USE_ROWDATA
#ifdef USE_ROWDATA
#include "datarow.h"
#endif
#include <map>

using namespace std;

namespace linguversa
{
    
class ParamItem;

class Query
{
public:
    Query();
    Query( Connection* pConnection);
    Query( Connection& connection);
    ~Query();

    // Set the database handle to a Connection object which encapsulates the ODBC connection.
    void SetDatabase( Connection* pConnection);
    void SetDatabase( Connection& connection);

    // Execute an arbitrary sql statement; on success the function returns SQL_SUCCESS, otherwise one of the return codes defined in <sql.h>.
    // If the statement contains question mark placeholders it is obligatory to bind a host variable for each placeholder(see below) 
    // before calling ExecDirect().
    SQLRETURN ExecDirect( tstring statement);

    // Finish the statement and release all its resources.
    SQLRETURN Close();

    // Gets the number of affected rows. Only for such statements as delete or update which do not return a result set.
    // For other statements which return a result set (e.g. select) the row count may be set by some ODBC drivers and 
    // not set for other drivers.
    SQLRETURN GetRowsAffected( SQLLEN& nRowCount);

    // Gets the number of columns in the result set. If there is no result set at all (e.g. insert, update) the
    // function returns 0.
    short GetODBCFieldCount() const;

    // Returns the 0-based column index which corresponds to the column named lpszFieldName, 
    // -1 if no column name matches.
    int GetFieldIndexByName(tstring lpszFieldName) const;

    // Retrieves information about column type and size. See Visual Studio / MFC help for further information 
    // on CODBCFieldInfo.
    void GetODBCFieldInfo( short nIndex, FieldInfo& fieldinfo) const;

    // Retrieves information about column name, type and size. See Visual Studio / MFC help for further information 
    // on CODBCFieldInfo. Returns false if no column with the specified column name exists.
    bool GetODBCFieldInfo( tstring lpszName, FieldInfo& fieldinfo) const ;

    // Some engines (like SQLite) do not always know the complete Fieldinfo. 
    // In those cases the application program should know and 
    // be able to tell the query if necessary.
    void SetODBCFieldInfo( short nIndex, const FieldInfo& fieldinfo);
    bool SetODBCFieldInfo( tstring lpszName, const FieldInfo& fieldinfo);
    void SetColumnSqlType( short nIndex, SWORD nSQLType);
    bool SetColumnSqlType( tstring lpszName, SWORD nSQLType);

    // TODO: mapping from DBItem type selector m_vartype to the according C type
    static SQLSMALLINT GetCType(DBItem::vartype vt);

    // Fetches the next row of the resultset. If successful it returns SQL_SUCCESS or SQL_SUCCESS_WITH_INFO. 
    // If there are no more rows in the result (after the last row) set it returns SQL_NO_DATA_FOUND. 
    // No explicit column binding to any host variable!
    // TODO: Since there are apparently no other possible return values than SQL_SUCCESS(_WITH_INFO) and 
    //       SQL_NO_DATA_FOUND it may prove better to get rid of the ugly ODBC macros and 
    //       return true instead of SQL_SUCCESS and false instead of SQL_NO_DATA_FOUND.
    SQLRETURN Fetch();

    void Cancel();

    // ODBC allows multiple result sets for one query. After reading the last row of a result set 
    // SQLMoreResults() asks whether there is another result set pending. 
    // Return value SQL_SUCCESS (or SQL_SUCCESS_WITH_INFO) means there is another result set to be read. 
    // SQL_NO_DATA_FOUND means all result sets have already been read. 
    // SQLMoreResults() also re-initializes the ODBCFieldInfos according to the new result set. 
    // TODO: Since there are apparently no other possible return values than SQL_SUCCESS(_WITH_INFO) and 
    //       SQL_NO_DATA_FOUND it may prove better to get rid of the ugly ODBC macros and 
    //       return true instead of SQL_SUCCESS and false instead of SQL_NO_DATA_FOUND.
    SQLRETURN SQLMoreResults();

    // The following functions retrieve the value of the specified column in the current row after Fetch(). 
    // Most sql data types can be converted to string.
    // If a column name is specified that does not exist, the function returns false.
    bool GetFieldValue( tstring lpszName, tstring& sValue );
    bool GetFieldValue( short nIndex, string& sValue );
    bool GetFieldValue( short nIndex, wstring& sValue );

    // Retrieve the value of the specified column in the current row into a variable of the respective type. 
    // Returns false if no column with name lpszName exists or if the sql type of the column cannot be converted into 
    // the target type. See the conversion matrix in Visual Studio help.
    bool GetFieldValue( tstring lpszName, long& lValue);
    bool GetFieldValue( tstring lpszName, int& iValue);
    bool GetFieldValue( tstring lpszName, short& siValue);
    bool GetFieldValue( tstring lpszName, bytearray& ba);
    bool GetFieldValue( tstring lpszName, unsigned ODBCINT64& ui64Value);
    bool GetFieldValue( tstring lpszName, double& dValue);
    bool GetFieldValue( tstring lpszName, SQLGUID& guid);
    bool GetFieldValue(tstring lpszName, TIMESTAMP_STRUCT& tsValue);
    bool GetFieldValue( short nIndex, long& lValue);
    bool GetFieldValue( short nIndex, int& iValue);
    bool GetFieldValue( short nIndex, short& siValue);
    bool GetFieldValue( short nIndex, bytearray& ba);
    bool GetFieldValue( short nIndex, unsigned ODBCINT64& ui64Value);
    bool GetFieldValue( short nIndex, double& dValue);
    bool GetFieldValue( short nIndex, SQLGUID& guid);
    bool GetFieldValue( short nIndex, TIMESTAMP_STRUCT& tsValue);

    // Retrieve the value of the specified column in the current row into a variant type DBItem. 
    // Returns false if no column with name lpszName exists.
    // DBItem is a helper class which supports 
    //  - additional data types, 
    //  - formatting into tstring, 
    //  - assignment and comparison operators. 
    bool GetFieldValue(short nIndex, DBItem& varValue, DBItem::vartype itemtype);
    bool GetFieldValue(tstring lpszName, DBItem& varValue, DBItem::vartype itemtype);

    // Actually all GetFieldValue() functions internally use the following functions 
    // and cache the data in a protected member which is an array of DBItem.
    // Be careful when specifying nFieldTyp: these are simply positive or even neagtive numbers.
    // But only few number indicate an existing data type, and even those referring to valid
    // types might not be implemented in this class or in your ODBC driver.
    void GetFieldValue( short nIndex, DBItem& varValue, short nFieldType = DEFAULT_FIELD_TYPE);
    bool GetFieldValue( tstring lpszName, DBItem& varValue, short nFieldType = DEFAULT_FIELD_TYPE);

    // If true the data field in the current row has been read and value or null indicator is initialized.
    bool IsFieldInit( short nIndex);
    bool IsFieldInit( tstring lpszName);
    // If true the column with nIndex has been read and holds the db null value.
    bool IsFieldNull( short nIndex);
    bool IsFieldNull( tstring lpszName);

    // set the format for various CTypes
    void SetCTypeFormat(const signed short ctype, const tstring fmt);
    // TODO: format the value of the specified column as string, according to its FieldInfo.
    tstring FormatFieldValue( short nIndex);
    tstring FormatFieldValue( tstring lpszName);

    // formatting of output
    tstring FormatCurrentRow(const std::tstring);

    // Set an arbitrary SQL statement which is to be executed. 
    // The statement may contain question marks as placeholders for variables which have to 
    // be bound to the statement according to their types and the sql type in the statement. 
    // For details see the conversion matrix in Visual Studio help.
    SQLRETURN Prepare(tstring statement);

    // Execute the statement which was previously set with Prepare(). 
    // If you want to execute the same statement with different parameter values, it is sufficient to 
    // assign the new values to the already bound host variables and call Execute() again. This is 
    // safer and also more performant than building new query strings and calling them by ExecuteDirect().
    SQLRETURN Execute();

    short GetParamCount() const;
    RETCODE GetParamInfo(SQLUSMALLINT ParameterNumber, ParamInfo& paraminfo);
    
    // Returns parameter index which corresponds to the parameter named ParameterName, 
    // -1 if no parameter name matches.
    int GetParamIndexByName(tstring ParameterName);

    // Set the name of a not yet named parameter
    SQLRETURN SetParamName(SQLUSMALLINT ParameterNumber, tstring ParameterName);
    
    // If there has been no previous Prepare() the host application itself must supply information on all parameters of the query.
    // For positional parameters ParammeterName should be empty.
    // Return value is the actual number of the parameter or -1 in case of error.
    int AddParameter(tstring ParameterName, SQLSMALLINT nSqlType, UDWORD nLength = 0, SWORD nScale = 0, SWORD nNullable = 1, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
    int AddParameter(const ParamInfo& paraminfo);
    
    // Some ODBC drivers (MySQL) are not capable of retrieving correct SQL type / ParamInfo.
    // In this case the application is able to override the (incorrect) information from the driver.
    SQLRETURN SetParamType(SQLUSMALLINT ParameterNumber, SQLSMALLINT nSqlType, UDWORD nLength = 0, SWORD nScale = 0, SWORD nNullable = 1, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
    SQLRETURN SetParamType(SQLUSMALLINT ParameterNumber, const ParamInfo& paraminfo);

	// Apparently MySQL's ODBC-driver does not set inout or output parameter by itself.
	// Instead it returns an additional last resultset consisting of the inout and output parameters 
	// in the same order as in the procedure's signature.
    // https://dev.mysql.com/doc/c-api/8.0/en/c-api-prepared-call-statements.html
	// I do a hack here to compensate for what I consider a bug in the driver.
	// For other database engines and drivers this function reads past the last result set
    // in order to make the set the output parameters.
    SQLRETURN Finish();

	// Host variable is of C++ type short(16 bit integer) which corresponds to the ODBC sql type SQL_SMALLINT.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, short& nParamRef, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);	// 16 bit, SQL_SMALLINT
	// Host variable is of C++ type int(32 bit integer) which corresponds to the ODBC sql type SQL_INTEGER.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, int& nParamRef, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);	// 32 bit, SQL_INTEGER
	virtual RETCODE BindParameter(tstring ParameterName, int& nParamRef, ParamInfo::InputOutputType inouttype = ParamInfo::input);	// 32 bit, SQL_INTEGER
	// Host variable is of C++ type long (32 bit integer) which corresponds to the ODBC sql type SQL_INTEGER.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, long& nParamRef, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);	// 32 bit, SQL_INTEGER
	RETCODE BindParameter(tstring ParameterName, long& nParamRef, ParamInfo::InputOutputType inouttype = ParamInfo::input);	// 32 bit, SQL_INTEGER
	// Host variable is of C++ type __int64 (64 bit integer) which corresponds to the ODBC sql type SQL_BIGINT.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, unsigned ODBCINT64& nParamRef, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);	// 64 bit, SQL_BIGINT
	// Host variable is of C++ type double.Length in memory of double is 64 bit integer.
	// It maybe used for ODBC sql type SQL_DOUBLE but also for SQL_DECIMAL, SQL_NUMERIC, or SQL_FLOAT.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, double& sParamRef, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);	// 64 bit
	// Host variable is a TIMESTAMP_STRUCT. It can be used for sql types SQL_DATETIME.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, TIMESTAMP_STRUCT& tsParamRef, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
	// Host variable is of C++ type SQLGUID (identically to GUID . It can be used for sql types SQL_DATETIME.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, SQLGUID& guid, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
	// Host variable is a buffer of _TCHARs. Length in memory depends. 
	// It may be used for ODBC sql types SQL_CHAR, SQL_VARCHAR, SQL_WCHAR, SQL_WVARCHAR. 
	// If inouttype is ParamInfo::inputoutput or ParamInfo::output the buffer must be big enough to receive the maximal possible length to be written,
    // in case of stored procedures at least as large as the maximum length of the corresponding stored proc parameter.
	// The buffer length has to be set in the bufParamlen parameter.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, TCHAR* bufParamRef, SQLLEN bufParamlen = 0, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
	RETCODE BindParameter(tstring ParameterName, TCHAR* bufParamRef, SQLLEN bufParamlen = 0, ParamInfo::InputOutputType inouttype = ParamInfo::input);
	// Host variable is a buffer of BYTEs. Length in memory depends. It is very similar to the previous function except that there is no terminating 0. 
	// It may be used for ODBC sql types SQL_BINARY, SQL_VARBINARY. 
	// paramDataLen is the length of input data or the special value SQL_NULL_DATA to indicate input is null.
	// The buffer size has to be set in the paramBufLen parameter and must be at least as big as the input data length.
	// For inouttype is ParamInfo::inputoutput or ParamInfo::output the buffer size bufParamLen must be at least as large as the maximum length of the 
	// corresponding stored proc parameter.
	// For inouttype = ParamInfo::input bufParamlen can be omitted and it will be used 0 or, if positive, the value of paramDataLen.
	RETCODE BindParameter(SQLUSMALLINT ParameterNumber, BYTE* pBa, SQLLEN paramDataLen, SQLLEN paramBufLen = 0, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
	// Because ParamInfo does not include the actual paramDataLenth, we need a function to retrieve the data length of inout and output parameters
	// after execution.
	SQLLEN GetParamDataLength(SQLUSMALLINT ParameterNumber);

	RETCODE SetParamValue(SQLUSMALLINT ParameterNumber, long nParamValue, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
	RETCODE SetParamValue(SQLUSMALLINT ParameterNumber, double dParamValue, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
	RETCODE SetParamValue(SQLUSMALLINT ParameterNumber, TIMESTAMP_STRUCT tsParamValue, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
	RETCODE SetParamValue(SQLUSMALLINT ParameterNumber, SQLGUID guid, ParamInfo::InputOutputType inouttype = ParamInfo::unknown);
	// fieldlen is the length of the corresponding sql field, NOT the length of lpszValue. It can be omitted if 
	// the fieldlen has already been set by Prepare() or a previous call of SetParamValue() for the same parameter.
	RETCODE SetParamValue(SQLUSMALLINT ParameterNumber, tstring lpszValue, ParamInfo::InputOutputType inouttype = ParamInfo::unknown, SQLLEN fieldlen = 0);
	RETCODE SetParamValue(SQLUSMALLINT ParameterNumber, const bytearray& ba, ParamInfo::InputOutputType inouttype = ParamInfo::unknown, SQLLEN fieldlen = 0);
    RETCODE SetParamNull(SQLUSMALLINT ParameterNumber);

	bool GetParamValue(SQLUSMALLINT ParameterNumber, long& nParamValue);
	bool GetParamValue(SQLUSMALLINT ParameterNumber, double& dParamValue);
	bool GetParamValue(SQLUSMALLINT ParameterNumber, TIMESTAMP_STRUCT& tsParamValue);
	bool GetParamValue(SQLUSMALLINT ParameterNumber, tstring& sValue);
	bool GetParamValue(SQLUSMALLINT ParameterNumber, SQLGUID& guid);
	bool GetParamValue(SQLUSMALLINT ParameterNumber, bytearray& ba);

	// Set special parameter values, e.g. SQL_NTS (Null terminated tstring), SQL_NULL_DATA (for null value), 
	// SQL_DEFAULT_PARAM (default parameter, only for stored procedure calls).
	void SetParamLenIndicator(SQLUSMALLINT ParameterNumber, SQLLEN lenInd);

protected:
    Connection* m_pConnection;
    //HENV m_henv;
    HDBC m_hdbc;
    HSTMT m_hstmt;
    ResultInfo m_FieldInfo;
    vector<ParamItem*> m_ParamItem;
    std::map<signed short, tstring> m_CTypeFormat;
    // If true map named params to the corresponding positional parameter,
    // otherwise append at the end of m_ParamItem regardless of ordinal position in signature.
    bool m_ParamInitComplete;

    void InitData();
    RETCODE InitFieldInfos();
    bytearray m_RowFieldState; // 0x01: initialized, 0x02: null value

#ifdef USE_ROWDATA
public:
    // Retrieve whole row into an array of DBItems (after previous Fetch().
    void GetCurrentRow( DataRow& currentRow);
protected:
    DataRow m_RowData;
    vector<bool> m_Init;
#endif

    friend class QueryException;
};

class QueryException : public DbException
{
public:
    QueryException(SQLRETURN ret, const Query& q)
        : DbException(ret, SQL_HANDLE_STMT, q.m_hstmt)
    {};
};

}

