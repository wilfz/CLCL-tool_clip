#pragma once

#include "paraminfo.h"

using namespace std;

namespace linguversa
{
    // internal helper class
    class ParamItem : public ParamInfo
    {
    protected:
        // sql side property
        SQLLEN m_nParamLen;

        // Below are the client side properties for each element of the 
        // Application Parameter Descriptor (APD). 
        // Contains information about the application buffer bound to the parameter in an 
        // SQL statement, such as its address, lengths, and C data type.
        SQLSMALLINT m_nCType; // ODBC C type identifier
        void* m_pParam;       // pointer to the actual data
        SQLLEN m_lenInd;      // data len or special value
        // true: m_pParam was instantiated within Query object,
        // false: pointer m_pParam was set by BindParam()
        //        to a variable outside of Query object
        bool m_local;

        ParamItem();
        ~ParamItem();
        void Clear();
        friend class Query;
        friend class SqlProcedure;
    };
}
