#include "BitReader.h"
#include <cassert>
#include <iostream>

BitReader::BitReader(const void *data, int bytesTotal)
    : data((const uint32_t*)data), numWordsTotal(bytesTotal / BYTES_PER_WORD) {
    // Make sure the incoming data is not null
    // and the number of bytes are divisible by word size
    assert(data != nullptr);
    assert((bytesTotal % BYTES_PER_WORD) == 0);
    numBitsTotal = numWordsTotal * BITS_PER_WORD;
    
    Reset();
}

void BitReader::Reset() {
    bitsRead = 0;
    scratchBitIndex = 0;
    wordIndex = 0;
    // load the first word into the lower 32 bits of scratch.
    // the top half is used for reading chunks from it.
    scratch = endianness_fix(this->data[0]);
    overflow = false;
}

uint32_t BitReader::ReadBits(int bits) {
    assert(bits > 0);
    assert(bits <= BITS_PER_WORD);
    
    int bitsAfterRead = (bitsRead + bits);
    // TODO: should we just read anyway, capped to max num bits?
    if (bitsAfterRead > numBitsTotal) {
        std::cerr << "Overflow: num bits read is "
            << bitsRead << " and you want to read an additional "
            << bits << " bits, which will put it over the limit "
            << numBitsTotal << ".\n";
        overflow = true;
        return 0;
    }
    overflow = false;
    
    bitsRead += bits;
    
    // this is used to read from scratch
    assert(scratchBitIndex < BITS_PER_WORD);
    
    // The amount of bits we are reading is within the
    // amount of bits the scratch space contains.
    if ((scratchBitIndex + bits) < BITS_PER_WORD) {
        scratch <<= bits;
        scratchBitIndex += bits;
    }
    // The amount of bits we are reading is more than
    // what the scratch space contains in its active word half, so we need to
    // deplete the current scratch space first, then read
    // the remainder.
    else {
        wordIndex++;
        assert(wordIndex < numWordsTotal);
        // how many bits from the lower half remain from
        // the current word? let's say we've put in 20 bits,
        // so 12 remain.
        const uint32_t currBitsRemaining = BITS_PER_WORD - scratchBitIndex;
        // how many more bits need to be read after the
        // remainder of the current word is read?
        const uint32_t bitsAfter = bits - currBitsRemaining;
        // shift in remainder of current word, then read next
        // word into lower half
        scratch <<= currBitsRemaining;
        scratch |= endianness_fix(data[wordIndex]);
        // read in part of following word to satisfy the
        // bit count of the current read request.
        scratch <<= bitsAfter;
        scratchBitIndex = bitsAfter;
    }
    
    // the bits being read from the consumer is
    // in the top half of the scratch space. Since we
    // need to read that into a 32-bit word, shift it down
    // by 32-bits and cast. And then clear the top half
    // by ANDing with 1s for the bottom half.
    const uint32_t output = uint32_t(scratch >> BITS_PER_WORD);
    scratch &= 0xFFFFFFFF;
    
    return output;
}

void BitReader::ReadAlign() {
    const int remainderBits = bitsRead % 8;
    if (remainderBits != 0) {
        // so if bits read was 10, the module gives you 2,
        // which means 6 bits remain till the next byte
        uint32_t value = ReadBits(BITS_PER_BYTE - remainderBits);
        assert(value == 0);
        assert(bitsRead % 8 == 0);
    }
}

void BitReader::ReadBytes(uint8_t* data, int bytes) {
    // already byte-aligned
    assert(GetAlignBits() == 0);
    
    // detect overflow case -- can't read beyond it
    // TODO: should we just do a partial read anyway?
    if (bitsRead + bytes * BITS_PER_BYTE >= numBitsTotal) {
        memset(data, 0, bytes);
        overflow = true;
        std::cerr << "Overflow: num bits read is "
            << bitsRead << " and you want to read an additional "
            << bytes << "bytes, which will be over limit "
            << numBitsTotal << ".\n";
        return;
    }
    overflow = false;
    
    // Make sure the bit index is on byte-divisible boundaries
    assert(scratchBitIndex == 0 || scratchBitIndex == 8 ||
           scratchBitIndex == 16 || scratchBitIndex == 24);
    
    // Step 1: if scratch has a partial word, read the remainder
    // bit index is divided by bits per byte to give the
    // number of bytes read so far. this is subtracted by
    // the number per word to give the amount remaining. We modulo
    // in case the index is 0 -- we need to give a 0 as a result.
    // This part of the code is to write leftover bytes before doing a bulk write operation below
    int headBytes = (BYTES_PER_WORD - scratchBitIndex / BITS_PER_BYTE) % BYTES_PER_WORD;
    // cap leftover in case it exceeds total requested
    if (headBytes > bytes) {
        headBytes = bytes;
    }
    for (int i = 0; i < headBytes; ++i) {
        data[i] = ReadBits(BITS_PER_BYTE);
    }
    // Done, return
    if (headBytes == bytes) {
        return;
    }
    
    assert(GetAlignBits() == 0);
    
    // Step 2: Calculate read as many bulk words as possible.
    int numWordsBulk = (bytes - headBytes) / BYTES_PER_WORD;
    if (numWordsBulk > 0) {
        assert(scratchBitIndex == 0);
        memcpy(data + headBytes, &this->data[wordIndex], numWordsBulk * BYTES_PER_WORD);
        bitsRead += numWordsBulk * BITS_PER_WORD;
        wordIndex += numWordsBulk;
        scratch = endianness_fix(data[wordIndex]);
    }
    
    assert(GetAlignBits() == 0);
    
    // Step 3: Do leftover reads after remaining scratch space and
    // bulk word reads have been satisfied.
    // Calculate offset into output array we need to write to
    // and the amount to write.
    int tailStart = headBytes + numWordsBulk * BYTES_PER_WORD;
    int tailBytes = bytes - tailStart;
    assert(tailBytes >= 0 && tailBytes < BYTES_PER_WORD);
    for (int i = 0; i < tailBytes; ++i) {
        data[tailStart + i] = ReadBits(BITS_PER_BYTE);
    }
    
    assert(GetAlignBits() == 0);
    assert(headBytes + numWordsBulk * BYTES_PER_WORD + tailBytes == bytes);
}
