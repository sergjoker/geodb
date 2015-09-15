#include "stdafx.h"
#include "./fileReader.hpp"

FileReader::FileReader( const string& path ) {
    _file = ::CreateFileA( path.c_str( ), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    BY_HANDLE_FILE_INFORMATION finfo = { 0 };
    GetFileInformationByHandle( _file, &finfo );
    _mapping = ::CreateFileMappingA( _file, NULL, PAGE_READONLY, 0, 0, NULL );
    _begin = static_cast<const char*>( ::MapViewOfFile( _mapping, FILE_MAP_READ, 0, 0, 0 ) );

    _p = _begin;
    _end = _begin + finfo.nFileSizeLow;
}

FileReader::~FileReader( ) {
    ::UnmapViewOfFile( _begin );
    ::CloseHandle( _mapping );
    ::CloseHandle( _file );
}

bool FileReader::next( ConversionRange& line ) {
    if ( _end == _p ) {
        return false;
    }
    const char* data = _p;
    size_t sz = 0;

    const char* newl = static_cast<const char*>( ::memchr( _p, '\n', _end - _p ) );

    if ( nullptr != newl ) {
        sz = newl - _p;
        if ( '\r' == newl[-1] ) {
            --sz;
        }
    } else { // not found
        sz = _end - _p;
    }
    _p = newl + 1;
    line = ConversionRange( data, data + sz );
    return true;
}