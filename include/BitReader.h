#pragma once

// https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6

#include <cstdint>
#include "BitOperationsCommon.h"

class BitReader {
public:
    BitReader(const void *data, int bytesTotal);
    void Reset();
    uint32_t ReadBits(int bits);
    
    void ReadAlign();
    void ReadBytes(uint8_t* data, int bytes);
    
    int GetAlignBits() const {
        return (BITS_PER_BYTE - bitsRead % BITS_PER_BYTE) % BITS_PER_BYTE;
    }

    int GetBitsRead() const {
        return bitsRead;
    }

    int GetBytesRead() const {
        // TODO: why the +1 here?
        return (wordIndex + 1) * BYTES_PER_WORD;
    }
    
    int GetBitsRemaining() const {
        return numBitsTotal - bitsRead;
    }

    int GetTotalBits() const {
        return numBitsTotal;
    }

    int GetTotalBytes() const {
        return numBitsTotal * BITS_PER_BYTE;
    }

    bool IsOverFlow() const {
        return overflow;
    }

private:
    const uint32_t* data;
    uint64_t scratch;

    int numBitsTotal;
    int numWordsTotal;
    int bitsRead;
    int scratchBitIndex;
    int wordIndex;
    bool overflow;
};
