packet_handling
===============

This is a small library to facilitate extracting (parsing)
 and accessing the data formed in packets of a fixed format.
 It was mainly meant in order to provide an easy way for 
 creating and using such packets.

 In some way it is similar to google protocol buffers
 (https://developers.google.com/protocol-buffers/),
 but:
 - the format / layout of buffer(s) can be created dynamically
 - the actual data in the buffer does not contain any additional
   information - this information is only held by the Packet object
 - Packet object is really like a 'template', or a parser that facilitates
   accessing various fields of the actual data.
 - it can help with implementing various protocols

 Example:
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