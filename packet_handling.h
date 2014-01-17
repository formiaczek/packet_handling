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
#include <iomanip>
#include <algorithm>

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
    GenericException(const std::string& format,  ...)
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
    explicit Packet(void* buffer_addr, uint32_t buffer_size, uint32_t big_endian = 0) :
                    verbose_print(false),
                    msg_buffer((uint8_t*)buffer_addr, buffer_size, big_endian), // can throw exception..
                    cur_length(0),
                    packet_name("(no name)")
    {
    }

    /**
     @brief  Constructor.
     */
    explicit Packet() :
                    verbose_print(false),
                    msg_buffer(NULL, 0),
                    cur_length(0),
                    packet_name("(no name)")
    {
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
    void setup_buffer(void* new_buffer, uint32_t buffer_size, uint32_t big_endian = 0)
    {
        if (new_buffer != NULL && buffer_size > 0)
        {
            msg_buffer.setup_buffer((uint8_t*)new_buffer, buffer_size, big_endian);
        }
        else
        {
            throw GenericException(
                            "Packet(%s)::%s(): parameter error(new_buffer: 0x%lx, buffer_size %d)",
                            packet_name.c_str(), __FUNCTION__, new_buffer, buffer_size);
        }
    }

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
    int add_field(const std::string& name, uint32_t length = 0)
    {
        uint32_t field_id = -1;
        if (msg_buffer.get_buffer_addr() == NULL)
        {
            throw GenericException(
                            "Packet(%s)::%s(): buffer is not set. Should call Packet::setup_buffer() before this method..",
                            packet_name.c_str(), __FUNCTION__);
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
                                "Packet(%s)::%s(): field \"%s\": length is needed for pointer-type field",
                                packet_name.c_str(), __FUNCTION__, name.c_str());
            }
        }

        if (name.length() <= 0)
        {
            throw GenericException("Packet(%s)::%s(): field name is empty!",
                                   packet_name.c_str(), __FUNCTION__);
        }

        if (cur_length + length <= msg_buffer.max_length())
        {
            field_id = fields_by_id.size();
            PacketField f = new_PacketField<T>(cur_length, length, field_id);
            std::pair<std::map<std::string, PacketField>::iterator, bool> res = fields.insert(make_pair(name, f));
            if(res.second)
            {
                cur_length += length;
                fields_by_id.push_back(f);
            }
            else
            {
                std::string msg;
                if(fields.find(name) != fields.end())
                {
                    msg = "- field already exists";
                }

                throw GenericException("Packet(%s)::%s(): Error while adding new field: \"%s\" (size: 0x%x) %s",
                                       packet_name.c_str(), __FUNCTION__, name.c_str(), f.length, msg.c_str());
            }
        }
        else
        {
            throw GenericException("Packet(%s)::%s(): Packet is too short to add new field: \"%s\" (size: 0x%x)",
                                   packet_name.c_str(), __FUNCTION__, name.c_str(), length);
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
    void set_field(const std::string& name, uint32_t value)
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
            throw GenericException("Packet(%s)::%s(): field \"%s\" not found",
                                   packet_name.c_str(), __FUNCTION__, name.c_str());
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
            throw GenericException("Packet(%s)::%s(): field_index %d not found",
                                   packet_name.c_str(), __FUNCTION__, field_id);
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
    template <typename T>
    uint32_t set_field(const std::string& name, const T* src)
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
                                "Packet(%s)::%s(): wrong method for non-pointer type field (%s).",
                                packet_name.c_str(), __FUNCTION__, name.c_str());
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
            throw GenericException("Packet(%s)::%s(): field \"%s\" not found",
                                   packet_name.c_str(), __FUNCTION__, name.c_str());
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
    template <class T>
    uint32_t set_field(uint32_t field_id, const T* src)
    {
        uint32_t length_written = 0;
        if (field_id < fields_by_id.size())
        {
            // call the method it points to to access the data
            PacketField& f = fields_by_id[field_id];
            if (!f.is_pointer())
            {
                throw GenericException("Packet(%s)::%s(): wrong method for non-pointer type field (%d).",
                                       packet_name.c_str(), __FUNCTION__, field_id);
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
            throw GenericException("Packet(%s)::%s(): field_index %s not found",
                                   packet_name.c_str(), __FUNCTION__, field_id);
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
    uint32_t get_field(const std::string& name)
    {
        uint32_t val = 0;
        std::map<std::string, PacketField>::iterator i = fields.find(name);
        if (i != fields.end())
        {
            // if field is found, call the method it points to to access the data
            PacketField& f = i->second;
            if (f.is_pointer())
            {
                throw GenericException("Packet(%s)::%s(): wrong method for pointer type field (%s).",
                                       packet_name.c_str(), __FUNCTION__, name.c_str());
            }
            val = (msg_buffer.*f.get_val)(f.offset);
        }
        else
        {
            throw GenericException("Packet(%s)::%s(): field \"%s\" not found",
                                   packet_name.c_str(), __FUNCTION__, name.c_str());
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
                throw GenericException("Packet(%s)::%s(): wrong method for pointer type field (%d).",
                                       packet_name.c_str(), __FUNCTION__, field_id);
            }
            val = (msg_buffer.*f.get_val)(f.offset);
        }
        else
        {
            throw GenericException("Packet(%s)::%s(): field_index %s not found",
                                   packet_name.c_str(), __FUNCTION__, field_id);
        }
        return val;
    }

    /**
     @brief Copies the value of the field.
     @param  name: name of the field.
     @param  destination: pointer for the destination that the data should be copied.
             If NULL - only a pointer to data will be returned.
             (be careful not to modify the data past the boundaries!)
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     - field is not of pointer type.
     @return: Returns a pointer to the original data (if destination was NULL) or to the copied data otherwise.
     */
    void* get_field(const std::string& name, void* destination)
    {
        uint8_t* src_ptr = NULL;
        std::map<std::string, PacketField>::iterator i = fields.find(name);
        if (i != fields.end())
        {
            // if field is found, call the method it points to to access the data
            PacketField& f = i->second;
            if (!f.is_pointer())
            {
                throw GenericException(
                                "Packet(%s)::%s(): wrong method for non-pointer type field (%s).",
                                packet_name.c_str(), __FUNCTION__, name.c_str());
            }
            src_ptr = msg_buffer.get_uint8_ptr(f.offset);
            if(destination != NULL)
            {
                memcpy(destination, src_ptr, f.length);
                src_ptr = static_cast<uint8_t*>(destination); // return pointer to the copy
            }
        }
        else
        {

            throw GenericException("Packet(%s)::%s(): field \"%s\" not found",
                                   packet_name.c_str(), __FUNCTION__, name.c_str());
        }
        return src_ptr;
    }

    /**
     @brief Copies the value of the field.
     @param  id: id of the field.
     @param  destination: pointer for the destination of the data stored in the field.
     @throws: Can throw GenericException if following situations:
     - field does not exist in the Packet.
     - field is not of pointer type.
     @return: Returns a pointer to the original data (if destination was NULL) or to the copied data otherwise.
     */
    void* get_field(uint32_t field_id, void* destination)
    {
        uint8_t* src_ptr = NULL;
        if (field_id < fields_by_id.size())
        {
            // call the method it points to to access the data
            PacketField& f = fields_by_id[field_id];
            if (!f.is_pointer())
            {
                throw GenericException(
                                "Packet(%s)::%s(): wrong method for non-pointer type field (%d).",
                                packet_name.c_str(), __FUNCTION__, field_id);
            }

            src_ptr = msg_buffer.get_uint8_ptr(f.offset);
            if(destination != NULL)
            {
                memcpy(destination, src_ptr, f.length);
                src_ptr = static_cast<uint8_t*>(destination); // return pointer to the copy
            }
        }
        else
        {
            throw GenericException("Packet(%s)::%s(): field_index %s not found",
                                   packet_name.c_str(), __FUNCTION__, field_id);
        }
        return src_ptr;
    }

    /**
     @brief Method to check if a field exists in this packet.
     @param name - name of the field.
     @returns true - if exists, false otherwise.
     */
    bool field_exists(const std::string& name)
    {
        return fields.find(name) != fields.end();
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
    uint32_t get_field_id(const std::string& name, uint32_t no_throw = 1)
    {
        uint32_t field_id = ~0u;
        std::map<std::string, PacketField>::iterator i = fields.find(name);
        if (i != fields.end())
        {
            field_id = i->second.field_id;
            if (fields_by_id[field_id].field_id != field_id)
            {
                throw GenericException("Packet(%s)::%s(), Packet \"%s\": field \"%s\" has wrong field_id!",
                                       packet_name.c_str(), __FUNCTION__, name.c_str());
            }
        }
        else
        {
            if (!no_throw)
            {
                throw GenericException("Packet(%s)::%s(): field \"%s\" not found",
                                       packet_name.c_str(), __FUNCTION__, name.c_str());
            }
        }
        return field_id;
    }

    /**
     @brief Returns the offset at which this field starts within the Packet.
     @param  field_name: name of the field.
     @throws: Can throw GenericException if field is not found
     */
    uint32_t get_field_offset(const std::string& field_name)
    {
        uint32_t offset = 0;
        std::map<std::string, PacketField>::iterator i = fields.find(field_name);
        if (i != fields.end())
        {
            offset = i->second.offset;
        }
        else
        {
                throw GenericException("Packet(%s)::%s(): field \"%s\" not found",
                                       packet_name.c_str(), __FUNCTION__, field_name.c_str());
        }
        return offset;
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
     @brief  Method for getting a pointer to the actual buffer holding the data.
     (i.e. to be used for receiving / sending the data)
     */
    uint8_t* get_addr_for_next_field()
    {
        uint8_t* addr_for_next_field = msg_buffer.get_buffer_addr() + cur_length;
        return addr_for_next_field;
    }


    /**
     @brief  returns current length for the buffer.
     */
    uint32_t length()
    {
        return cur_length;
    }


    /**
     @brief  returns maximum length for the buffer.
     */
    uint32_t max_length()
    {
        return msg_buffer.max_length();
    }


    /**
     @brief  returns bytes that are available in this packet (as specified by a buffer constraints).
     */
    uint32_t bytes_left()
    {
        return msg_buffer.max_length() - cur_length;
    }


    /**
     @brief adjusts the maximum length of the buffer.
            Use this method to setup a lower value of the maximum buffer size.
     @param to_length - requested length. It it is bigger or equal to existing max_length,
            max_length will not change.
     */
    void adjust_max_length(uint32_t to_length)
    {
        if(to_length < msg_buffer.max_length())
        {
            msg_buffer.setup_buffer(msg_buffer.get_buffer_addr(), to_length);
        }
    }


    /**
     @brief adjusts the maximum length of the buffer to the current length.
            Use this method to setup the maximum buffer size to the current length.
     */
    void adjust_max_length()
    {
        if(cur_length < msg_buffer.max_length())
        {
            msg_buffer.setup_buffer(msg_buffer.get_buffer_addr(), cur_length);
        }
    }


    /**
     @brief  returns name of this packet.
     */
    std::string name()
    {
        return packet_name;
    }


    /**
     @brief  sets name of this packet.
     */
    void set_name(const std::string& new_name)
    {
        packet_name = new_name;
    }

    /**
     @brief Gets a reference to a sub-packet for a field.
            If sub-packet for this field does not exist yet, it will be created.
     @param name_of_existing_field - name of an existing field. Field has to be of a pointer type.
     @throws GenericException in following situations:
       - specified field does not exist,
       - specified field is not of a pointer type
       - other error while adding a sub-packet (e.g. in OOM)
     */
    Packet& sub_packet(const std::string& name_of_existing_field)
    {
        std::map<std::string, PacketField>::iterator fi = fields.find(name_of_existing_field);
        if (fi == fields.end())
        {
            throw GenericException("Packet(%s)::%s(): field \"%s\" does not exists!",
                                   packet_name.c_str(), __FUNCTION__, name_of_existing_field.c_str());
        }

        PacketField& f = fi->second;
        if (!f.is_pointer())
        {
            throw GenericException("Packet(%s)::%s(): can only create a sub-packet for fields of pointer type.",
                                   packet_name.c_str(), __FUNCTION__);
        }

        typedef std::map<std::string, Packet>::iterator fields_iterator;
        std::map<std::string, Packet>::iterator pi = sub_packets.find(name_of_existing_field);
        if(pi == sub_packets.end())
        {
            Packet packet(msg_buffer.get_uint8_ptr(f.offset), f.length);
            std::pair<fields_iterator, bool> res = sub_packets.insert(std::make_pair(name_of_existing_field, packet));
            if(res.second)
            {
                pi = res.first;
                pi->second.set_name("");
            }
            else
            {
                throw GenericException("Packet(%s)::%s(): Error creating sub_packet for \"%s\"",
                                       packet_name.c_str(), __FUNCTION__, name_of_existing_field.c_str());
            }
        }
        return pi->second;
    }

    /**
     * @brief Method to determine if a field has a sub-packet
     */
    bool has_sub_packet(const std::string& field_name)
    {
        bool result = false;
        if(sub_packets.size() && field_name.size())
        {
            result = sub_packets.find(field_name) != sub_packets.end();
        }
        return result;
    }


    /**
     * @brief Copy fields of another packet into this packet.
     * @param source_packet - All fields of this source packet will be appended to the current packet.
     */
    void copy_fields(Packet& source_packet)
    {
        std::vector<PacketField>::iterator fi;
        std::map<std::string, PacketField>::iterator pi;

        for(fi = source_packet.fields_by_id.begin(); fi != source_packet.fields_by_id.end(); fi++)
        {
            PacketField f = *fi; // local copy

            // find name
            for(pi = source_packet.fields.begin(); pi != source_packet.fields.end(); pi++)
            {
                if (pi->second.field_id == f.field_id)
                {
                    break;
                }
            }

            if (cur_length + f.length <= msg_buffer.max_length())
            {
                uint32_t field_id = fields_by_id.size();
                f.field_id = field_id; // adjust field_id
                std::pair<std::map<std::string, PacketField>::iterator, bool> res = fields.insert(make_pair(pi->first, f));
                if(res.second)
                {
                    cur_length += f.length;
                    fields_by_id.push_back(f);
                }
                else
                {
                    std::string msg;
                    if(fields.find(pi->first) != fields.end())
                    {
                        msg = "- field already exists";
                    }

                    throw GenericException("Packet::%s(): Error while adding new field: \"%s\" (size: 0x%x) %s",
                                           __FUNCTION__, pi->first.c_str(), f.length, msg.c_str());
                }
            }
            else
            {
                throw GenericException("Packet::%s(): Current packet is too short to add new field: \"%s\" (size: 0x%x)",
                                       __FUNCTION__, pi->first.c_str(), f.length);
            }
        }
    }


    /**
     * @brief Changes the name of a valid field.
     * @param old_name - name of the existing field. This field should exist in this packet.
     * @param new_name - new name for the selected field. Note, that field_id does not change, and old
     *                   name will not be available if this method executes correctly.
     * @throws GenericException if the original field does not exist or if the new one couldn't be added.
     */
    void rename_field(const std::string& old_name, const std::string& new_name)
    {
        std::map<std::string, PacketField>::iterator pi;
        pi = fields.find(old_name);
        if(pi == fields.end())
        {
            throw GenericException("Packet::%s(): Error, field: \"%s\" does not exist.", __FUNCTION__, old_name.c_str());
        }

        PacketField f = pi->second; // local copy
        std::pair<std::map<std::string, PacketField>::iterator, bool> res = fields.insert(make_pair(new_name, f));
        if(!res.second)
        {
            std::string msg = "other error?";
            if(fields.find(new_name) != fields.end())
            {
                msg = "- field already exists";
            }

            throw GenericException("Packet::%s(): Error when adding new field: \"%s\" %s",
                                   __FUNCTION__, new_name.c_str(), msg.c_str());
        }

        // only erase it when everything is OK
        fields.erase(pi); // (pi is valid: "only iterators and references to the erased elements are invalidated [23.2.4/9])
    }

    /**
     * @brief  declaration of insert operator to dump Packet info.
     */
    friend std::ostream& operator<<(std::ostream &out, Packet& m);


    /**
     * @brief sets a formatting prefix string.
     *        This prefix will be placed at the beginning of each line
     *        printed by the insert operator (<<).
     * @param prefix string to be used as prefix.
     */
    void set_formatting_prefix(std::string prefix)
    {
        formatting_prefix = prefix;
    }

    /**
     * @brief sets the verbose flag. This will cause that
     *        operator (<<) will insert more information about the packet.
     */
    void set_verbose()
    {
         verbose_print = true;
    }

    /**
     * @brief Clears the ferbose flag.
     */
    void clear_verbose()
    {
        verbose_print = false;
    }

    /**
     * @brief Returns the status of the verbose flag.
     */
    bool verbose()
    {
        return verbose_print;
    }

private:
    bool verbose_print;
    std::string formatting_prefix;
    PacketBuffer msg_buffer;
    uint32_t cur_length;
    std::string packet_name;
    std::map<std::string, PacketField> fields;
    std::vector<PacketField> fields_by_id;
    std::map<std::string, Packet> sub_packets;
};

/**
 * @brief  insert operator to dump Packet information in some more formatted way.
 */
inline std::ostream& operator<<(std::ostream &out,  Packet& m)
{
    const uint32_t default_prefix_size = 2;

    std::map<std::string, PacketField>::iterator f;
    std::vector<PacketField>::iterator i;

    size_t max_name_len = 0;
    for(f = m.fields.begin(); f != m.fields.end(); f++)
    {
        max_name_len = (std::max)(max_name_len, f->first.length());
    }
    max_name_len++;

    // print name and length
    out << "\n";
    std::string prefix = m.formatting_prefix;
    if(m.name().size() > 0)
    {
        out << prefix << m.name();
        out << ", total size: " << std::hex << std::showbase << m.length() << " :\n\n";
    }

    for (i = m.fields_by_id.begin(); i != m.fields_by_id.end(); i++)
    {

        for (f = m.fields.begin(); f != m.fields.end(); f++)
        {
            if (f->second.field_id == i->field_id)
            {
                break;
            }
        }

        if (m.verbose())
        {
            out << prefix << "--\n";
            out << prefix << "name:   " << f->first << std::endl;
            out << prefix << "id:     " << i->field_id << std::endl;
            out << prefix << "type:   " << i->type_name() << std::endl;
            out << prefix << "offset: " << i->offset << std::endl;
            out << prefix << "length: " << i->length << std::endl;
            out << prefix << "value";
        }
        else
        {
            out << prefix << f->first;
            out << prefix << std::string(max_name_len - f->first.length(), ' ');
        }

        if (i->is_pointer())
        {
            uint8_t* vals = m.msg_buffer.get_uint8_ptr(i->offset);
            out << prefix <<  ": (size " << std::hex << std::showbase << i->length << "): ";

            if(m.has_sub_packet(f->first))
            {
                Packet& sub = m.sub_packet(f->first);
                sub.set_formatting_prefix(m.formatting_prefix + std::string(default_prefix_size, ' '));
                out << sub;
            }
            else
            {
                std::string vals_s(default_prefix_size, ' ');
                if( i->length > 15)
                {
                    out << "\n" << prefix << std::string(max_name_len + 15, ' ');
                }
                uint32_t a;
                for (a = 0; a < i->length; a++)
                {
                    out << std::hex << std::noshowbase << std::setw(2)
                        << std::setfill('0') << (uint32_t)vals[a] << " ";

                    char to_put = vals[a];
                    if(to_put < 32 || to_put > 126)
                    {
                        to_put = '.';
                    }
                    vals_s.push_back(to_put);

                    if((a >= 15) && !(a%16 - 15))
                    {
                        out << vals_s;
                        vals_s.clear();
                        vals_s += "  ";
                        if(a < i->length - 1)
                        {
                            out << "\n" << prefix << std::string(max_name_len + 15, ' ');
                        }
                    }
                    if(a > 62)
                    {
                        out << " (..skipping the rest of data..) ";
                        break;
                    }
                }
                if(vals_s.size() > 2)
                {
                    // print the remaining values (if they haven't been yet)..
                    out << std::string(3 * (16 - (a%16)), ' ') << vals_s;
                }
            }
        }
        else
        {
            out << prefix << ": " << std::hex << std::showbase << m.get_field(i->field_id);
        }
        out << std::endl;
    }
    return out;
}

#endif /* PACKET_HANDLING_H_ */
