/*
 * example0.cpp
 *
 *  Created on: 2012-11-18
 *  Author: lukasz.forynski@gmail.com
 */


#include <iostream>
#include <string>
#include <stdio.h>

#include "packet_handling.h"


void simple_example()
{
    std::cout << __FUNCTION__ << ":\n";
    uint8_t* buffer = new uint8_t[25];
    memset(buffer, 0, 25);

    Packet gps_id_128(buffer, 25);
    gps_id_128.set_name("GPS 128");

    gps_id_128.add_field<uint8_t> ("Packet ID");
    gps_id_128.add_field<uint32_t>("ECEF X");
    gps_id_128.add_field<uint32_t>("ECEF Y");
    gps_id_128.add_field<uint32_t>("ECEF Z");
    gps_id_128.add_field<uint32_t>("Clock Offset");
    gps_id_128.add_field<uint32_t>("Time of Week");
    gps_id_128.add_field<uint16_t>("Week Number");
    gps_id_128.add_field<uint8_t> ("Channels");
    gps_id_128.add_field<uint8_t> ("Reset Config");

    // from now on, these fields can be accessed either by the name:
    gps_id_128.set_field("Time of Week", 0xffeb3fe3); // to set the field.
    gps_id_128.set_field(7, 2); // id == 7 for channels 
    gps_id_128.set_field(1, 2); // id == 2 for ECEF X

    std::cout << gps_id_128 << "\n"; // can also print it out..

    // can now do whatever is needed with the buffer.. 

    delete [] buffer;
}



void other_example()
{
    std::cout << __FUNCTION__ << ":\n";
    const unsigned int total_size = 64;
    char* buffer = new char[total_size];
    memset(buffer, 0, total_size);

    Packet all(buffer, total_size);
    all.set_name("Flat data packet");
    all.add_field<char*> ("payload", 32); // create one field, 32 bytes long..

    // packet is just a way of interpreting / using the data- there could be more packetso
    // to interpret the same buffer
    Packet car(buffer, total_size);
    car.set_name("CAR");

    car.add_field<char*>("make",   10); // make up to 15 chars
    car.add_field<char*>("model",   10); //

    car.add_field<int>("prod_year");
    car.add_field<char*>("engine", 27);

    // can add sub-packets within ptr fields:
    Packet& engine = car.sub_packet("engine");
    engine.add_field<char*>("type", 8);
    engine.add_field<char*>("fuel", 8);
    engine.add_field<char*>("version", 3);

    engine.add_field<char*>("params", 6); // 6 bytes for params
    Packet& engine_params = engine.sub_packet("params");
    engine_params.add_field<short>("ps");
    engine_params.add_field<short>("top speed mph");
    engine_params.add_field<short>("cylinders");

    // accessing fields
    car.set_field("make", "Porshe", sizeof("Porshe"));
    car.set_field("model", "911 GT1", sizeof("911 GT1"));
    car.set_field("prod_year", 2008);

    engine_params.set_field("cylinders", 6);
    engine_params.set_field("top speed mph", 191);

    // can also access sub-packets by traversing from the root..
    car.sub_packet("engine").set_field("fuel", "Ethanol", 7);
    car.sub_packet("engine").set_field("type", "flat-6", 6);

    car.sub_packet("engine").sub_packet("params").set_field("ps", 544);


    std::cout << "\n" << car << "\n"; // again will nicely format the output..

    std::cout << all << "\n"; // print the whole payload data..

    // (probably car is not the best example here, but analysing data that would have sub-packets for different payload
    // can be convenient for debugging etc..)

    delete [] buffer;
}


int main(int argc, char **argv)
{
    simple_example();
    other_example();
    return 0;
}

/* Example output(s):

simple_example:

GPS 128, total size: 0x19 :

Packet ID    : 0
ECEF X       : 0x2
ECEF Y       : 0
ECEF Z       : 0
Clock Offset : 0
Time of Week : 0xffeb3fe3
Week Number  : 0
Channels     : 0x2
Reset Config : 0



other_example:

CAR, total size: 0x33 :

make      : (size 0xa): 50 6f 72 73 68 65 00 00 00 00                     Porshe....
model     : (size 0xa): 39 31 31 20 47 54 31 00 00 00                     911 GT1...
prod_year : 0x7d8
engine    : (size 0x1b):
  type        : (size 0x8): 66 6c 61 74 2d 36 00 00                           flat-6..
  fuel        : (size 0x8): 45 74 68 61 6e 6f 6c 00                           Ethanol.
  version     : (size 0x3): 00 00 00                                          ...
  params      : (size 0x6):
    ps                    : 0x220
    top speed mph         : 0xbf
    cylinders             : 0x6



Flat data packet, total size: 0x20 :

payload : (size 0x20):
                       50 6f 72 73 68 65 00 00 00 00 39 31 31 20 47 54   Porshe....911 GT
                       31 00 00 00 d8 07 00 00 66 6c 61 74 2d 36 00 00   1.......flat-6..

 */

