#include "stdafx.h"
#include "./IPAddress.hpp"

IPAddress::IPAddress( uint32_t val )
    : _val( val )
{
}

IPAddress::IPAddress( uint8_t i0, uint8_t i1, uint8_t i2, uint8_t i3 )
    : IPAddress( ( ( ( i0 * 256 + i1 ) * 256 + i2 ) * 256 + i3 ) )
{

}

IPAddress::IPAddress( const boost_string& str )
    : IPAddress( readIp( str ) )
{
}

uint8_t IPAddress::operator[]( size_t idx ) const {
    assert( idx < 4 );
    return ( _val >> ( ( 3 - idx ) * 8 ) ) & 0xFF;
}

uint32_t IPAddress::asNum( ) const {
    return _val;
}

IPAddress::operator uint32_t( ) const {
    return asNum( );
}

bool operator<( const IPAddress& ip1, const IPAddress& ip2 ) {
    return ip1.asNum( ) < ip2.asNum( );
}


//////////////////////////////////////////////////////////////////////////
IPRange::IPRange( const boost_string& str )
    : IPRange( readIpRange( str ) )
{
}

/*static */const IPRange::BY_IPS_TAG IPRange::BY_IPS;
/*static*/ const IPRange::BY_MASKLEN_TAG IPRange::BY_MASKLEN;

static
uint32_t MaskFromLen( uint8_t maskLen ) {
    return 0xffffffffu << ( 32 - maskLen );
}

IPRange::IPRange( const BY_MASKLEN_TAG&, uint32_t ip, uint8_t maskLen )    
    : _ipStart( ip )
    , _ipEnd( ip + ~MaskFromLen(maskLen ) )
{
    assert( 0== (ip & ~MaskFromLen(maskLen ) ) );
}

IPRange::IPRange( const BY_IPS_TAG&, uint32_t ip1, uint32_t ip2 )
    : _ipStart(ip1)
    , _ipEnd(ip2)
{
    assert( ip1 <= ip2 );
}


IPRange::IPRange( )
    : _ipStart( 0 )
    , _ipEnd( 0 )
{
}

bool IPRange::inRange( uint32_t ip ) const {
    return ( (ip >= _ipStart) && (ip <= _ipEnd) );
}

IPAddress IPRange::startIp( ) const {
    return _ipStart;
}

IPAddress IPRange::endIp    ( ) const {
    return _ipEnd;
}


bool operator==( const IPRange& r1, const IPRange& r2 ) {
    return r1.startIp( ) == r2.startIp( )
        && r1.endIp( ) == r2.endIp( );
}

bool operator<( const IPRange& r1, const IPRange& r2 ) {
    return r1.endIp( ) < r2.startIp( );
}

bool intersect( const IPRange& r1, const IPRange& r2 ) {
    return !( r1 < r2 ) && !( r2 < r1 );
}

//////////////////////////////////////////////////////////////////////////
IPAddress readIp( const char*& p, bool consumeAll ) {
    assert( nullptr != p );

    const char* orig = p;

    uint32_t fullVal = 0;

    try {
        for ( int groupIdx = 0; groupIdx != 4; ++groupIdx ) {
            uint32_t groupVal = readDecimal( p, false );
            if ( groupVal > 255 ) {
                throw NotIP( orig );
            }
            fullVal *= 256;
            fullVal += groupVal;

            if ( groupIdx != 3 ) {
                if ( '.' != *p ) {
                    throw NotIP( orig );
                }
                ++p; // Skip '.'
            }

        }
    } catch ( const ParseError& ) {
        throw NotIP( orig );
    }

    if ( consumeAll && '\0' != *p ) {
        throw NotIP( orig );
    }
    return IPAddress( fullVal );
}

IPAddress readIp( const ConversionRange& str ) {
    const char* p = str.begin( );
    return readIp( p, true );
}

IPRange readIpRange( const ConversionRange& str ) {
    const char* p = str.begin( );
    return readIpRange( p, true );
}

IPRange readIpRange( const char*& p, bool consumeAll /*= true */ ) {
    assert( nullptr != p );
    const char* orig = p;

    try {
        IPAddress ip = readIp( p, false );
        if ( '/' != *p ) {
            throw NotIPRange( orig );
        }
        ++p;
        uint32_t len = readDecimal( p, consumeAll );
        if ( len > 32 ) {
            throw NotIPRange( orig );
        }
        return IPRange(IPRange::BY_MASKLEN, ip, len );

    } catch ( const ParseError& ) {
        throw NotIPRange( orig );
    }
}

void Convert( const ConversionRange& src, IPRange& dst ) {
    dst = readIpRange( src );
}

void Convert( const ConversionRange& src, IPAddress& dst ) {
    dst = readIp( src );
}
