#include "BitWriter.h"
#include <cassert>
#include <cstring>
#include <iostream>

// https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6

BitWriter::BitWriter(void *data, int bytesTotal)
    : data((uint32_t*)data), numWordsTotal(bytesTotal / BYTES_PER_WORD)
{
    assert(this->data != nullptr);
    assert(bytesTotal % BYTES_PER_WORD == 0);
    numBitsTotal = numWordsTotal * BITS_PER_WORD;
    Reset();
}

void BitWriter::Reset() {
    bitsWritten = 0;
    scratch = 0;
    bitIndex = 0;
    wordIndex = 0;
    overflow = false;
    memset(data, 0, numWordsTotal * BYTES_PER_WORD);
}

bool BitWriter::WriteBits32(uint32_t value, int numBitsToWrite) {
    if (bitsWritten == numBitsTotal) {
        std::cerr << "Cannot write anymore, already written "
            << bitsWritten << " total.\n";
        return false;
    }

    assert(numBitsToWrite > 0);
    assert(numBitsToWrite <= BITS_PER_WORD);
    
    int numBitsAfterWrite = bitsWritten + numBitsToWrite;
    if (numBitsAfterWrite > numBitsTotal) {
        std::cerr << "Overflow: num written is "
            << bitsWritten << " and want to write an additional "
            << numBitsToWrite << ", resulting in "
            << numBitsAfterWrite << " which is over limit "
            << numBitsTotal << ".\n";
        overflow = true;
        return;
    }
    overflow = false;
    
    // This is masking the number of bits we want to write.
    // Should do this if value has other bits set.
    // Let's say we want to mask 4 bits.That would be 0...1111
    // So we shift 1 four times to 0...10000 to position 5.
    // Then subtract by 1 to get 0...01111 and mask against
    // that to make sure only the bottom 4 bits pass. Use
    // a 64-bit value in case want to write all 32-bits,
    // because we need to move the 1 initially to bit pos 33.
    value &= (uint64_t(1) << numBitsToWrite) - 1;
    
    // This seems to write from the most significant bit
    // to the least.
    scratch |= uint64_t(value) << (64 - bitIndex - numBitsToWrite);
    
    bitIndex += numBitsToWrite;
    
    // once we hit word boundary, write the word.
    // If we had used a 32-bit scratch space, then we would have
    // run into a problems if bitIndex was at 31, and we wanted
    // to write another 32 bits. This would require ORing
    // the remaining 1-bit, THEN writing the new bits into the
    // scratch value. Here, we write to the scratch, and just
    // flush the word that we have filled up.
    if (bitIndex >= BITS_PER_WORD) {
        assert(wordIndex < numWordsTotal);
        // Write the top 32 bits by shifting it down
        // to fit a 32-bit word.
        uint32_t scratchTo32BitWord = uint32_t(scratch >> BITS_PER_WORD);
        data[wordIndex] = endianness_fix(scratchTo32BitWord);
        // shift the word that we have written out. Note that
        // if our current write crossed the 32-bit boundary,
        // we won't lose that.
        scratch <<= BITS_PER_WORD;
        // move back write to indicate the amount we flushed out.
        // If we have written across 32-bits, like 40 bits
        // we flush 32-bits and should have bit index at 8
        // from the top.
        bitIndex -= BITS_PER_WORD;
        wordIndex++;
    }
    
    bitsWritten += numBitsToWrite;
    
    return true;
}

void BitWriter::WriteAlign() {
    const int remainderBits = bitsWritten % BITS_PER_BYTE;
    // if we are not byte-aligned...so 18 would give us "2"
    if (remainderBits != 0) {
        uint32_t zero = 0;
        // if remainder bits was 2, write 6 bits using zero
        // to fill a byte.
        // so it would go from 18->24. Can't delete bits here to be
        // aligned.
        WriteBits32(zero, BITS_PER_BYTE - remainderBits);
        assert(bitsWritten % BITS_PER_BYTE == 0);
    }
}

bool BitWriter::WriteBytes(const uint8_t* newData, int bytes) {
    assert(GetAlignBits() == 0);
    
    if (bitsWritten == numBitsTotal) {
        std::cerr << "Cannot write anymore, already written "
            << bitsWritten << " total.\n";
        return false;
    }
    
    if (bitsWritten + bytes * BITS_PER_BYTE >= numBitsTotal) {
        std::cout << "Byte overflow!\n";
        overflow = true;
        return false;
    }
    
    overflow = false;
    
    // Make sure we are byte-aligned by this point.
    assert(bitIndex == 0 || bitIndex == 8 || bitIndex == 16 || bitIndex == 24);
    
    // this will go from 0 - 3 based on the number of bytes
    // per word. So if the bit index is 0, that should give you 0
    // so % with bytes_per_word (i.e. the extra % is necessary).
    // If the index is 8, need 3.
    // Here we write the remaining word in the scratch space
    // and below we write whole words.
    int headBytes = (BYTES_PER_WORD - bitIndex / BITS_PER_BYTE) % BYTES_PER_WORD;
    // If there are more head bytes than bytes, cap to bytes
    // otherwise, write header first, then deal with
    // remainder later
    if (headBytes > bytes) {
        headBytes = bytes;
    }
    for (int i = 0; i < headBytes; ++i) {
        WriteBits32(newData[i], BITS_PER_BYTE);
    }
    if (headBytes == bytes) {
        return;
    }
    
    assert(GetAlignBits() == 0);
    
    // Now we can just write whole words. Write as many
    // whole words as possible.
    int numWholeWordsToWrite = (bytes - headBytes) / BYTES_PER_WORD;
    if (numWholeWordsToWrite > 0) {
        assert(bitIndex == 0);
        memcpy(&data[wordIndex], newData + headBytes,
               numWholeWordsToWrite * BYTES_PER_WORD);
        bitsWritten += numWholeWordsToWrite * BITS_PER_WORD;
        wordIndex += numWholeWordsToWrite;
        scratch = 0;
    }
    
    assert(GetAlignBits() == 0);
    
    // write after the whole portion
    int tailStartBytes = headBytes + numWholeWordsToWrite * BYTES_PER_WORD;
    int tailBytes = bytes - tailStartBytes;
    assert(tailBytes >= 0 && tailBytes < BYTES_PER_WORD);
    for (int i = 0; i < tailBytes; ++i) {
        WriteBits32(newData[tailStartBytes + i], BITS_PER_BYTE);
    }
    
    assert(GetAlignBits() == 0);
    
    assert(headBytes + numWholeWordsToWrite + BYTES_PER_WORD * tailBytes == bytes);
    
    return true;
}

bool BitWriter::FlushPendingBits() {
    if (bitIndex != 0) {
        if (wordIndex >= numWordsTotal) {
            overflow = true;
            return false;
        }
        data[wordIndex++] = endianness_fix(uint32_t(scratch >> BITS_PER_WORD));
    }
    
    return true;
}
