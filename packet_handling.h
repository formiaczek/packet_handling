/**
 @file:   packet_handling.h
 @date:   28 Aug 2012
 @author: Lukasz Forynski

 @brief  This is a small library to facilitate extracting
 (parsing) and accessing the data formed in packets of
 a fixed format. It was mainly meant in order to provide
 an easy way for creating and using such packets.

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
 @endcode

 Similarly for reading.
 Endianess can be specified for the Packet, and also
 char* and other pointer type fields are supported.
 (Refer to code and documentation for more details).

 Nibbles and bits will appear here soon too hopefully. :)

 ___________________________

 Copyright (c) 2012 Lukasz Forynski <lukasz.forynski@gmail.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy of this
 software and associated documentation files (the "Software"), to deal in the Software
 without restriction, including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sub-license, and/or sell copies of the Software, and to permit persons
 to whom the Software is furnished to do so, subject to the following conditions:

 - The above copyright notice and this permission notice shall be included in all copies
 or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 */

#ifndef PACKET_HANDLING_H_
#define PACKET_HANDLING_H_

#include <stdint.h>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <stdio.h>

#include <typeinfo>
#include <stdarg.h>
#include <cstring>
#include <exception>
#include <iostream>

#ifdef _MSC_VER
#define VSNPRINTF(dest, max_len, format, vargs) vsnprintf_s(dest, max_len, max_len, format, vargs)
#define __PRETTY_FUNCTION__ __FUNCTION__
#else
#define VSNPRINTF(dest, max_len, format, vargs) vsnprintf(dest, max_len, format, vargs)
#endif



/**
 @brief  Default exception to allow to print some useful error info.
 */
class GenericException: public std::exception
{
public:

    /**
     @brief  Overloaded constructor. Use it as printf() for exceptions..
     */
    GenericException(std::string format, ...)
    {
        if(format.length() > 0)
        {
            va_list vArgs;
            va_start(vArgs, format);
            VSNPRINTF(msg, MAX_MSG_LEN, format.c_str(), vArgs);
            va_end(vArgs);
        }
    }

    /**
     @brief  Overridden method to print exception info..
     */
    virtual const char* what() const throw ()
    {
        return msg;
    }
protected:
    enum constants
    {
        MAX_MSG_LEN = 512
    };

    char msg[MAX_MSG_LEN];
};


/**
 * @brief template overloaded function used to determine if parameter is a pointer.
 */
template <class P>
bool is_pointer_type(P*)
{
    return true;
}

/**
 * @brief template overloaded function used to determine if parameter is a pointer.
 */
template <class R>
bool is_pointer_type(R)
{
    return false;
}


/**
 @brief  PacketBuffer class. This class will hold the data to be processed
 (i.e. buffer) and abstract operations that are to be performed on that data.
 */
class PacketBuffer
{
public:

    /**
     @brief  Constructor.
     @param   buffer_addr: pointer to the actual data buffer this Packet should use.
     @param   buffer_size: size of the data buffer.
     @param   big_endian: If set to true - all multi-byte fields will be read according to
     big_endian rules. Little_endian used otherwise.
     */
    explicit PacketBuffer(uint8_t* buffer_addr, uint32_t buffer_size, uint32_t big_endian = 0) :
                    max_len(buffer_size),
                    use_big_endian(big_endian),
                    buffer(buffer_addr)
    {
    }

    /**
     @brief  Method for getting a pointer to this buffer.
     (i.e. to be used for receiving / sending the data)
     */
    uint8_t* get_buffer_addr()
    {
        return buffer;
    }

    /**
     @brief sets a new buffer address
     */
    void setup_buffer(uint8_t* new_buffer, uint32_t buffer_size, uint32_t big_endian = 0)
    {
        if (new_buffer != NULL && buffer_size > 0)
        {
            buffer = new_buffer;
            max_len = buffer_size;
            use_big_endian = big_endian;
        }
        else
        {
            throw GenericException(
                            "Packet::%s() parameter error(new_buffer: 0x%lx, buffer_size %d)",
                            __FUNCTION__, new_buffer, buffer_size);
        }
    }

