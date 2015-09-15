#pragma once
#include <unordered_map>
#include <list>
#include <deque>

#include <boost/fusion/adapted/struct/define_struct_inline.hpp>

#include "./common.hpp"
#include "./IPAddress.hpp"

namespace maxmind {
using std::string;

struct CityBlock;
typedef std::deque<CityBlock*> BlocksList;

struct CityLocation {
    geoname_id_t geoname_id;
    country_t    country;
    state_t      state_name;
    city_t       city_name;    

    BlocksList   blocks;
};

struct CountryLocation : public CityLocation { // We'll just replace ParseRecord
};


struct CityBlock
{
    IPRange             range;
    const CityLocation* location;
    geoname_id_t        geoname_id;
    latitude_t          latitude;
    longitude_t         longitude;
};

struct AsnNum {
    IPRange range;
    isp_t   asn_name;
};

typedef std::vector<AsnNum> AsnNums;
void ReadAsnNums( const string& path, AsnNums& asnNums );

static
const CityLocation& Location( const CityBlock& block ) {
    return *block.location;
}


typedef std::vector<CityLocation> CityLocations;
typedef std::vector<CityBlock> CityBlocks;

class Reader
{
public:
    CityLocations _locations;
    CityBlocks    _blocks;
    AsnNums       _asnNums;

    Reader( const string& basePath );
    
    const CityBlock* findBlock(const IPAddress& ip) const;
    const CityLocation* findLocation(const IPAddress& ip) const;

    //const country_t& getCountryById( geoname_id_t id ) const;
    geoname_id_t getGeoId( const country_t& countryName ) const;
private:
    //country_t findCountryByRange( const IPRange& range ) const;
    typedef std::unordered_map< country_t, uint32_t > CountryToId;
    typedef std::unordered_map< uint32_t, country_t> IdToCountry;

    CountryToId _countryToId;
    IdToCountry _idToCountry;
};

} //namespace maxmind

