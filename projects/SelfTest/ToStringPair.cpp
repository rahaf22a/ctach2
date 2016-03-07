#include "catch.hpp"

// === Pair ===
TEST_CASE( "std::pair<int,std::string> -> toString", "[toString][pair]" ) {
    std::pair<int,std::string> value( 34, "xyzzy" );
    REQUIRE( Catch::toString( value ) == "{ 34, \"xyzzy\" }" );
}

TEST_CASE( "std::pair<int,const std::string> -> toString", "[toString][pair]" ) {
    std::pair<int,const std::string> value( 34, "xyzzy" );
    REQUIRE( Catch::toString(value) == "{ 34, \"xyzzy\" }" );
}

TEST_CASE( "std::vector<std::pair<std::string,int> > -> toString", "[toString][pair]" ) {
    std::vector<std::pair<std::string,int> > pr;
    pr.push_back( std::make_pair("green", 55 ) );
    REQUIRE( Catch::toString( pr ) == "{ { \"green\", 55 } }" );
}

// This is pretty contrived - I figure if this works, anything will...
TEST_CASE( "pair<pair<int,const char *,pair<std::string,int> > -> toString", "[toString][pair]" ) {
    typedef std::pair<int,const char *> left_t;
    typedef std::pair<std::string,int> right_t;

    left_t  left( 42, "Arthur" );
    right_t right( "Ford", 24 );

    std::pair<left_t,right_t> pair( left, right );
    REQUIRE( Catch::toString( pair ) == "{ { 42, \"Arthur\" }, { \"Ford\", 24 } }" );
}
