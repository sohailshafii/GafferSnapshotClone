#include "WriteStream.h"

// https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6

WriteStream::WriteStream(void* data, int bytesTotal)
    : writer(data, bytesTotal)
{
}

void WriteStream::Reset() {
    writer.Reset();
}

void WriteStream::SerializeInteger(int32_t value, int32_t min, int32_t max) {
    assert(min < max);
    assert(value >= min);
    assert(value <= max);
    const int bits = bits_required(min, max);
    uint32_t unsignedValue = value - min;
    writer.WriteBits32(unsignedValue, bits);
}

void WriteStream::SerializeBits(uint32_t value, int bits) {
    assert(bits > 0);
    assert(bits <= 32);
    writer.WriteBits32(value, bits);
}

void WriteStream::SerializeBytes(const uint8_t* data, int bytes) {
    Align();
    writer.WriteBytes(data, bytes);
}

// TODO: need???
bool WriteStream::Check(uint32_t magic) {
    Align();
    SerializeBits(magic, 32);
    return true;
}
