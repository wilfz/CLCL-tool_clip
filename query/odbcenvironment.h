#pragma once

#include "tstring.h"

#include <sql.h>
#include "odbcexception.h"
#include <vector>

namespace linguversa
{
    class ODBCEnvironment
    {
    public:
        ODBCEnvironment();
        ~ODBCEnvironment();
        
        SQLRETURN FetchDriverName(std::tstring& driverdescription, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        SQLRETURN FetchDriverInfo(std::tstring& driverdescription, std::vector<std::tstring>& attributes, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        SQLRETURN FetchDataSourceInfo(std::tstring& dsn, std::tstring& drivername, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        
    protected:
        HENV m_henv;
    };
}
