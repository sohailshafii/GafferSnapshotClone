#include <gtest/gtest.h>
#include "BitWriter.h"

static const int kBufferSize = 64; // bytes

TEST(BitWriterTest, InitialState)
{
    uint8_t buffer[kBufferSize] = {};
    BitWriter writer(buffer, kBufferSize);
    EXPECT_EQ(writer.GetBitsWritten(), 0);
    EXPECT_EQ(writer.GetBitsAvailable(), kBufferSize * BITS_PER_BYTE);
    EXPECT_FALSE(writer.HasOverflowed());
}

TEST(BitWriterTest, WriteSingleBit)
{
    uint8_t buffer[kBufferSize] = {};
    BitWriter writer(buffer, kBufferSize);
    EXPECT_TRUE(writer.WriteBits32(1, 1));
    EXPECT_EQ(writer.GetBitsWritten(), 1);
    EXPECT_FALSE(writer.HasOverflowed());
}

TEST(BitWriterTest, WriteByte)
{
    uint8_t buffer[kBufferSize] = {};
    BitWriter writer(buffer, kBufferSize);
    EXPECT_TRUE(writer.WriteBits32(0xAB, 8));
    EXPECT_EQ(writer.GetBitsWritten(), 8);
    EXPECT_FALSE(writer.HasOverflowed());
}

TEST(BitWriterTest, WriteFullWord)
{
    uint8_t buffer[kBufferSize] = {};
    BitWriter writer(buffer, kBufferSize);
    EXPECT_TRUE(writer.WriteBits32(0xDEADBEEF, 32));
    EXPECT_EQ(writer.GetBitsWritten(), 32);
    EXPECT_FALSE(writer.HasOverflowed());
}

TEST(BitWriterTest, OverflowDetection)
{
    uint8_t buffer[4] = {}; // only 32 bits
    BitWriter writer(buffer, 4);
    EXPECT_TRUE(writer.WriteBits32(0xFFFFFFFF, 32));
    EXPECT_FALSE(writer.WriteBits32(1, 1));
    EXPECT_TRUE(writer.HasOverflowed());
}

TEST(BitWriterTest, WriteAlignNoopWhenAligned)
{
    uint8_t buffer[kBufferSize] = {};
    BitWriter writer(buffer, kBufferSize);
    writer.WriteAlign();
    EXPECT_EQ(writer.GetBitsWritten(), 0);
}

TEST(BitWriterTest, WriteAlignPadsToByteBoundary)
{
    uint8_t buffer[kBufferSize] = {};
    BitWriter writer(buffer, kBufferSize);
    writer.WriteBits32(1, 3); // 3 bits — not byte-aligned
    writer.WriteAlign();
    EXPECT_EQ(writer.GetBitsWritten(), 8);
}

TEST(BitWriterTest, FlushPendingBits)
{
    uint8_t buffer[kBufferSize] = {};
    BitWriter writer(buffer, kBufferSize);
    writer.WriteBits32(0xF, 4);
    EXPECT_TRUE(writer.FlushPendingBits());
    EXPECT_FALSE(writer.HasOverflowed());
}