    /**
     @brief  returns maximum length for the buffer.
     */
    uint32_t max_length()
    {
        return max_len;
    }

protected:
    /**
     @brief  Default constructor.
     Hidden to allow only overloaded constructor to be used.
     */
    PacketBuffer() :
                    max_len(0),
                    use_big_endian(0),
                    buffer(NULL)
    {
    }

public:
    typedef uint32_t (PacketBuffer::*get_value)(uint32_t);
    typedef uint32_t (PacketBuffer::*set_value)(uint32_t, uint32_t);
    typedef uint8_t* (PacketBuffer::*get_ptr)(uint32_t);

    /**
     @brief  Returns one byte data stored in the buffer at at specified offset.
     @param   offset: offset from the beginning of the buffer.
     */
    uint32_t get_uint8(uint32_t offset)
    {
        return buffer[offset];
    }

    /**
     @brief  Returns two-byte data stored in the buffer at at specified offset.
     @param   offset: offset from the beginning of the buffer.
     */
    uint32_t get_uint16(uint32_t offset)
    {
        uint32_t value = buffer[offset++];
        if (use_big_endian)
        {
            value <<= 8;
            value |= buffer[offset];
        }
        else
        {
            value |= (uint16_t) buffer[offset] << 8;
        }
        return value;
    }

    /**
     @brief  Returns four-byte data stored in the buffer at at specified offset.
     @param   offset: offset from the beginning of the buffer.
     */
    uint32_t get_uint32(uint32_t offset)
    {
        uint32_t value;
        if (use_big_endian)
        {
            value = get_uint16(offset + 2);
            value |= (uint32_t) get_uint16(offset) << 16;
        }
        else
        {
            value = get_uint16(offset);
            value |= (uint32_t) get_uint16(offset + 2) << 16;
        }
        return value;
    }

    /**
     @brief  Returns a pointer to the item at specified offset.
     @param   offset: offset from the beginning of the buffer.
     */
    uint8_t* get_uint8_ptr(uint32_t offset)
    {
        return &buffer[offset];
    }

    /**
     @brief  Stores one byte in the buffer at at specified offset.
     @param   offset: offset from the beginning of the buffer.
     */
    uint32_t set_uint8(uint32_t offset, uint32_t value)
    {
        buffer[offset] = value;
        return 0;
    }

    /**
     @brief  Stores two bytes in the buffer at at specified offset.
     @param   offset: offset from the beginning of the buffer.
     */
    uint32_t set_uint16(uint32_t offset, uint32_t value)
    {
        if (use_big_endian)
        {
            buffer[offset++] = (value >> 8) & 0xff;
            buffer[offset] = value & 0xff;
        }
        else
        {
            buffer[offset++] = value & 0xff;
            buffer[offset] = (value >> 8) & 0xff;
        }
        return 0;
    }

    /**
     @brief  Stores four bytes in the buffer at at specified offset.
     @param   offset: offset from the beginning of the buffer.
     */
    uint32_t set_uint32(uint32_t offset, uint32_t value)
    {
        if (use_big_endian)
        {
            set_uint16(offset + 2, value & 0xffff);
            set_uint16(offset, (value >> 16) & 0xffff);
        }
        else
        {
            set_uint16(offset, value & 0xffff);
            set_uint16(offset + 2, (value >> 16) & 0xffff);
        }
        return 0;
    }

    /**
     @brief  Dummy get operation. All uninitialised fields will have their 'get_value'
     pointers set to point at this method.
     @param   offset: offset from the beginning of the buffer.
     */
    uint32_t dummy_get(uint32_t offset)
    {
        // assertion here perhaps?
        return 0;
    }

    /**
     @brief  Dummy get operation. All uninitialised fields will have their 'get_pointer'
     pointers set to point at this method.
     @param   offset: offset from the beginning of the buffer.
     */
    uint8_t* dummy_ptr(uint32_t offset)
    {
        // assertion here perhaps?
        return NULL;
    }

