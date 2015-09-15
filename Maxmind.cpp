#include "stdafx.h"
#include <algorithm>
#include <list>

#include "./Maxmind.hpp"
#include "./csvReader.hpp"


namespace maxmind {
// http://dev.maxmind.com/geoip/geoip2/geoip2-csv-databases/#CSV_File_Format

static const char* const CITY_BLOCKS       = "GeoLite2-City-Blocks-IPv4.csv";
static const char* const CITY_LOCATIONS    = "GeoLite2-City-Locations-en.csv";
static const char* const COUNTRY_BLOCKS    = "GeoLite2-Country-Blocks-IPv4.csv";
static const char* const COUNTRY_LOCATIONS = "GeoLite2-Country-Locations-en.csv";
static const char* const ASN_NUMBERS       = "GeoIPASNum2.csv";

typedef std::list<CountryLocation> CountryLocations;
typedef std::list<CityBlock> CountryBlocks;

template< class CONT, class KEY >
typename CONT::const_iterator binary_search( const CONT& cont, KEY CONT::value_type::*fld, KEY key ) {
    const auto& keyIsLess = [ key, fld ]( const CONT::value_type& location ) -> bool {
        return ( location.*fld ) < key;
    };
    const auto& it = std::partition_point( cont.cbegin( ), cont.cend( ), keyIsLess );
    if ( ( *it ).*fld == key ) {
        return it;
    } else {
        return cont.cend( );
    }
}


bool CompareBlockByNetwork( const CityBlock& b1, const CityBlock& b2 ) {
    return b1.range < b2.range;
}

bool CompareLocationByGeoNameId( const CityLocation& l1, const CityLocation& l2 )  {
    return l1.geoname_id < l2.geoname_id;
}

void ParseRecord( csv::FieldReader& fieldReader, CountryLocation& record, size_t ) {
    READ_FIELD( geoname_id );
    SKIP_FIELD( locale_code );
    SKIP_FIELD( continent_code );
    SKIP_FIELD( continent_name );
    SKIP_FIELD( country_iso_code );
    READ_FIELD( country );
};

void ParseRecord( csv::FieldReader& fieldReader, CityLocation& record, size_t ) {
    READ_FIELD( geoname_id );
    SKIP_FIELD( locale_code );
    SKIP_FIELD( continent_code );
    SKIP_FIELD( continent_name );
    SKIP_FIELD( country_iso_code );
    READ_FIELD( country );
    SKIP_FIELD( subdivision_1_iso_code );
    READ_FIELD( state_name );
    SKIP_FIELD( subdivision_2_iso_code );
    SKIP_FIELD( subdivision_2_name );
    READ_FIELD( city_name );
    SKIP_FIELD( metro_code );
    SKIP_FIELD( time_zone );
};

template< class LOCATION_T >
const LOCATION_T& findLocation ( const std::vector<LOCATION_T>& locations, geoname_id_t id ) {
    const auto& it = binary_search( locations, &LOCATION_T::geoname_id, id );
    assert( it != locations.end( ) );

    return *it;
}

void ParseBlockRecord( csv::FieldReader& fieldReader, CityBlock& record, const CityLocations& locations ) {
    geoname_id_t geoname_id;
    geoname_id_t registered_country_geoname_id;
    READ_FIELD( range );
    fieldReader.readField( geoname_id );
    fieldReader.readField( registered_country_geoname_id );
    SKIP_FIELD( represented_country_geoname_id );
    SKIP_FIELD( is_anonymous_proxy );
    SKIP_FIELD( is_satellite_provider );
    SKIP_FIELD( postal_code );
    READ_FIELD( latitude );
    READ_FIELD( longitude );

    if ( 0 == geoname_id ){
        geoname_id = registered_country_geoname_id;
    }
    record.geoname_id = geoname_id;
    record.location = &findLocation( locations, record.geoname_id );
}

void ParseRecord( csv::FieldReader& fieldReader, CityBlock& record, const CityLocations& locations ) {
    ParseBlockRecord( fieldReader, record, locations  );
}

void ReadCityBlocks( const string& path, CityBlocks& blocks, CountryBlocks& countryBlocks, CityLocations& locations ) {
    std::list<CityBlock> tmp;
    csv::ReadCsv( path, tmp, 1, locations );

    // Merge
    blocks.assign( tmp.begin( ), tmp.end( ) );
    for ( CityBlock& block: blocks ) {
        const_cast<CityLocation*>(block.location)->blocks.push_back(&block);
    }
    bool sorted = std::is_sorted( blocks.cbegin( ), blocks.cend( ), CompareBlockByNetwork );
    assert( sorted );
}

void ReadCountryLocations( const string& path, CountryLocations& locations ) {
    csv::ReadCsv( path, locations, 1 );
    bool sorted = std::is_sorted( locations.cbegin( ), locations.cend( ), CompareLocationByGeoNameId );
    assert( sorted );
}

void ReadCityLocations( const string& path, CityLocations& locations, CountryLocations& countryLocations ) {
    //abs
    std::list<CityLocation> tmp;
    csv::ReadCsv( path, tmp, 1 );
    for( const CityLocation& cl: tmp ) {
        if (countryLocations.empty()) {
            locations.push_back( cl );
            continue;
        }
        while ( !countryLocations.empty() && countryLocations.front().geoname_id <= cl.geoname_id ) {
            if ( countryLocations.front().geoname_id < cl.geoname_id) {
                locations.push_back( countryLocations.front() );
            }            
            countryLocations.pop_front( );
        }

        locations.push_back( cl );
    }
    
    bool sorted = std::is_sorted( locations.cbegin( ), locations.cend( ), CompareLocationByGeoNameId );
    assert( sorted );
}

void ReadCountryBlocks( const string& path, CountryBlocks& blocks, CityLocations& locations ) {
    csv::ReadCsv( path, blocks, 1, locations );
    bool sorted = std::is_sorted( blocks.cbegin( ), blocks.cend( ), CompareBlockByNetwork );
    assert( sorted );
}
void ParseRecord( csv::FieldReader& fieldParser, AsnNum& record, size_t ) {
    uint32_t start_ip = 0;
    uint32_t stop_ip = 0;
    fieldParser( start_ip );
    fieldParser( stop_ip );
    record.range = IPRange( IPRange::BY_IPS, start_ip, stop_ip );
    fieldParser( record.asn_name );
}
static
bool CompareAsnByRange( const AsnNum& r1, const AsnNum& r2 ) {
    return r1.range < r2.range;
};

void ReadAsnNums( const string& path, AsnNums& asnNums ) {
    csv::ReadCsv( path, asnNums, 0 );

    bool sorted = std::is_sorted( asnNums.cbegin( ), asnNums.cend( ), CompareAsnByRange );
    assert( sorted );
}



Reader::Reader( const string& basePath ) {
    {
        CountryLocations countryLocations; // We'll merge all the data into cityLocations, so no need after this block
        CountryLocation defaultCountryLocation;
        defaultCountryLocation.geoname_id = 0;
        defaultCountryLocation.country = "";
        countryLocations.push_back( defaultCountryLocation );
    
        ReadCountryLocations( basePath + COUNTRY_LOCATIONS, countryLocations );   
        ReadCityLocations( basePath + CITY_LOCATIONS, _locations, countryLocations );
    }


    {
        CountryBlocks countryBlocks;
        ReadCountryBlocks( basePath + COUNTRY_BLOCKS, countryBlocks, _locations );
        ReadCityBlocks( basePath + CITY_BLOCKS, _blocks, countryBlocks, _locations );
    }
        
    
//     for ( const CountryLocation& location : countryLocations ) {
//         _countryToId[location.country_name] = location.geoname_id;
//         _idToCountry[location.geoname_id] = location.country_name;
//     }
//     for ( const CityLocation& location : cityLocations ) {
//         //assert( _countryToId.cend() != _countryToId.find(location.country_name) );
//     }

    ReadAsnNums( basePath + ASN_NUMBERS, _asnNums );
}

const CityBlock* Reader::findBlock(const IPAddress& ip) const {
    const auto& rangeIsLess = [&ip](const CityBlock& block) -> bool {
        return block.range.endIp() < ip;
    };

    const auto& it = std::partition_point( _blocks.cbegin(), _blocks.cend(), rangeIsLess );

    if (_blocks.cend() == it) {
        return nullptr;
    }

    if (ip < it->range.startIp()) {
        return nullptr;
    }
    return &*it;
}

const CityLocation * Reader::findLocation(const IPAddress & ip) const {
    const CityBlock* block = findBlock(ip);
    if ( nullptr == block ) {
        return nullptr;
    }

    return block->location;
}

// const country_t& Reader::getCountryById( geoname_id_t id ) const {
//     const auto& it = _idToCountry.find( id );
//     if ( _idToCountry.cend() != it ) {
//         return it->second;
//     } else {
//         return EMPTY_COUNTRY;
//     }
// }

// country_t Reader::findCountryByRange( const IPRange& range ) const {
//     const auto& rangeIsLess = [ &range ]( const CountryBlock& block ) -> bool {
//         return block.range < range;
//     };
// 
//     const auto& it = std::partition_point( countryBlocks.cbegin( ), countryBlocks.cend( ), rangeIsLess );
// 
//     if ( countryBlocks.cend( ) == it ) {
//         return EMPTY_COUNTRY;
//     }
// 
//     if ( intersect( range, it->range ) ) {
//         return getCountryById( it->geoname_id );
//     }
// 
//     return EMPTY_COUNTRY;
// }


geoname_id_t Reader::getGeoId( const country_t& countryName ) const {
    return _countryToId.at(countryName);
}

} //namespace maxmind
