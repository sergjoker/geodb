#pragma once
#include "./common.hpp"

class IPAddress
{
public:
    explicit IPAddress( uint32_t val );
    IPAddress( uint8_t i0, uint8_t i1, uint8_t i2, uint8_t i3 );

    operator uint32_t( ) const;

    uint8_t operator[]( size_t idx ) const;

    IPAddress( const boost_string& str );
    uint32_t asNum() const;

    IPAddress& operator++() {
        ++_val;
        return *this;
    }
private:
    uint32_t _val;
};

bool operator< ( const IPAddress& ip1, const IPAddress& ip2 );


class IPRange
{
public:
    struct BY_IPS_TAG{};
    static const BY_IPS_TAG BY_IPS;

    struct BY_MASKLEN_TAG{};
    static const BY_MASKLEN_TAG BY_MASKLEN;
    

    IPRange();
    explicit IPRange( const boost_string& str );
    IPRange( const BY_MASKLEN_TAG&, uint32_t ip, uint8_t maskLen );
    IPRange( const BY_IPS_TAG&, uint32_t ip1, uint32_t ip2 );


    bool        inRange( uint32_t ip ) const;
    IPAddress   startIp() const;
    IPAddress   endIp( ) const;
    
    size_t count( ) const {
        return _ipEnd - _ipStart + 1;
    }
    
private:
    IPAddress   _ipStart;
    IPAddress   _ipEnd;
};

bool operator==( const IPRange& r1, const IPRange& r2 );

bool operator< ( const IPRange& r1, const IPRange& r2 );

bool intersect( const IPRange& r1, const IPRange& r2  );

static
std::string to_string(const IPAddress& addr) {
    return (format("%||.%||.%||.%||") % int(addr[0]) % int(addr[1]) % int(addr[2]) % int(addr[3])).str();
}

static
std::ostream& operator<<(std::ostream& strm, const IPAddress& addr ) {
    strm << format("%||.%||.%||.%||") % int(addr[0]) % int(addr[1]) % int(addr[2]) % int(addr[3]);
    return strm;
}

static
std::ostream& operator<<( std::ostream& strm, const IPRange& range ) {
    strm << format( "[%|| .. %||]" ) % range.startIp( ) % range.endIp( );
    return strm;
}

class NotIP : public ParseError
{
public:
    NotIP( const char* p )
        : ParseError( string( "Not IP address: " ) + p ) {
    }
};

class NotIPRange : public std::runtime_error
{
public:
    NotIPRange( const char* p )
        : std::runtime_error( std::string( "Not IP range: " ) + p ) {
    }
};



IPAddress readIp( const char*& p, bool consumeAll );
IPAddress readIp( const ConversionRange& str );
IPRange readIpRange( const char*& p, bool consumeAll = true );
IPRange readIpRange( const ConversionRange& str );

void Convert( const ConversionRange& src, IPRange& dst );
void Convert( const ConversionRange& src, IPAddress& dst );