    /**
     @brief  Dummy set operation. All uninitialised fields will have their 'set_value'
     pointers set to point at this method.
     @param   offset: offset from the beginning of the buffer.
     */
    uint32_t dummy_set(uint32_t offset, uint32_t value)
    {
        //assertion here perhaps?
        return 0;
    }

protected:
    uint32_t max_len;
    uint32_t use_big_endian;
    uint8_t* buffer;
};

/**
 @brief  PacketField class.
 This class is meant to abstract Packet fields.
 Each of the fields defined for Packet will know it's type,
 offset within the buffer where its corresponding data is to be found
 and operations to access this data.
 */
class PacketField
{
public:
    /**
     @brief  Constructor.
     @param   starts_at_offset: offset from the beginning of the buffer for this field.
     @param   len: length of this field.
     @param   id: Id of the field. It will be used to identify the field within the Packet,
     i.e. the order of field.
     */
    PacketField() :
                    offset(0),
                    length(0),
                    field_id(0),
                    type(0),
                    pointer(false)
    {
        printf("\nPacketField() called!!\n");
        get_val = &PacketBuffer::dummy_get;
        set_val = &PacketBuffer::dummy_set;
        get_ptr = &PacketBuffer::dummy_ptr;
    }

    PacketField(uint32_t starts_at_offset, uint32_t len, uint32_t id = -1) :
                    offset(starts_at_offset),
                    length(len),
                    field_id(id),
                    type(0),
                    pointer(false)
    {
        get_val = &PacketBuffer::dummy_get;
        set_val = &PacketBuffer::dummy_set;
        get_ptr = &PacketBuffer::dummy_ptr;
    }

public:

    /**
     @brief  Template method used to set-up (store) the type this field is defined for.
     @param (template) T: type this field is representing.
     */
    template<typename T>
    void set_type_info()
    {
        type = const_cast<std::type_info*>(&typeid(T));
        pointer = is_pointer_type(T());
    }

    /**
     @brief  Template method used to check if this field is of a specific type.
     @param (template) T: reference type used for comparison.
     */
    template<typename T>
    bool is_my_guessed_type()
    {
        bool res = false;
        if (type && type == &typeid(T))
        {
            res = true;
        }
        return res;
    }

    /**
     @brief Returns name of the type (std::type_info::name()) this field represents.
     */
    const char* type_name()
    {
        if (type)
        {
            return type->name();
        }
        return NULL;
    }

    bool is_pointer()
    {
        return pointer;
    }

    uint32_t offset;
    uint32_t length;
    uint32_t field_id;
    std::type_info* type;
    bool pointer;
    PacketBuffer::get_value get_val;
    PacketBuffer::set_value set_val;
    PacketBuffer::get_ptr get_ptr;
};

/**
 @brief  Template method used to create a field of a specific type.
 Since the set of operation requires more than compiler could do resolving macros,
 this template is not going to be used, and specialisations will be provided for specified
 types.
 */
template<typename T>
inline PacketField new_PacketField(uint32_t offset, uint32_t len, uint32_t field_id)
{
    return PacketField(offset, len, field_id);
}

