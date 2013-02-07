/*
 * packet_handling_test.cpp
 *
 *  Created on: 28 Sep 2012
 *      Author: lukasz.forynski
 */

#include "test_generic.h"
#include <string.h>

#include <packet_handling.h>

enum
{
    buff_size = 128
};
uint8_t buffer[buff_size];
Packet global(buffer, buff_size);


TEST_CASE("add fields to global packet", "should not throw")
{
    REQUIRE_NOTHROW( global.add_field<uint8_t>("first"));
    REQUIRE_NOTHROW( global.add_field<uint16_t>("second"));
    REQUIRE_NOTHROW(global.add_field<uint32_t>("third"));

    REQUIRE_THROWS( global.add_field<uint8_t>("first")); // can't add field of the same name again..
    REQUIRE_THROWS( global.add_field<uint16_t>("second"));  // can't add field of the same name again..
    REQUIRE_THROWS(global.add_field<uint32_t>("third"));  // can't add field of the same name again..
}


TEST_CASE("test using global packet", "should not throw and values should be as expected")
{
    memset(buffer, 0xff, buff_size); // set everything to ffff

    Packet& m = global; // reference - so we are using the global one

    REQUIRE(m.get_field("first") == 0xff );
    REQUIRE(m.get_field("second") == 0xffff );
    REQUIRE(m.get_field("third") == 0xffffffff );

    REQUIRE(m.get_field(0) == 0xff );
    REQUIRE(m.get_field(1) == 0xffff );
    REQUIRE(m.get_field(2) == 0xffffffff );


    REQUIRE_NOTHROW(m.set_field(0, 0xab ));
    REQUIRE_NOTHROW(m.set_field("second", 0xcdef ));
    REQUIRE_NOTHROW(m.set_field("third", 0x01234567 ));

    REQUIRE(m.get_field(0) ==  0xab );
    REQUIRE(m.get_field("second") ==  0xcdef );
    REQUIRE(m.get_field("third") == 0x01234567 );

}


TEST_CASE( "Copy global packet", "existing fields should appear in new packet" )
{
    Packet copy_of_m = global;

    REQUIRE(copy_of_m.get_field(0) ==  0xab );
    REQUIRE(copy_of_m.get_field("second") ==  0xcdef );
    REQUIRE(copy_of_m.get_field("third") == 0x01234567 );
}


TEST_CASE( "Create new from global", "existing fields should appear in new packet, should be able to extend packet" )
{
    Packet new_from_m (global);

    REQUIRE_THROWS( new_from_m.add_field<uint8_t>("first")); // can't add field of the same name again..
    REQUIRE_THROWS( new_from_m.add_field<uint16_t>("second"));  // can't add field of the same name again..
    REQUIRE_THROWS(new_from_m.add_field<uint32_t>("third"));  // can't add field of the same name again..

    REQUIRE_NOTHROW(new_from_m.add_field<uint32_t>("fourth"));
    REQUIRE_NOTHROW(new_from_m.add_field<uint8_t>("fifth"));

    // set values for new fields..
    REQUIRE_NOTHROW(new_from_m.set_field(3, 0x55664433 ));
    REQUIRE_NOTHROW(new_from_m.set_field("fifth", 0xde ));

    // confirm them back
    REQUIRE( new_from_m.get_field("fourth") == 0x55664433 );
    REQUIRE( new_from_m.get_field(4) == 0xde );

    // check old fields - they shouldn't change..
    REQUIRE(new_from_m.get_field(0) ==  0xab );
    REQUIRE(new_from_m.get_field("second") ==  0xcdef );
    REQUIRE(new_from_m.get_field("third") == 0x01234567 );

    std::cout << new_from_m << std::endl;
}


TEST_CASE( "Use strings in packets", "Should be able to use string fields in packets" )
{
    Packet new_from_m (global);

    REQUIRE_THROWS( new_from_m.add_field<uint8_t>("first")); // can't add field of the same name again..
    REQUIRE_THROWS( new_from_m.add_field<uint16_t>("second"));  // can't add field of the same name again..
    REQUIRE_THROWS(new_from_m.add_field<uint32_t>("third"));  // can't add field of the same name again..

    REQUIRE_NOTHROW(new_from_m.add_field<char*>("name", 10));
    REQUIRE_NOTHROW(new_from_m.add_field<char*>("city", 12));

    // set values for new fields..
    REQUIRE_NOTHROW(new_from_m.set_field("name", "John Doe"));
    REQUIRE_NOTHROW(new_from_m.set_field("city", "New York"));

    // confirm them back
    char buf[32];
    memset(buf, 0, 32);

    REQUIRE_NOTHROW(new_from_m.get_field("name", buf));
    REQUIRE(std::string(buf) == "John Doe");
    REQUIRE(std::string("John Doe") == static_cast<char*>(new_from_m.get_field("name", NULL) ));

    std::cout << new_from_m << std::endl;
}


