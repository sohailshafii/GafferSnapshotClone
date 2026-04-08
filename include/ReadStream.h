#pragma once

// https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6

#include <cstdint>
#include "BitOperationsCommon.h"
#include "BitReader.h"

class ReadStream {
public:
    ReadStream(const void* data, int bytesTotal);
    void Reset();
    
    void DeserializeInteger(int32_t& value, int32_t min, int32_t max);
    void DeserializeBits(uint32_t& value, int bits);
    void DeserializeBytes(uint8_t* data, int bytes);
    
    void Align() {
        reader.ReadAlign();
    }
    
    bool GetAlignBits() const {
        return reader.GetAlignBits();
    }
    bool Check(uint32_t magic);
    
    int GetBitsProcessed() const {
        return bitsRead;
    }
    
    int GetBytesProcessed() const {
        return bitsRead / BITS_PER_BYTE + (bitsRead % BITS_PER_BYTE ? 1 : 0);
    }
    
    bool IsOverflow() const {
        return reader.IsOverFlow();
    }
    
    int GetBytesRead() const {
        return reader.GetBytesRead();
    }

private:
    BitReader reader;
    int bitsRead;
};