/**
 @brief Specialisation of a template method for creating fields for <uint32_t> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<uint32_t>(uint32_t offset, uint32_t len, uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<uint32_t>();
    f.get_val = &PacketBuffer::get_uint32;
    f.set_val = &PacketBuffer::set_uint32;
    return f;
}

/**
 @brief Specialisation of a template method for creating fields for <int32_t> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<int32_t>(uint32_t offset, uint32_t len, uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<int32_t>();
    f.get_val = &PacketBuffer::get_uint32;
    f.set_val = &PacketBuffer::set_uint32;
    return f;
}

/**
 @brief Specialisation of a template method for creating fields for <uint16_t> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<uint16_t>(uint32_t offset, uint32_t len, uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<uint16_t>();
    f.get_val = &PacketBuffer::get_uint16;
    f.set_val = &PacketBuffer::set_uint16;
    return f;
}

/**
 @brief Specialisation of a template method for creating fields for <int16_t> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<int16_t>(uint32_t offset, uint32_t len, uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<int16_t>();
    f.get_val = &PacketBuffer::get_uint16;
    f.set_val = &PacketBuffer::set_uint16;
    return f;
}

/**
 @brief Specialisation of a template method for creating fields for <uint8_t> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<uint8_t>(uint32_t offset, uint32_t len, uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<uint8_t>();
    f.get_val = &PacketBuffer::get_uint8;
    f.set_val = &PacketBuffer::set_uint8;
    return f;
}

/**
 @brief Specialisation of a template method for creating fields for <int8_t> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<int8_t>(uint32_t offset, uint32_t len, uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<int8_t>();
    f.get_val = &PacketBuffer::get_uint8;
    f.set_val = &PacketBuffer::set_uint8;
    return f;
}

/**
 @brief Specialisation of a template method for creating fields for <uint8_t*> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<uint8_t*>(uint32_t offset, uint32_t len, uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<uint8_t*>();
    f.get_ptr = &PacketBuffer::get_uint8_ptr;
    return f;
}

/**
 @brief Specialisation of a template method for creating fields for <int8_t*> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<int8_t*>(uint32_t offset, uint32_t len, uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<int8_t*>();
    f.get_ptr = &PacketBuffer::get_uint8_ptr;
    return f;
}

/**
 @brief Specialisation of a template method for creating fields for <char*> type.
 @param  offset: Offset from the beginning of the Packet for this field.
 */
template<>
inline PacketField new_PacketField<char*>(uint32_t offset, uint32_t len,
        uint32_t field_id)
{
    PacketField f(offset, len, field_id);
    f.set_type_info<char*>();
    f.get_ptr = &PacketBuffer::get_uint8_ptr;
    return f;
}

/**
 @brief Packet class.
 Objects of this class will hold all the information needed to access the data,
 providing the interface to create and access fields.

 Two main ways to access Packet fields are available:
 1. By field name.
 If field is accessed by name, get/set operations have O(log n)
 complexity (lookup implemented with std::map).

 2. By field id. The ID is the actual number of the field since the beggining of
 the Packet (starting from 0).
 Get/set operations have O(1) complexity (no lookup, direct indexing)
 */
class Packet
{
public:
    /**
     @brief  Constructor.
     @param   buffer_addr: pointer to the actual buffer this Packet should be representing.
     @param   buffer_size: size of the buffer.
     @param   big_endian: endianness specification.
     @throws: Can throw GenericException (from PacketBuffer) if buffer_addr is NULL.
     */
    explicit Packet(uint8_t* buffer_addr, uint32_t buffer_size, uint32_t big_endian = 0) :
                    msg_buffer(buffer_addr, buffer_size, big_endian), // can throw exception..
                    cur_length(0)
    {
    }

    /**
     @brief  Constructor.
     */
    explicit Packet() :
                    msg_buffer(NULL, 0),
                    cur_length(0)
    {
        printf("\n\n===Packet() called\n\n===\n");
    }

    /**
     @brief Destructor.
     */
    ~Packet()
    {
    }

    /**
     @brief sets a new buffer to be used with this Packet
     */
    void setup_buffer(uint8_t* new_buffer, uint32_t buffer_size, uint32_t big_endian = 0)
    {
        if (new_buffer != NULL && buffer_size > 0)
        {
            msg_buffer.setup_buffer(new_buffer, buffer_size, big_endian);
        }
        else
        {
            throw GenericException(
                            "Packet::%s() parameter error(new_buffer: 0x%lx, buffer_size %d)",
                            __FUNCTION__, new_buffer, buffer_size);
        }
    }

public:

