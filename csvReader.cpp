#include "stdafx.h"
#include "./csvReader.hpp"
#include <boost/scope_exit.hpp>

namespace csv {

static char FIELD_STORAGE[1024];
static const char CSV_FIELD_SEPARATOR = ',';
bool FieldReader::nextCsvField( ConversionRange& strOut ) const {
    char* out = FIELD_STORAGE;

    BOOST_SCOPE_EXIT( &strOut, &out ) {
        *out = '\0';
        strOut = ConversionRange( FIELD_STORAGE, out);
    }BOOST_SCOPE_EXIT_END;


    if ( _line.end() == _p ) {
        return false;
    }


    const char* orig = _p;
    const bool quoted = ( '"' == *_p );
    if ( quoted ) {
        ++_p;
    }


    bool quote = false;

    for ( ;; ++_p ) {
        if ( _line.end() == _p ) {
            return true;
        }

        if ( CSV_FIELD_SEPARATOR == *_p && !quoted ) {
            break;
        }

        if ( '"' != *_p ) {
            *out++ = *_p;
        } else { // Current character is quote
            const char lastChar = ( _line.end( ) == _p + 1 );
            if ( !lastChar ) {
                if ( '"' == _p[1] ) { // Double quote
                    *out++ ='"';
                    ++_p;
                    continue;
                }
                // Enclosing quote
                ++_p;
                break;
            } else { // Last char in a line
                if ( quoted ) {
                    ++_p;
                    break;
                } else { // Ends with single " but not quoted
                    throw NotValidField( orig );
                }
            }
        }
    }

    if ( CSV_FIELD_SEPARATOR == *_p ) {
        ++_p;
    } else if ( _line.end( ) != _p ) {
        throw NotValidField( orig );
    }
    return true;
}

void Convert( const ConversionRange& src, uint32_t& dst ) {
    dst = readDecimal( src );
}

void Convert( const ConversionRange& src, string& dst ) {
    dst.assign(src.begin(), src.end());
}

void Convert( const ConversionRange& src, double& dst ) {
    dst = readDouble( src );
}

void Convert( const ConversionRange& src, bool& dst ) {
    dst = readBoolean( src );
}

} // namespace csv

