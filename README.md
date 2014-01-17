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

 See example.cpp for another example with sub-packets etc.