    /**
     @brief Interface for adding a named field.
     @param (template) T: specify type of the field being added.
     @param  name: name of the field.
     @param  length: length of the field.
     @throws: Can throw GenericException if following situations:
     - field being added is of pointer type but length was not specified
     - field being added has a zero-length name
     - field of specified has already been defined.
     - Packet has no more space for the field being added.
     */
    template<typename T>
    int add_field(std::string name, uint32_t length = 0)
    {
        uint32_t field_id = -1;
        if (msg_buffer.get_buffer_addr() == NULL)
        {
            throw GenericException(
                            "Packet::%s() buffer is not set. Should call Packet::setup_buffer() before this method..",
                            __FUNCTION__);
        }

        if (!is_pointer_type(T()))
        {
            if (length == 0)
            {
                length = sizeof(T);
            }
        }
        else
        {
            if (length == 0)
            {
                throw GenericException(
                                "Packet::%s() field \"%s\": length is needed for pointer-type field",
                                __FUNCTION__, name.c_str());
            }
        }

        if (name.length() <= 0)
        {
            throw GenericException("Packet::%s(): field name is empty!", __FUNCTION__);
        }

        if (fields.find(name) != fields.end())
        {
            throw GenericException("Packet::%s(): field \"%s\" already exists!", __FUNCTION__,
                                   name.c_str());
        }

        if (cur_length + length <= msg_buffer.max_length())
        {
            field_id = fields_by_id.size();
            PacketField f = new_PacketField<T>(cur_length, length, field_id);
            fields.insert(make_pair(name, f));
            cur_length += length;
            fields_by_id.push_back(f);

        }
        else
        {
            throw GenericException(
                            "Packet::%s(): Packet too short to add new field: \"%s\" (size: %d)",
                            __FUNCTION__, name.c_str(), length);
        }
        return field_id;
    }

    /**
     @brief Returns the id (order number within the Packet) of the field.
     @param  name: name of the field.
     @param  no_throw: if specified - method returns -1 if field is not found.
     @throws: Can throw GenericException in following situations:
     - field is not found and no_throw was set to 0
     - field being requested has a mismatched field_id with the one
     that Packet is using to identify it within the vector.
     */
    uint32_t get_field_id(std::string name, uint32_t no_throw = 1)
    {
        uint32_t field_id = -1;
        std::map<std::string, PacketField>::iterator i = fields.find(name);
        if (i != fields.end())
        {
            field_id = i->second.field_id;
            if (fields_by_id[field_id].field_id != field_id)
            {
                throw GenericException("Packet::%s(): field \"%s\" has wrong field_id!",
                                       __FUNCTION__, name.c_str());
            }
        }
        else
        {
            if (!no_throw)
            {
                throw GenericException("Packet::%s(): field \"%s\" not found", __FUNCTION__,
                                       name.c_str());
            }
        }
        return field_id;
    }

    /**
     @brief Updates the value of the field.
     @param  name: name of the field.
     @param  value: new value for the field.
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     */
    void set_field(std::string name, uint32_t value)
    {
        std::map<std::string, PacketField>::iterator i = fields.find(name);
        if (i != fields.end())
        {
            // if field is found, call the method it points to to access the data
            PacketField& f = i->second;
            (msg_buffer.*f.set_val)(f.offset, value);
        }
        else
        {
            throw GenericException("Packet::%s(): field \"%s\" not found", __FUNCTION__,
                                   name.c_str());
        }
    }

    /**
     @brief Updates the value of the field.
     @param  field_id: id of the field.
     @param  value: new value for the field.
     @throws: Can throw GenericException if following situations:
     - field pointed by id does not exist in the Packet.
     */
    void set_field(uint32_t field_id, uint32_t value)
    {
        if (field_id < fields_by_id.size())
        {
            // call the method it points to to access the data
            PacketField& f = fields_by_id[field_id];
            (msg_buffer.*f.set_val)(f.offset, value);
        }
        else
        {
            throw GenericException("Packet::%s(): field_index %d not found", __FUNCTION__,
                                   field_id);
        }
    }

    /**
     @brief Updates the value of the field.
     @param  name: name of the field.
     @param  src: pointer to the source data the field should be updated from.
     Note, that length is not needed, as the field has a fixed length,
     and so there is no requirement for the null-termination etc.
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     - field is not of pointer type.
     @return: Returns the length of data (in bytes) that was copied to the Packet.
     */
    uint32_t set_field(std::string name, const char* src)
    {
        uint32_t length_written = 0;
        std::map<std::string, PacketField>::iterator i = fields.find(name);
        if (i != fields.end())
        {
            // if field is found, call the method it points to to access the data
            PacketField& f = i->second;
            if (!f.is_pointer())
            {
                throw GenericException(
                                "Packet::%s(): wrong method for non-pointer type field (%s).",
                                __FUNCTION__, name.c_str());
            }
            uint8_t* dst_ptr = msg_buffer.get_uint8_ptr(f.offset);
            if (src != NULL)
            {
                memcpy(dst_ptr, src,  f.length);
            }
            else
            {
                memset(dst_ptr, 0u, f.length);
            }
            length_written = f.length;
        }
        else
        {
            throw GenericException("Packet::%s(): field \"%s\" not found", __FUNCTION__,
                                   name.c_str());
        }
        return length_written;
    }

