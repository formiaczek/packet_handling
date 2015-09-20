packet_handling
===============

This is a small library originally written to facilitate 
 access to the data serialized in packets of a fixed size.
 Providing a high-level interface it can be used for serializing
 any data.

 In some ways it is similar to google protocol buffers
 (https://developers.google.com/protocol-buffers/)

 but:
 - it is a C++ library, so it gets compiled with your program
 - the format / layout of the data is defined dynamically
   (no need to generate the it for a given format)
 - the actual data in the buffer does not contain any additional
   information - this information is only held by the Packet object.
   Packet is really like a 'template', or a 'parser', that 
   facilitates accessing various fields of the data.
 - it can be useful when implementing protocols 
   giving an easy interface to define data structures. 
 - down side is that format has to be known at both sides:
   - when reading / writing the data unless(*)...

 - (*)TODO: perhaps could add some code to create/handle a header,
    containing all format information needed to build a packet.
   This header could then be stored/transmitted and used to re-create
   a Packet at the 'reception' side?

Refer to in-source documentation and examples for more information. 

```c
/* Example:
 Example packet (Initialise Data Source-Packet I.D. 128
 for the GPS protocol) (message sent over serial port):

 Name                 Bytes     Units
 0    Packet ID       1
 1    ECEF X          4        Meters
 2    ECEF Y          4        Meters
 3    ECEF Z          4        Meters
 4    Clock Offset    4        Hz
 5    Time of Week    4        Seconds
 6    Week Number     2
 7    Channels        1
 8    Reset Config    1

 Could be defined as follows:
 @code
 uint8_t* buffer = new uint8_t[25];
 Packet gps_id_128(buffer, 25);

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

 // or by id:
 gps_id_128.set_field(7, 2); // id == 7 for channels
 gps_id_128.set_field(1, 2); // id == 2 for ECEF X

 std::cout << gps_id_128 << "\n"; // can also print it out..
/* Example output:

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
 */

```

Another example (multiple level of packets):

```c
    /* packet created as follows: */
    Packet all(buffer, total_size);
    all.set_name("Flat data packet");
    all.add_field<char*> ("payload", all.bytes_left()); // create one field only with remaining space

    // packet is just a way of interpreting / using the data- there could be more packetso
    // to interpret the same buffer
    Packet car(buffer, total_size);
    car.set_name("CAR");

    car.add_field<char*>("make", 9); // 9 bytes for make
    car.add_field<char*>("model", 9);

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

// could be printed as follows:
std::cout << "\n" << car << "\n";

/* output:

CAR, total size: 0x31 :
make      : (size 0x9): 50 6f 72 73 68 65 00 00 00                        Porshe...
model     : (size 0x9): 39 31 31 20 47 54 31 00 00                        911 GT1..
prod_year : 0x7d8
engine    : (size 0x1b):
  type        : (size 0x8): 66 6c 61 74 2d 36 00 00                           flat-6..
  fuel        : (size 0x8): 45 74 68 61 6e 6f 6c 00                           Ethanol.
  version     : (size 0x3): 00 00 00                                          ...
  params      : (size 0x6):
    ps                    : 0x220
    top speed mph         : 0xbf
    cylinders             : 0x6
*/


std::cout << all << "\n"; // print the whole payload data..

/* output:

Flat data packet, total size: 0x36 :
payload : (size 0x36):
                       50 6f 72 73 68 65 00 00 00 39 31 31 20 47 54 31   Porshe...911 GT1
                       00 00 d8 07 00 00 66 6c 61 74 2d 36 00 00 45 74   ......flat-6..Et
                       68 61 6e 6f 6c 00 00 00 00 20 02 bf 00 06 00 00   hanol.... ......
                       00 00 00 00 00 00                                 ......
 */

// or in JSON:
car.to_json(std::cout);

/* output:
{"name": "CAR", "total_size": 49, "fields": ["make": "Porshe", "model": "911 GT1", "prod_year": 2008, "engine": {"fields": ["type": "flat-6", "fuel": "Ethanol", "version": "", "params": {"fields": ["ps": 544, "top speed mph": 191, "cylinders": 6]}]}]}
*/


```




