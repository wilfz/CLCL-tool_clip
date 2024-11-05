#pragma once

#include <vector>
#include "fieldinfo.h"

namespace linguversa
{
    class ResultInfo : public std::vector<FieldInfo>
    {
    public:
        int GetSqlColumn(std::tstring colname) const;        
    };
}
