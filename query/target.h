#pragma once

#include "query.h"
#include <fstream>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#ifdef UNICODE
#define tcout wcout
#define tostream wostream
#define tofstream wofstream
#define tstringstream wstringstream
#define tstreambuf wstreambuf
#else
#define tcout cout
#define tostream ostream
#define tofstream ofstream
#define tstringstream stringstream
#define tstreambuf streambuf
#endif
#else
#define tcout cout
#define tostream ostream
#define tofstream ofstream
#define tstringstream stringstream
#define tstreambuf streambuf
#endif


namespace linguversa
{
    class ResultGroup
    {
    protected:
        uint64_t selector;
        std::tstring firstRowFormat;
        std::tstring eachRowFormat;
        std::tstring lastRowFormat;
        //DataRow aggregation;
    };

    class Target
    {
    public:
        Target();
        ~Target();

        typedef enum {
            sel_stdout,
            sel_fileout,
            sel_odbc
        } selector_type;

        bool OpenStdOutput();
        bool OpenConnection(std::tstring connection);
        bool OpenFile(std::tstring filepath);
        bool IsOpen();
        void Close();

        bool Apply(std::tstring str);
        void Create( const Query& query, tstring tablename);
        void InsertCurrentRow(Query& query, tstring tablename);
        void InsertAll(Query&, tstring tablename);
        void OutputAsCSV(Query& query, const tstring& fieldseparator);

    protected:
        selector_type selector;
        std::tofstream m_outstream;
        linguversa::Connection m_connnection;
    };


    class TargetStream : public std::tostream
    {
    public:
    	// default constructor
        TargetStream() : std::tostream(nullptr) { _pCon = nullptr; };
        TargetStream( std::tstreambuf* pbuf) : std::tostream(pbuf) { _pCon = nullptr; };
        TargetStream( linguversa::Connection& con);

        void SetConnection( linguversa::Connection& con);

        void OutputAsCSV( linguversa::Query& query, const tstring fieldseparator, 
            const tstring decimalformat = _T(""), const tstring datetimeformat = _T(""));
        void OutputFormatted( linguversa::Query& query, tstring rowformat);
        void CreateTable( const linguversa::Query& query, tstring tablename);
        void InsertAll( linguversa::Query& query, tstring tablename);

        bool IsODBC() { return (_pCon != nullptr); };
        SQLRETURN Apply();

    private:
        tstringstream _strstream;
        Connection* _pCon;
    };
}
