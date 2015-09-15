#include "stdafx.h"
#include <functional>
#include <algorithm>
#include <utility>
#include "./oldGeo.hpp"
#include "./csvReader.hpp"
#ifdef max
#   undef max
#endif

#ifdef min
#   undef min
#endif
namespace oldgeo {

static const isp_t EMPTY_ISP("");
static const isp_t MINUS_ISP("-");

static const string     RANGES      = "ip_ranges.v4.csv";

void ParseRecord( csv::FieldReader& fieldParser, CityBlock& record, size_t ) {
    SkipVal skip;
    fieldParser( skip ); // GLIR_ID
    fieldParser( skip ); // GLO_ID
    fieldParser( skip ); // PART_ID
    uint32_t start_ip = 0;
    uint32_t stop_ip = 0;
    fieldParser( start_ip );
    fieldParser( stop_ip );
    record.range = IPRange( IPRange::BY_IPS, start_ip, stop_ip );
    fieldParser( record.country );
    fieldParser( record.state );
    fieldParser( record.city );
    fieldParser( record.latitude );
    fieldParser( record.longitude );
    fieldParser( record.isp );
    if ( MINUS_ISP == record.isp ) {
        record.isp = EMPTY_ISP;
    }
}

static
bool CompareByRange( const CityBlock& r1, const CityBlock& r2 ) {
    return r1.range < r2.range;
};

void ReadGeoRanges( const string& path, CityBlocks& geoRanges ) {
    geoRanges.reserve( 8727830 );
    csv::ReadCsv( path, geoRanges, 1 );
    
    assert( std::is_sorted( geoRanges.cbegin( ), geoRanges.cend( ), CompareByRange ));
}


Reader::Reader( const string& basePath ) {
    ReadGeoRanges( basePath + oldgeo::RANGES, _blocks );
}

const country_t& Reader::findCountryByIp( const IPAddress& ip ) const {
    const auto& rangeIsLess = [ ip ]( const CityBlock& range ) -> bool {
        return range.range.endIp( ) < ip;
    };
    const auto& it = std::partition_point( _blocks.cbegin( ), _blocks.cend( ), rangeIsLess);

    if ( _blocks.cend( ) == it ) {
        return EMPTY_COUNTRY;
    }

    if ( ip >= it->range.startIp( ) ) {
        return it->country;
    }

    return EMPTY_COUNTRY;
}


} // namespace oldgeo 
