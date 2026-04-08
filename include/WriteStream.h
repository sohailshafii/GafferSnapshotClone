#pragma once

// https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6

#include <cstdint>
#include <cassert>
#include "BitOperationsCommon.h"
#include "BitWriter.h"

class WriteStream {
public:
    WriteStream(void* data, int bytesTotal);
    void Reset();
    
    void SerializeInteger(int32_t value, int32_t min, int32_t max);
    void SerializeBits(uint32_t value, int bits);
    void SerializeBytes(const uint8_t* data, int bytes);
    
    void Align() {
        writer.WriteAlign();
    }
    
    int GetAlignBits() const {
        return writer.GetAlignBits();
    }
    
    bool Check(uint32_t magic);
    void Flush() {
        writer.FlushPendingBits();
    }
    
    const uint8_t* GetData() const {
        return writer.GetData();
    }

    int GetBytesProcessed() const {
        return writer.GetBytesWrittenTotal();
    }
    
    int GetBitsProcessed() const {
        return writer.GetBitsWritten();
    }
    
    int GetBitsRemaining() const {
        return GetTotalBits() - GetBitsProcessed();
    }
    
    int GetTotalBits() const {
        return writer.GetTotalBytesAllocated() * BITS_PER_BYTE;
    }
    
    int GetTotalBytes() const {
        return writer.GetTotalBytesAllocated();
    }
    
    bool IsOverFlow() const {
        return writer.HasOverflowed();
    }
    
private:
    BitWriter writer;
};
