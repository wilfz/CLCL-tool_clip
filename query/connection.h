#pragma once

#include "tstring.h"

#include <sql.h>
#include <sqlext.h>
#include "odbcexception.h"

namespace linguversa
{

class Connection
{
public:
    Connection();
    ~Connection();
    
    bool Open(std::tstring connectionstring);
    bool Open(std::tstring& connectionstring, HWND hWnd, SQLUSMALLINT driverCompletion = SQL_DRIVER_COMPLETE);
    bool IsOpen() const;
    void Close();
    
    // valid InfoTypes are:
    // SQL_DATA_SOURCE_NAME, SQL_DATABASE_NAME, SQL_USER_NAME, SQL_DBMS_NAME, SQL_DBMS_VER, 
    // SQL_DRIVER_NAME, SQL_DRIVER_VER, SQL_DRIVER_ODBC_VER, SQL_ODBC_VER
    SQLRETURN SqlGetInfo(SQLUSMALLINT InfoType, std::tstring& info) const;
    SQLRETURN SqlGetDriverName(std::tstring& drivername) const
        { return SqlGetInfo(SQL_DRIVER_NAME, drivername); };
    SQLRETURN SqlGetDriverVersion(std::tstring& sDriverVersion) const
        { return SqlGetInfo(SQL_DRIVER_VER, sDriverVersion); };
    
    HENV GetSqlHEnv() const { return m_henv;};
    HDBC GetSqlHDbc() const { return m_hdbc;};

protected:
    static HENV m_henv;
    static int m_ConnectionCounter;
    HDBC m_hdbc;
};

}

