#pragma once

#include "tstring.h"
// only for the formatting
#include <memory>
#include <stdexcept>
#include <sql.h>

// helper to manipulate strings in MFC style
namespace linguversa
{

// The following template function is a snippet from 
// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
// under the CC0 1.0. (https://creativecommons.org/publicdomain/zero/1.0/)
template<typename ... Args>
std::tstring string_format(const std::tstring& format, Args ... args)
{
#ifdef UNICODE
    #ifdef _MSC_VER
    __pragma(warning(disable : 4996))
    #endif
    int size_s = _snwprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
    _snwprintf(buf.get(), size, format.c_str(), args ...);
#else
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
#endif
    return std::tstring(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

std::tstring lower(const std::tstring& str);
std::tstring upper(const std::tstring& str);
std::tstring FormatTimeStamp(const std::tstring& colFmt, const TIMESTAMP_STRUCT* pdt);

class lvstring : public std::tstring
{
public:
    lvstring() {};
    lvstring(std::tstring s) : std::tstring(s) {};
    lvstring(const TCHAR s[]) : std::tstring(s) {};
    
    void Replace(const std::tstring from, const std::tstring to);
    std::tstring Left(const size_t n) const;
    std::tstring Right(const size_t n) const;
    std::tstring Mid(const size_t from, const size_t cnt) const;
    std::tstring& MakeLower();
    std::tstring& MakeUpper();
};

}
