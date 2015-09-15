#pragma once
#include <Windows.h>
#include <boost/scope_exit.hpp>
#include "./common.hpp"
#include "./fileReader.hpp"

namespace csv {

class NotValidField
    : public ParseError
{
public:
    NotValidField( const char* p )
        : ParseError( string( "Not a valid CSV field: " ) + p ) {
    }
};

void Convert( const ConversionRange& src, string& dst);
void Convert( const ConversionRange& src, double& dst );
void Convert( const ConversionRange& src, uint32_t& dst );
void Convert( const ConversionRange& src, bool& dst );


template<typename T,typename... Args >
void Convert( const ConversionRange& src,boost::flyweight<T,Args...>& dst ) {
    dst = T(src.begin( ), src.end( ));
}


static
void Convert( const ConversionRange& src, SkipVal ) {}

class FieldReader
{
public:
    FieldReader( )
        : _p(nullptr)
    {
    }

    void newRecord( const ConversionRange& range ) {
        _line = range;
        _p = range.begin();
    }
    
    void skipField( ) const {
        try {
            nextCsvField( _conv );
        } catch ( const std::exception& ex ) {
            throw std::runtime_error( string( "Error while reading field " ) + _p + ":" + ex.what( ) );
        }
    }

    template<class T>
    void readField( T& data ) const {
        try {
            nextCsvField( _conv );
            if ( !_conv.empty() ){
                Convert( _conv, data );
            } else {
                data = T( );
            }
            
        } catch (const std::exception& ex ) {
            throw std::runtime_error( string("Error while reading field ") + _p + ":"+ ex.what());
        }
    }

    template<class T>
    void operator()( T& data ) const {
        return readField( data );
    }
private:
    bool nextCsvField( ConversionRange& strOut ) const;
    ConversionRange _line;
    mutable ConversionRange _conv;
    mutable const char* _p;
    
};

// template< typename T, class A  >
// void ParseRecord( FieldReader& fieldParser, T& record, A& ) {
//     boost::fusion::for_each( record, fieldParser );
// }
// 

static const size_t NO_ADDITIONAL;


template< class STORAGE, class A = size_t >
void ReadCsv( const string& path, STORAGE& out, int skip_lines, const A& a = NO_ADDITIONAL ) {
    ULONGLONG t1 = ::GetTickCount64( );
//     BOOST_SCOPE_EXIT_TPL( (t1) ) {
//         ULONGLONG t2 = ::GetTickCount64( );
//         std::cout << "Have read records " << out.size( ) << " in " << ((t2-t1)/1000.0) << " sec from " << path << "\n";
//     }BOOST_SCOPE_EXIT_END();

    typedef typename STORAGE::value_type RecordType;
    
    FileReader lineSource( path);
    ConversionRange line;

    
    for ( int i = 0; i != skip_lines; ++i ) {
        lineSource.next( line);
    }

    int c = 0;
    std::string storage;
    FieldReader fieldParser;
    
    while ( lineSource.next( line) ) {
        fieldParser.newRecord( line );

        out.push_back( RecordType());
        RecordType& record = out.back();
        ParseRecord( fieldParser, record, a );        
    }
    
}

} // namespace csv
