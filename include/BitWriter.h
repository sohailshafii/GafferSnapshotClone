#pragma once

// https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6

#include <cstdint>
#include "BitOperationsCommon.h"

class BitWriter {
public:
    BitWriter(void *data, int bytesTotal);
    
    void Reset();
    
    bool WriteBits32(uint32_t value, int numBitsToWrite);
    
    void WriteAlign();
    
    bool WriteBytes(const uint8_t* newData, int bytes);
    
    bool FlushPendingBits();
    
    // This tells us how many bits we need to write to
    // in order to be byte-aligned. We can compute this without
    // the extra % 8 EXCEPT for the case where
    // bitsWritten is already a multiple of 8, in which
    // case not having the extra % 8 would return 8 instead of 0
    // Furthermore, the extra % 8 is probably cheaper than
    // a branch that would test for the same condition.
    int GetAlignBits() const {
        return (BITS_PER_BYTE - bitsWritten % BITS_PER_BYTE) % BITS_PER_BYTE;
    }
    
    int GetBitsWritten() const {
        return bitsWritten;
    }
    
    int GetBitsAvailable() const {
        return numBitsTotal - bitsWritten;
    }
    
    const uint8_t* GetData() const {
        return (uint8_t*)data;
    }
    
    int GetBytesWrittenTotal() const {
        return wordIndex * BYTES_PER_WORD;
    }
    
    int GetTotalBytesAllocated() const {
        return numWordsTotal * BYTES_PER_WORD;
    }
    
    bool HasOverflowed() const {
        return overflow;
    }
    
private:
    uint32_t *data;
    uint64_t scratch;
    
    int numBitsTotal;
    int numWordsTotal;
    int bitsWritten;
    int bitIndex;
    int wordIndex;
    bool overflow;
};
