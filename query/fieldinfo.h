#pragma once

#include "tstring.h"

#include <sqlext.h>

#define DEFAULT_FIELD_TYPE SQL_TYPE_NULL // pick "C" data type to match SQL data type

namespace linguversa
{
    class FieldInfo
    {
    public:
        FieldInfo();
        ~FieldInfo();

        FieldInfo& operator = (const FieldInfo& src);

        bool operator == (const FieldInfo& other) const;

        void InitData();
        static short GetDefaultCType(const FieldInfo& fi);
        const short GetDefaultCType();
        const std::tstring GetDefaultFormat();

        signed short m_nCType;
        // meta data from ODBC
        std::tstring m_strName;
        SWORD m_nSQLType;
        SQLULEN m_nPrecision;
        SWORD m_nScale;
        SWORD m_nNullability;
    };
}
