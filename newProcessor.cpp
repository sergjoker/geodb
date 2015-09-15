// newProcessor.cpp : Defines the entry point for the console application.
#include "stdafx.h"
#include <Windows.h>
#include <map>
#include <iomanip>

#include <boost/range/adaptor/map.hpp>
#include <boost/format.hpp>
#include <boost/scope_exit.hpp>
#include <boost/container/flat_set.hpp>


#include "./Maxmind.hpp"
#include "./oldGeo.hpp"

#define TEST_DIR R"(d:\work\geo\test\)"

#define MAXMIND_DIR     TEST_DIR  R"(maxmind\)"
//#define MAXMIND_DIR     TEST_DIR  R"(maxmind_full\)"
#define OUT_DIR         TEST_DIR  R"(out\)"
#define OLD_GEO_DIR     TEST_DIR  R"(old-geodata\)"

using boost::format;
using namespace std;


template<class CONT, class T >
StringSet GetAllValues( const CONT& c,T typename CONT::value_type::* pmem ) {
    StringSet ret;
    for( const auto & val: c) {
        ret.insert( (val.*pmem).get( ) );
    }

    return std::move( ret );
}

template<class CONT>
StringSet GetAllCountries( const CONT& c) {
    StringSet ret;
    for ( const auto & val : c ) {
        ret.insert( Country( val ).get());
    }
    return std::move( ret );
}
/*
class RangesSet {
public:
    void add( const IPRange& range ) {
        if ( !_ranges.empty() ) {
            IPRange& last = _ranges.back( );
            
            assert( last < range ); // Should always grow
            
            if ( (last.endIp()+ 1) == range.startIp() ) {
                last = IPRange( IPRange::BY_IPS, last.startIp(), range.endIp() );
            } else {
                _ranges.push_back( range );
            }
        } else {
            _ranges.push_back( range );
        }
    }

    bool contains( const IPAddress& ip ) const {
        const auto& it = std::partition_point( _ranges.cbegin( ), _ranges.cend( ), 
                                                [&ip]( const IPRange& r ) -> bool { 
                                                    return ip < r.startIp( );
                                                }
                                              );

        if (it == _ranges.cend()) {
            return false;
        }
        const IPRange& r = *it;
        if ( ip > r.endIp() ) {
            return false;
        }
        return true;
    }
private: // types
    typedef std::vector<IPRange> RangesList;
private: // Data
    RangesList _ranges;
};
static const RangesSet EMPTY_RANGESET;




template< typename KEY>
class RangesByKey {
public:
    void add( const KEY& key, const IPRange& range ) {
        RangesSet& ranges = _rangesByKey[key];
        ranges.add( range );
    }

    const RangesSet& ranges( const KEY& key ) const {
        const auto& it = _rangesByKey.find( key );
        if ( _rangesByKey.end() != it ) {
            *it;
        }
        return EMPTY_RANGESET;
    }

    typedef std::map<KEY, RangesSet> Storage;
    Storage _rangesByKey;
};
*/
 
typedef std::map<country_t, StatsPerCountry> CountersStoragePerCountry;

template< typename BLOCKS >
void GetStatsForRange( const IPRange& range, const BLOCKS& blocks, StatsPerCountry& counters ) {
    typedef typename BLOCKS::value_type CityBlock;
    IPAddress startIp = range.startIp( );
    IPAddress endIp = range.endIp( );

    const auto& rangeIsLess = [ startIp ]( const CityBlock& block ) -> bool {
        return block.range.endIp( ) < startIp;
    };

    auto rangesIt = std::partition_point( blocks.cbegin( ), blocks.cend( ), rangeIsLess );
    if ( blocks.cend( ) == rangesIt ) { // All existing ranges are before IP
        uint32_t rest = endIp - startIp + 1;
        counters.add( EMPTY_COUNTRY, rest );
        return;
    }

    typename BLOCKS::const_iterator rangesEnd = blocks.cend( );

    for ( ; rangesIt != rangesEnd; ++rangesIt ) {
        const IPRange& geoRange = rangesIt->range;

        if ( geoRange.startIp( ) > endIp ) {
            uint32_t rest = endIp - startIp + 1;
            counters.add( EMPTY_COUNTRY, rest );
            return;
        }

        uint32_t start = std::max( geoRange.startIp( ), startIp );
        uint32_t end = std::min( geoRange.endIp( ), endIp );
        uint32_t count = end - start + 1;
        uint32_t rest = start - startIp;
        counters.add( EMPTY_COUNTRY, rest );
        startIp = IPAddress( end + 1 );
        counters.add( Location( *rangesIt ).country, count );
    }
}

