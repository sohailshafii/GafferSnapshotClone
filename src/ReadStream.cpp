#include "ReadStream.h"
#include "BitOperationsCommon.h"
#include <cassert>

// https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6

ReadStream::ReadStream(const void* data, int bytesTotal)
    : bitsRead(0), reader(data, bytesTotal)
{
}

void ReadStream::Reset() {
    bitsRead = 0;
    reader.Reset();
}

void ReadStream::DeserializeInteger(int32_t& value, int32_t min, int32_t max) {
    assert(min < max);
    assert(value >= min);
    assert(value <= max);
    const int bits = bits_required(min, max);
    uint32_t unsignedValue = reader.ReadBits(bits);
    value = (uint32_t)unsignedValue + min;
    bitsRead += bits;
}

void ReadStream::DeserializeBits(uint32_t& value, int bits) {
    assert(bits > 0);
    assert(bits <= BITS_PER_WORD);
    uint32_t readValue = reader.ReadBits(bits);
    value = readValue;
    bitsRead += bits;
}

void ReadStream::DeserializeBytes(uint8_t* data, int bytes) {
    Align();
    reader.ReadBytes(data, bytes);
    bitsRead += bytes * BITS_PER_BYTE;
}

bool ReadStream::Check(uint32_t magic) {
    Align();
    uint32_t value = 0;
    DeserializeBits(value, BITS_PER_WORD);
    assert(value == magic);
    return value == magic;
}