    /**
     @brief Updates the value of the field.
     @param  id: id of the field.
     @param  src: pointer to the source data the field should be updated from.
     Note, that length is not needed, as the field has a fixed length,
     and so there is no requirement for the null-termination etc.
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     - field is not of pointer type.
     @return: Returns the length of data (in bytes) that was copied to the Packet.
     */
    uint32_t set_field(uint32_t field_id, const char* src)
    {
        uint32_t length_written = 0;
        if (field_id < fields_by_id.size())
        {
            // call the method it points to to access the data
            PacketField& f = fields_by_id[field_id];
            if (!f.is_pointer())
            {
                throw GenericException(
                                "Packet::%s(): wrong method for non-pointer type field (%d).",
                                __FUNCTION__, field_id);
            }
            uint8_t* dst_ptr = msg_buffer.get_uint8_ptr(f.offset);
            if (src != NULL)
            {
                memcpy(dst_ptr, src, f.length);
            }
            else
            {
                memset(dst_ptr, 0, f.length);
            }
            length_written = f.length;
        }
        else
        {
            throw GenericException("Packet::%s(): field_index %s not found", __FUNCTION__,
                                   field_id);
        }
        return length_written;
    }

    /**
     @brief Gets the value of the field from the Packet.
     @param  name: name of the field.
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     - field is a pointer type (these should be accessed by other
     overloads of this method.
     @return: Returns the actual value for this field.
     */
    uint32_t get_field(std::string name)
    {
        uint32_t val = 0;
        std::map<std::string, PacketField>::iterator i = fields.find(name);
        if (i != fields.end())
        {
            // if field is found, call the method it points to to access the data
            PacketField& f = i->second;
            if (f.is_pointer())
            {
                throw GenericException("Packet::%s(): wrong method for pointer type field (%s).",
                                       __FUNCTION__, name.c_str());
            }
            val = (msg_buffer.*f.get_val)(f.offset);
        }
        else
        {
            throw GenericException("Packet::%s(): field \"%s\" not found", __FUNCTION__,
                                   name.c_str());
        }
        return val;
    }

    /**
     @brief Gets the value of the field from the Packet.
     @param  id: id of the field.
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     - field is a pointer type (these should be accessed by other
     overloads of this method.
     @return: Returns the actual value for this field.
     */
    uint32_t get_field(uint32_t field_id)
    {
        uint32_t val = 0;
        if (field_id < fields_by_id.size())
        {
            // call the method it points to to access the data
            PacketField& f = fields_by_id[field_id];
            if (f.is_pointer())
            {
                throw GenericException("Packet::%s(): wrong method for pointer type field (%d).",
                                       __FUNCTION__, field_id);
            }
            val = (msg_buffer.*f.get_val)(f.offset);
        }
        else
        {
            throw GenericException("Packet::%s(): field_index %s not found", __FUNCTION__,
                                   field_id);
        }
        return val;
    }

    /**
     @brief Copies the value of the field.
     @param  name: name of the field.
     @param  destination: pointer for the destination of the data stored in the field.
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     - field is not of pointer type.
     - destination is NULL
     @return: Returns a pointer to the copied data.
     */
    void* get_field(std::string name, void* destination)
    {
        if (destination == NULL)
        {
            throw GenericException("Packet::%s(): destination for field (%s) is NULL",
                                   __FUNCTION__, name.c_str());
        }

        std::map<std::string, PacketField>::iterator i = fields.find(name);
        if (i != fields.end())
        {
            // if field is found, call the method it points to to access the data
            PacketField& f = i->second;
            if (!f.is_pointer())
            {
                throw GenericException(
                                "Packet::%s(): wrong method for non-pointer type field (%s).",
                                __FUNCTION__, name.c_str());
            }
            uint8_t* src_ptr = msg_buffer.get_uint8_ptr(f.offset);
            memcpy(destination, src_ptr, f.length);
        }
        else
        {

            throw GenericException("Packet::%s(): field \"%s\" not found", __FUNCTION__,
                                   name.c_str());
        }
        return destination;
    }

