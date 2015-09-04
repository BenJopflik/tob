#include "frame.hpp"
#include <endian.h>
#include <arpa/inet.h>
#include <string.h>
#include <functional>

Frame::Frame()
{

}

Frame::~Frame()
{

}

std::shared_ptr<uint8_t> Frame::serialize(const uint8_t * payload, uint64_t & size)
{
    uint8_t length = 0;
    uint8_t length_ext = 0;

    if (size <= 0x7D)
    {
        length = size;
    }
    else if (size > 0xFFFF)
    {
        length = 0x7E;
        length_ext = 2;
    }
    else
    {
        length = 0x7F;
        length_ext = 8;
    }

    payload_length = size;

    uint64_t full_size = size + 2 + (masked ? 4 : 0) + length_ext;
    size = full_size;
    std::shared_ptr<uint8_t> result(new uint8_t[full_size], [](uint8_t * ptr){delete [] ptr;});
    memset(result.get(), 0, full_size);

    uint64_t offset = 0;
    uint8_t * data = result.get();
    if (fin)
        data[offset] = 1 << 7;
    data[offset++] |= type & 0x0f;
    if (masked)
        data[offset] = 1 << 7;
    data[offset++] |= length;
    if (length_ext)
    {
        if (length_ext == 2)
        {
            *(uint32_t *)(data + offset) = htons(payload_length);
            offset += 2;
        }
        else
        {
            *(uint64_t *)(data + offset) = htobe64(payload_length);
            offset += 8;
        }
    }

    if (masked)
    {
        *(uint32_t *)(data + offset) = htonl(mask_key);
        offset += 4;
    }

    memcpy(data + offset, payload, payload_length);

    if (masked)
    {
        uint64_t i = 0;
        for (; i + sizeof(mask_key) < payload_length; i += sizeof(mask_key))
            *(uint32_t *)(data + offset + i) ^= mask_key;

        for (; i < payload_length; ++i)
            *(uint8_t *)(data + offset + i) ^= ((uint8_t *)(&mask_key))[i % 4];
    }
    return result;
}

uint8_t * Frame::deserialize(uint8_t * data, uint64_t size)
{
    if (size < 2)
        return data;

    uint8_t * current = data;
    const uint8_t * const END = data + size;

    fin  = *current & 0x80;
    type = *current & 0x0f;
    ++current;
    masked = *current & 0x80;
    payload_length = *current & 0x7f;
    ++current;

    if (current >= END)
        return data;

    if (payload_length == 0x7E)
    {
        if (current + 2 >= END)
            return data;

        payload_length = ntohs(*(uint16_t *)current);
        current += 2;
    }
    else if (payload_length == 0x7F)
    {
        if (current + 8 >= END)
            return data;

        payload_length = be64toh(*(uint64_t *)current);
        current += 8;
    }

    if ((current + payload_length + ((masked) ? sizeof(mask_key) : 0)) > END)
        return data;

    mask_key = 0;
    if (masked)
    {
        mask_key = ntohl(*(uint32_t *)current);
        current += 4;
        uint64_t i = 0;
        for (; i + 4 < payload_length; i += 4)
            *(uint32_t *)(current + i) ^= mask_key;

        for (; i < payload_length; ++i)
            *(uint8_t *)(current + i) ^= ((uint8_t *)(&mask_key))[i % 4];
    }
    payload = current;
    return payload + payload_length;
}
