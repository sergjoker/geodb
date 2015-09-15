#pragma once
#include "./common.hpp"
#include "./IPAddress.hpp"

namespace oldgeo {

struct CityBlock {
    IPRange   range;
    country_t country;
    state_t   state;
    city_t    city;
    double    latitude;
    double    longitude;
    isp_t     isp;
};

static
const CityBlock& Location( const CityBlock& block ) {
    return block;
}


typedef std::vector<CityBlock> CityBlocks;
class Reader
{
public:
    Reader( const string& basePath );
    const country_t& findCountryByIp( const IPAddress& ip ) const;
    CityBlocks _blocks;
};
} // namespace oldgeo 
