#pragma once

#include "tstring.h"
#include <vector>
#include "dbitem.h"
#include "resultinfo.h"

namespace linguversa
{
class DataRow : public std::vector<DBItem>
{
public:
    //LwDataRow();
    //LwDataRow( const LwDataRow& src);
    //const LwDataRow& operator = ( const LwDataRow& src);
    //bool operator == (const LwDataRow& other) const;
    //bool operator != (const LwDataRow& other) const;

    std::tstring Format( const ResultInfo& resultinfo, const std::tstring fmt = _T("")) const;

protected:
    //ResultInfo* m_pResultInfo;
};

}