template< class SOURCE_MAP, class REFERENCE_MAP > static
CountersStoragePerCountry CalculateStats(
    const SOURCE_MAP& source,
    const REFERENCE_MAP& reference,
    size_t& totalIps )
{
    totalIps = 0;
    CountersStoragePerCountry stats;
    for ( const auto& block : source._blocks ) {
        const country_t& newCountry = Location(block).country;
        StatsPerCountry& counters = stats[newCountry];
        const IPRange& ipRange = block.range;
        GetStatsForRange(ipRange, reference._blocks, counters );
        totalIps += ipRange.count();
    }
    return std::move(stats);
}

//using RangesByCountry = RangesByKey < country_t > ;
void InitGlobal( ) {
    std::ios::sync_with_stdio( false );
    //     std::locale::global( std::locale::empty() );
    //     std::cout.imbue( std::locale::empty() );
    //     std::locale::global( std::locale::classic() );
    //     std::cout.imbue( std::locale::classic() );
    std::locale::global( std::locale( "" ) );
    std::cout.imbue( std::locale( "" ) );
    //     std::locale::global( std::locale( "" ) );
    //     std::cout.imbue( std::locale( "" ) );
}


class TimeMeasure {
public:
    TimeMeasure( const string& note ) 
        : _note(note)
        , _start(::GetTickCount64( ))
        , _stopped(false)
    {
    }

    void stop() {
        ULONGLONG stop = ::GetTickCount64( );
        std::cout << format( "%|| %|0.4| sec\n" ) % _note % ( ( stop - _start ) / 1000.0 );
        _stopped = true;
    }

    ~TimeMeasure() {
        if (!_stopped) {
            stop( );
        }
    }

    const string _note;
    ULONGLONG _start;
    bool _stopped;
};

static const string FILE_HEADER = "GLO_ID,GLO_COUNTRY,GLO_STATE,GLO_CITY,GLO_LATITUDE,GLO_LONGITUDE\n"; 

void Dump(const maxmind::CityBlocks& blocks, const string& filename) {
    std::set<geoname_id_t> processed;

    std::ofstream o( filename );
    o << FILE_HEADER;
    for ( const auto& block: blocks ) {
        if ( processed.find(block.geoname_id) != processed.cend() ) {
            continue;;
        }
        processed.insert( block.geoname_id );
        o << block.geoname_id << ','
            << block.location->country << ','
            << block.location->state_name << ','
            << block.location->city_name << ','
            << block.latitude << ','
            << block.longitude << '\n';
    }
}

std::map<std::string, size_t> counters;

static
size_t GetAndIncrementCount( const city_t& city ) {
    size_t& counter = counters[city];
    return counter++;
}

std::string INDENTS[] = {
    "",
    "    ",
    "        ",
    "            ",
    "                ",
    "                    ",
    "                        ",
    "                            ",
    "                                ",
    "                                    "
};


static
const std::string& Indent(int level) {
    return INDENTS[level];
}

class Node {
public:
    Node(std::ostream& strm, int level, const std::string& name, const std::string& args = std::string(""), const std::string& val = std::string(""))
        : _strm(strm)
        , _level(level)
        , _name(name)        
        , _args(args)
        , _val(val)
        , _indent(Indent(_level))
    {
        _strm << _indent << '<' << name;
        if (!_args.empty()) {
            _strm << ' ' << _args;
        }

        if (!_val.empty()) {
            _strm << '>' << _val << "</" << _name;
        }

        _strm << ">\n";
    }

    ~Node() {
        if ( _val.empty() ) {
            _strm << _indent << "</" << _name << ">\n";
        }
    }

    std::ostream& _strm;

    std::string _name;
    int _level;
    std::string _args;
    std::string _val;
    
    const std::string& _indent;


};

typedef std::set<const maxmind::CityBlock*> BlocksSet;
typedef std::map<country_t, std::map< state_t, std::map<city_t, BlocksSet> > > hierarchy_t;
hierarchy_t hierarchy;

template< class BLOCKS_T> static
void ShowRanges(std::ostream& strm, int level, const BLOCKS_T& blocks) {
    if ( blocks.empty() ) {
        return;
    }
    Node ip_range(strm, level, "ip_ranges");
    for (const maxmind::CityBlock* block : blocks) {
        Node ip_range(strm, level + 1, "ip_range");
        Node start_ip(strm, level + 2, "start_ip", "", to_string(block->range.startIp()));
        Node end_ip(strm, level + 2, "end_ip", "", to_string(block->range.endIp()));
        Node isp(strm, level + 2, "isp", "", "null");
    }
}

