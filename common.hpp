#pragma once
#include <exception>
#include <string>
#include <cstdint>
#include <cassert>
//#include <map>
#include <unordered_map>

#include <boost/container/string.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/format.hpp>
#ifdef max
#   undef max
#endif

#ifdef min
#   undef min
#endif

typedef boost::container::string boost_string;
using std::string;
using std::uint32_t;
using std::uint8_t;
typedef boost::iterator_range<const char*> ConversionRange;
using boost::format;

#define  FLYWEIGHTS_PARAMS ,boost::flyweights::no_tracking ,boost::flyweights::no_locking
#define STRING_TYPE(TAG) boost::flyweight<string, boost::flyweights::tag<TAG> FLYWEIGHTS_PARAMS >
//#define STRING_TYPE(TAG) string

typedef uint32_t geoname_id_t;
typedef double   latitude_t;
typedef double   longitude_t;

struct COUNTRY_TAG{};
struct STATE_TAG{};
struct CITY_TAG{};
struct ISP_TAG{};

typedef STRING_TYPE(COUNTRY_TAG)     country_t;
typedef STRING_TYPE(STATE_TAG)       state_t;
typedef STRING_TYPE(CITY_TAG)        city_t;
typedef STRING_TYPE(ISP_TAG)         isp_t;

static const country_t EMPTY_COUNTRY("");

namespace std {
 
template <typename... ARGS>
struct hash<boost::flyweight<ARGS...> > : public unary_function<boost::flyweight<ARGS...>, size_t>
{
    size_t operator()(const boost::flyweight<ARGS...>& v) const {
        return std::hash<boost::flyweight<ARGS...>::value_type>()( v.get() );
    }
};
 
}

template< class BLOCK_T>
country_t Country( const BLOCK_T& block ) {
    return Location( block ).country;
}

class StatsPerCountry {
public:

    typedef std::unordered_map<country_t, uint32_t > Storage;
    typedef std::unordered_map<country_t, uint32_t >::value_type SingleStat;

    
    void add( const country_t& country, uint32_t count );
    uint32_t erase( const country_t& country );
    uint32_t total() const;
    const SingleStat& getEntryWithMaximumStat() const;

private:
    
    Storage _storage;
    uint32_t _total;
};


class ParseError
    : public std::runtime_error
{
public:
    ParseError( const string& m );
};

class NotDecimal
    : public ParseError
{
public:
    NotDecimal( const char* p );
};

class NotBoolean
    : public ParseError
{
public:
    NotBoolean( const char* p );
};

class NotDouble
    : public ParseError
{
public:
    NotDouble( const char* p );
};


struct SkipVal {};


uint32_t readDecimal( const char*& p, bool consumeAll );
uint32_t readDecimal( const ConversionRange& str );

bool readBoolean( const char*& p, bool consumeAll );
bool readBoolean( const ConversionRange& str );

double readDouble( const char*& p, bool consumeAll );
double readDouble( const ConversionRange& str );

#define READ_FIELD( N ) fieldReader.readField( record.N )
#define SKIP_FIELD( N ) fieldReader.skipField( )
typedef std::set<string> StringSet;

inline
StringSet operator-( const StringSet& set1, const StringSet& set2 ) {
    StringSet ret;
    std::set_difference(
        set1.begin( ), set1.end( ),
        set2.begin( ), set2.end( ),
        std::inserter( ret, ret.end( ) ) );

    return std::move( ret );
}

inline
StringSet operator+( const StringSet& set1, const StringSet& set2 ) {
    StringSet ret( set1 );
    ret.insert( set2.cbegin( ), set2.cend( ) );
    return std::move( ret );
}

inline
bool operator&( const StringSet& set, const string& string ) {
    return set.find( string ) != set.cend( );
}

template< class CONT> inline
void StoreSetTo( const string& filename, const CONT& cont ) {
    std::ofstream o( filename );
    for ( const auto& v : cont ) {
        o << v << "\n";
    }
}

