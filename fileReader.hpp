#pragma once
#include <Windows.h>
#include "./common.hpp"

class FileReader
{
public:
    FileReader( const string& path );
    ~FileReader( );
    bool next( ConversionRange& line );
private:
    HANDLE      _file;
    HANDLE      _mapping;
    const char* _p;
    const char* _begin;
    const char* _end;
};