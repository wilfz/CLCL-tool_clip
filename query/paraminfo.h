#pragma once

#include "tstring.h"

#include <sql.h>
#include <sqlext.h>

using namespace std;

namespace linguversa
{
    // Element of Implementation Parameter Descriptor (IPD):
    // Contains information about the parameter in an SQL statement..
    class ParamInfo
    {
    public:
        // members of CODBCParamInfo
        SWORD m_nSQLType;
        SQLULEN m_nPrecision;
        SWORD m_nScale;
        SWORD m_nNullability;

        typedef enum {
            unknown = SQL_PARAM_TYPE_UNKNOWN,     //  0
            input = SQL_PARAM_INPUT,              //  1
            inputoutput = SQL_PARAM_INPUT_OUTPUT, //  2
            resultcoumn = SQL_RESULT_COL,         //  3
            output = SQL_PARAM_OUTPUT,            //  4
            returnvalue = SQL_RETURN_VALUE        //  5
        } InputOutputType;

        // other members
        InputOutputType m_InputOutputType;
        // nur für stored proceduers
        tstring m_ParamName;

        ParamInfo()
        {
            m_nSQLType = SQL_UNKNOWN_TYPE;
            m_nPrecision = 0;
            m_nScale = 0;
            m_nNullability = 0;
            m_InputOutputType = unknown;
        }
    };
    
}