TEST_CASE( "try with mismatched field types", "Should be able handle pointer/non-pointer types correctly" )
{
    Packet p(buffer, buff_size);
    REQUIRE_THROWS( p.add_field<char*>("pointer") ); // can't add pointer type without specifying length
    REQUIRE_NOTHROW( p.add_field<char*>("pointer", 10) ); // now OK with length > 0
    REQUIRE_NOTHROW( p.add_field<int>("non_pointer") );

    // confirm them back
    char buf[32];
    memset(buf, 0, 32);

    REQUIRE_NOTHROW(p.set_field("pointer", "John Doe")); // OK for pointer fields..
    REQUIRE_NOTHROW(p.get_field("pointer", buf));
    REQUIRE_THROWS(p.set_field("non_pointer", "John Doe")); // .. but not OK for non-pointer fields..
    REQUIRE_THROWS(p.get_field("non_pointer", buf));

    // similar for fields-by-id access
    REQUIRE_NOTHROW(p.set_field(0, "John Doe")); // OK for pointer fields..
    REQUIRE_NOTHROW(p.get_field(0, buf));
    REQUIRE_THROWS(p.set_field(1, "John Doe")); // .. but not OK for non-pointer fields..
    REQUIRE_THROWS(p.get_field(1, buf));
}


TEST_CASE( "test sub-packets", "Should be able handle sub-packets correctly" )
{
    // prepare packet
    Packet p(buffer, buff_size);
    REQUIRE_NOTHROW( p.add_field<char*>("full_name", 20) );
    REQUIRE_NOTHROW( p.add_field<int>("non_pointer") );
    REQUIRE_NOTHROW( p.set_field("full_name", (char*)NULL) );

    REQUIRE_THROWS( p.sub_packet("nonexisting")); // cant' create packet for non-existing field..
    REQUIRE_THROWS( p.sub_packet("non_pointer")); // cant' create packet for non-pointer field..

    REQUIRE_NOTHROW( p.sub_packet("full_name")); // OK

    Packet& sub = p.sub_packet("full_name"); // second time should just return it

    // confirm it's OK and boundaries do not exceed the original field
    REQUIRE( sub.get_buffer_addr() == p.get_field("full_name", NULL) ); // should point at the original field
    REQUIRE( sub.max_length() == 20 );

    REQUIRE_NOTHROW( sub.add_field<uint8_t*>("name", 10) ); // should behave as normal packet, obviously..
    REQUIRE_THROWS( sub.add_field<uint8_t*>("surname", 11) ); // too big - total of sub-fields should be no more than original one..
    REQUIRE_NOTHROW( sub.add_field<uint8_t*>("surname", 10) );

    std::cout << "whole packet:" << p;
}


TEST_CASE( "test sub-packets- adjust max size", "Adjusting maximum size." )
{
    // prepare packet
    Packet p(buffer, buff_size);
    REQUIRE_NOTHROW( p.add_field<char*>("full_name", 40) );
    REQUIRE_NOTHROW( p.add_field<int>("non_pointer") );
    REQUIRE_NOTHROW( p.set_field("full_name", (char*)NULL) );

    REQUIRE_THROWS( p.sub_packet("nonexisting")); // cant' create packet for non-existing field..
    REQUIRE_THROWS( p.sub_packet("non_pointer")); // cant' create packet for non-pointer field..

    REQUIRE_NOTHROW( p.sub_packet("full_name")); // OK

    Packet& sub = p.sub_packet("full_name"); // second time should just return it

    // confirm it's OK and boundaries do not exceed the original field
    REQUIRE( sub.get_buffer_addr() == p.get_field("full_name", NULL) ); // should point at the original field
    REQUIRE_FALSE( sub.max_length() == 20 );

    sub.adjust_max_length(30);
    REQUIRE( sub.max_length() == 30 );

    REQUIRE_NOTHROW( sub.add_field<uint8_t*>("name", 10) ); // should behave as normal packet, obviously..
    REQUIRE_NOTHROW( sub.add_field<uint8_t*>("surname", 11) ); // OK

    sub.adjust_max_length(); // not it should be a sum of the two fields that have been added above
    REQUIRE( sub.max_length() == 21 );

    std::cout << "whole packet:" << p;
}


TEST_CASE( "test sub-packets- copy fields", "Copying fields of a packet into another packet." )
{
    std::cout << "test copying fields" << "\n";
    // prepare packet
    Packet p(buffer, buff_size);
    REQUIRE_NOTHROW( p.add_field<char*>("full_name", 40) );
    REQUIRE_NOTHROW( p.add_field<int>("non_pointer") );
    REQUIRE_NOTHROW( p.set_field("full_name", (char*)NULL) );

    std::cout << "packet before copy: " << p << "\n";

    uint32_t len = p.length();

    char temp_buf[24];

    Packet p1(temp_buf, 24);
    REQUIRE_NOTHROW( p1.add_field<uint8_t>("first"));
    REQUIRE_NOTHROW( p1.add_field<uint16_t>("second"));
    REQUIRE_NOTHROW( p1.add_field<uint32_t>("third"));

    std::cout << "source of the copy: " << p1  << "\n";

    REQUIRE_NOTHROW( p.copy_fields(p1) );

    REQUIRE_THROWS( p.copy_fields(p1) ); // all copied fields already exist..
    REQUIRE (p.length() == len + p1.length() );

    std::cout << "result:" << p;

}