static
void ShowLocation(std::ostream& strm, int level, const std::string& name, const maxmind::CityLocation& location) {
    if (location.blocks.empty()) {
        return;
    }

    Node city(strm, level, "location", std::string(R"(location_name = ")") + name + R"(" location_type = "city")");
    Node latitude(strm, level + 1, "latitude", "", std::to_string(location.blocks.front()->latitude));
    Node longitude(strm, level + 1, "longitude", "", std::to_string(location.blocks.front()->longitude));
    ShowRanges(strm, level, location.blocks);
}

typedef boost::container::flat_set<IPAddress> InSet;

static
InSet ReadCustomerIps( const std::string& path ) {
    std::ifstream s(path);
    std::set<IPAddress> tmp;
    int v  = 0;
    while ( s>>v)  {
        tmp.insert(IPAddress(static_cast<uint32_t>(v)));
    }
    return InSet(tmp.cbegin(), tmp.cend());   

}

BlocksSet NULL_BLOCKS;
const BlocksSet& GetBlocks(const std::string& country, const std::string& state, const std::string& city) {
    const auto countryData = hierarchy.find(country_t(country));
    if ( countryData == hierarchy.cend() ) {
        return NULL_BLOCKS;
    }

    const auto& states = countryData->second;
    auto nullState = states.find(state_t(state));

    if (states.cend() == nullState) {
        return NULL_BLOCKS;
    }

    const auto& cities = nullState->second;
    auto nullCity = cities.find( city_t(city) );
    if ( cities.cend() == nullCity ) {
        return NULL_BLOCKS;
    }

    const auto& blocks = nullCity->second;
    return blocks;
}



int main() {
    try 
    {   
#if 1
        ofstream outStrm(R"(c:\india.xml)");
#else
        ostream& outStrm = cout;
#endif
        const InSet& inSet = ReadCustomerIps(R"(d:\work\geo\test\qq\Locations_MySQL_DB.csv)");
        //return 0;
        //TimeMeasure measureReadTime( "Total read time" );            
        maxmind::Reader mm(MAXMIND_DIR);

        std::set<const maxmind::CityBlock*> coveringBlocks;
        std::list<IPAddress> notCoveredIPs;

        for ( const IPAddress& ip : inSet ) {
            const maxmind::CityBlock* block = mm.findBlock(ip);
            if (nullptr == block) {
                notCoveredIPs.push_back(ip);
                //outStrm << " Not found : " << ip << endl;
            } else {
                coveringBlocks.insert(block);
                const maxmind::CityLocation& location = *(block->location);
                hierarchy[location.country][location.state_name][location.city_name].insert(block);
                //outStrm << location.country << ":" << location.state_name << ":" << location.city_name << ":" << ip <<endl;
            }
        }
        
        for ( const auto& countryData : hierarchy ) {
            if ( "India" != countryData.first) {
                continue;
            }
            country_t country = countryData.first;
            Node country_node(outStrm, 0, "location", R"(location_name = ")" + country.get() + R"(" location_type = "country")");
            const BlocksSet& freeBlocks = GetBlocks( country.get(), "", "" );
            longitude_t longitude = 0.0;
            longitude_t latitude = 0.0;
            if (!freeBlocks.empty()) {
                const maxmind::CityBlock* bl = *freeBlocks.begin();
                latitude = bl->latitude;
                longitude = bl->longitude;
            }

            Node latitude_node(outStrm, 1, "latitude", "", to_string(latitude) );
            Node longitude_node(outStrm, 1, "longitude", "", to_string(longitude));

            ShowRanges(outStrm, 1, freeBlocks);
            Node locations_list(outStrm, 1, "locations_list");

            for (const auto& stateData : countryData.second ) {
                state_t state = stateData.first;
                if ( state.get().empty()) {
                    continue;
                }
                    
                const BlocksSet& freeBlocks = GetBlocks(country.get(), state.get(), "");
                if (!freeBlocks.empty()) {                
                    Node state_node(outStrm, 2, "location", R"(location_name = ")" + state.get() + R"( state" location_type = "city")");

                    longitude_t longitude = 0.0;
                    longitude_t latitude = 0.0;
                    if (!freeBlocks.empty()) {
                        const maxmind::CityBlock* bl = *freeBlocks.begin();
                        latitude = bl->latitude;
                        longitude = bl->longitude;
                    }
                    Node latitude_node(outStrm, 3, "latitude", "", to_string(latitude));
                    Node longitude_node(outStrm, 3, "longitude", "", to_string(longitude));

                    ShowRanges(outStrm, 3, freeBlocks);                    
                }
                //Node locations_list(outStrm, 3, "locations_list");
                for (const auto& cityData : stateData.second ) {
                    city_t city = cityData.first;
                    if (city.get().empty()) {
                        continue;
                    }
                    size_t idx = GetAndIncrementCount(city);
                    const string& name = city.get() + ((0 == idx) ? "" : to_string(idx));
                    Node city_node(outStrm, 2, "location", R"(location_name = ")" + name + R"(" location_type = "city")");
                    
                    const BlocksSet& blocks = cityData.second;
                    longitude_t longitude = 0.0;
                    longitude_t latitude = 0.0;
                    if (!blocks.empty()) {
                        const maxmind::CityBlock* bl = *blocks.cbegin();
                        latitude = bl->latitude;
                        longitude = bl->longitude;
                    }
                    Node latitude_node(outStrm, 3, "latitude", "", to_string(latitude));
                    Node longitude_node(outStrm, 3, "longitude", "", to_string(longitude));
                    ShowRanges(outStrm, 3, blocks);
                }
            }
        }

        
        
        return 0;
#if 0
        int bas_l = 0;
        Node india(strm, bas_l,"location",R"(location_name = "India" location_type = "country")" );
        Node latitude(strm, bas_l + 1, "latitude", "", "28.6");
        Node longitude(strm, bas_l+ 1, "longitude", "", "77.2");
        // Show ranges with no state
        {
            const maxmind::CityLocation* loc = hierarchy[country_t("India")][state_t("")][city_t("")];
            ShowRanges(strm, bas_l, *loc);
        }

        // Show sub_locations
        {
            Node locations_list(strm, bas_l + 1, "locations_list");
            for (const auto& state_data: hierarchy[country_t("India")]) {
                if ( "" == state_data.first.get() ) {
                    continue;
                }

                for (const auto& city_data : state_data.second) {
                    const std::string& name = ("" == city_data.first)
                        ? state_data.first.get() + " state"
                        : city_data.first.get();

                    ShowLocation(strm, bas_l + 2, name, *city_data.second);

                }
            }
        }
#endif
#if 0
        oldgeo::Reader old(OLD_GEO_DIR );
        measureReadTime.stop( );

        Dump( mm._blocks, OUT_DIR "geo3.csv" );

        
        const StringSet& old_countries = GetAllCountries( old._blocks );
        const StringSet& new_countries = GetAllCountries( mm._blocks );

        const StringSet& added_countries = new_countries - old_countries;
        const StringSet& removed_countries = old_countries - new_countries;
      
        
        size_t ips_new = 0;        
        size_t ips_old = 0;
        
        TimeMeasure measureStatsTime( "Total stat processing time" );
        CountersStoragePerCountry newToOldStats = CalculateStats( mm, old, ips_new );
        CountersStoragePerCountry oldToNewStats = CalculateStats( old, mm , ips_old );        
        measureStatsTime.stop( );
        MessageBeep( MB_OK );

        std::cout << format( "Total %d IP addresses\n") % ips_new;
        for ( auto& country: newToOldStats ) {
            const country_t& newName = country.first;
            if ( EMPTY_COUNTRY == newName ) {
                continue;
            }

            bool existedBefore = old_countries & newName;
            if ( existedBefore ) {
                //std::cout << format( "\"%||\" (No rename)\n" ) % newName;
                continue;
            }

            StatsPerCountry& counters = country.second;
            counters.erase( EMPTY_COUNTRY );
            for( auto& country: new_countries ) {
                counters.erase( country_t(country) );
            }

#define REP "\"%||\"(%||)%|60t|--> "

            size_t total = counters.total();
            if ( 0 == counters.total()) {
                std::cout << format( REP "No Match!!!!!\n" ) % newName % total;
                continue;
            }

            const StatsPerCountry::SingleStat& best = counters.getEntryWithMaximumStat( );
            
            const country_t& oldName = best.first;
            if ( newName == oldName ) {
                continue;
            }
            
#if 0
            std::cout << "------------------------------------------\n";
            for ( auto& x : counters ) {
                std::cout << format( "\"%||\"%|60t|%|10.4f|%%%|14|\n" ) % x.first % (x.second *100. / total) % x.second ;                
            }
#endif

            std::cout << format( REP ) % newName % total;            
            std::cout << format("\"%||\" %|60t|") % oldName;                
            size_t maxCount = best.second;
            if ( maxCount == total ) {
                std::cout << "All included";
            } else {
                std::cout << format("%|10.4f|%%")  % (maxCount*100. / total);
            }

            std::cout << '\n';
        }

#if 0
        //for( const string& countryStr : StringSet{"USA"} ) {
        
        for( const string& countryStr : added_countries ) {
            country_t country(countryStr);
            
            CountersStorage counters;
            uint32_t ips = 0;
            
            uint32_t country_id = mm2.getGeoId( country );

            for( const auto& v : mm2.countryBlocks ) {
                if (v.geoname_id != country_id ) {
                    continue;
                }
                const IPRange& range = v.network;
                for ( uint32_t ip = range.startIp( ); ip != range.endIp( ); ++ip,++ips ) {
                    const country_t& newCountry = old_geo.findCountryByIp( IPAddress(ip) );

                    if ( !newCountry.get( ).empty( ) ) {
                        ++( counters[newCountry] );
                    }
                }
                //const country_t& newCountry = mm2.findCountryByRange( v.range );

            }

            const auto& it = std::max_element(counters.cbegin(),counters.cend(), 
                                               [](const CountersStorage::value_type& v1, const CountersStorage::value_type& v2) -> bool 
            {
                return v1.second < v2.second;
            }
            );


//             for ( auto& x : counters )
//             {
//                 std::cout << x.first << ':' << x.second << '\n';
//             }
            std::cout << country << " --> ";
            if( counters.cend() != it ) {
                std::cout << it->first;
                if ( it->second == ips ) {
                    std::cout << "              !~~~~~~~~~~~~~";
                }
            } else {
                std::cout << "No Match!!!!!";
            }
            std::cout << '\n';
        }
#endif

#if 0

        //for( const string& countryStr : StringSet{"USA"} ) {
        for( const string& countryStr : removed_countries ) {
            country_t country(countryStr);

            typedef std::map<country_t, size_t> CountersStorage;
            uint32_t ips = 0;

            for( const auto& v : old_geo._ranges ) {
                if (v.country != country ) {                
                    continue;
                }
                for ( uint32_t ip = v.range.startIp( ); ip != v.range.endIp( ); ++ip,++ips ) {
                    const country_t& newCountry = mm2.findCountryByIp( IPAddress(ip) );

                    if ( !newCountry.get( ).empty( ) ) {
                        ++( counters[newCountry] );
                    }
                }
                //const country_t& newCountry = mm2.findCountryByRange( v.range );
                
            }

            const auto& it = std::max_element(counters.cbegin(),counters.cend(), 
                                               [](const CountersStorage::value_type& v1, const CountersStorage::value_type& v2) -> bool 
                                                {
                                                    return v1.second < v2.second;
                                                }
            );
        
            
//             for ( auto& x : counters )
//             {
//                 std::cout << x.first << ':' << x.second << '\n';
//             }

            std::cout << country << " --> " << it->first;
            if ( it->second == ips ) {
                std::cout << "              !~~~~~~~~~~~~~";
            }
            std::cout << '\n';
        }

#endif
        StoreSetTo( OUT_DIR "added_countries.txt", added_countries );
        StoreSetTo( OUT_DIR "removed_countries.txt", removed_countries );

        /*for (const auto& c : removed_countries ) {
            for( const auto& e: oldGeoRanges ) {
                for ( const auto& new_country: )
                
            }
        }*/
        
// 
//         for ( const auto& d : removed_countries ) {
//             std::cout << d << "\n";
//         }
        
        //MessageBoxA( 0, "a", "Ad", MB_OK );
        //int g = 0;
#endif

    } catch ( const std::exception& ex ) {
        std::cout << ex.what() << '\n';
        return 1;
    }
    return 0;
}   



#if 0
I. New name for old location
If A not exists in IP2LOC:
    if 90% of ips from location A in MM are resolved to B in IP2L 
        and opposite:
     
        then it's rename B->A ( we have to map A->B while creating DB)
if 90% of ips from location A in MM are resolved to B in IP2L   but 



A   5 
B   6

A1  5.5

Maxmind
lat,lon, name 

A

A'


Rename A->B   => Mapping

Split  A-> B | C    

Split  A-> A | B

Merge  A|B -> C

Merge  A|B -> A

ip from A -> B


/// Ask platform how they use coordinates in LM

#endif