    /**
     @brief Copies the value of the field.
     @param  id: id of the field.
     @param  destination: pointer for the destination of the data stored in the field.
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     - field is not of pointer type.
     @return: Returns a pointer to the copied data.
     */
    void* get_field(uint32_t field_id, void* destination)
    {
        if (field_id < fields_by_id.size())
        {
            // call the method it points to to access the data
            PacketField& f = fields_by_id[field_id];
            if (!f.is_pointer())
            {
                throw GenericException(
                                "Packet::%s(): wrong method for non-pointer type field (%d).",
                                __FUNCTION__, field_id);
            }
            uint8_t* src_ptr = msg_buffer.get_uint8_ptr(f.offset);
            memcpy(destination, src_ptr, f.length);
        }
        else
        {
            throw GenericException("Packet::%s(): field_index %s not found", __FUNCTION__,
                                   field_id);
        }
        return destination;
    }

    /**
     @brief  Method for getting a pointer to the actual buffer holding the data.
     (i.e. to be used for receiving / sending the data)
     */
    uint8_t* get_buffer_addr()
    {
        return msg_buffer.get_buffer_addr();
    }

    /**
     @brief  returns current lenght for the buffer.
     */
    uint32_t length()
    {
        return cur_length;
    }

    /**
     * @brief  declaration of insert operator to dump Packet info.
     */
    friend std::ostream& operator<<(std::ostream &out, Packet& m);

private:
    PacketBuffer msg_buffer;
    uint32_t cur_length;
    std::map<std::string, PacketField> fields;
    std::vector<PacketField> fields_by_id;
};

/**
 * @brief  insert operator to dump Packet info.
 */
inline std::ostream& operator<<(std::ostream &out,  Packet& m)
{
    std::map<std::string, PacketField>::iterator f;
    std::vector<PacketField>::iterator i;
    long buf_addr = reinterpret_cast<long>(m.msg_buffer.get_buffer_addr());
    out << "buffer address: " << std::hex << std::showbase << buf_addr << std::endl;
    out << "buffer size   : " << m.msg_buffer.max_length() << std::endl;

    for (i = m.fields_by_id.begin(); i != m.fields_by_id.end(); i++)
    {

        for (f = m.fields.begin(); f != m.fields.end(); f++)
        {
            if (f->second.field_id == i->field_id)
            {
                break;
            }
        }

        if (false) // TODO:verbose)
        {
            out << "--\n";
            out << "name:   " << f->first.c_str() << std::endl;
            out << "id:     " << i->field_id << std::endl;
            out << "type:   " << i->type_name() << std::endl;
            out << "offset: " << i->offset << std::endl;
            out << "length: " << i->length << std::endl;
            out << "value";
        }
        else
        {
            out << f->first.c_str();
        }

        if (i->is_pointer())
        {
            uint8_t* vals = m.msg_buffer.get_uint8_ptr(i->offset);
            if (false) // TODO: verbose)
            {
                printf("s");
            }
            out << ": (size " << std::hex << std::showbase << i->length << "): ";

            bool has_null_termination = false;
            for (uint32_t a = 0; a < i->length; a++)
            {
                out << std::hex << std::noshowbase << (uint32_t)vals[a] << " ";
                if(vals[a]==0)
                {
                    has_null_termination = true;
                }
            }
            // try to print it all if it's null-terminated (perhaps it's a string?)
            if(has_null_termination)
                {
                out << "(" << vals << ")";
                }
        }
        else
        {
            out << ":  " << std::hex << std::showbase << m.get_field(i->field_id);
        }
        out << std::endl;
    }
    return out;
}

#endif /* PACKET_HANDLING_H_ */
