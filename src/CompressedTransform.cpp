#include "CompressedTransform.h"

#include <algorithm>
#include <cstdlib>

namespace {
    // Each quantized unit represents ~0.00195 meters. Must be a power of two
    // so that bit-shifting can replace division when extracting whole/fractional parts.
    static const int UnitsPerMeter = 512;
    static const int UnitsPerMeterBits = 9;

    // Maximum position extent in any direction, in meters.
    static const int PositionBoundMeters = 8;

    // Total quantized steps across the full position range.
    // 512 steps/meter * 8 meters = 4096 steps.
    // Stored as a signed 12-bit integer: range [-2048, 2047].
    static const int QuantizedStepsBound = UnitsPerMeter * PositionBoundMeters;
    static const int NumBitsRequiredPos = 12;

    // Quaternion components are stored using the smallest-three scheme.
    // Each of the three non-largest components fits in [-1/sqrt(2), 1/sqrt(2)],
    // so 9 bits gives sufficient precision.
    static const int NumBitsRequiredQuat = 9;

    // Maximum absolute delta (in quantized steps) that qualifies for
    // small-delta encoding. Values outside this range fall back to full encoding.
    static const int SmallPosDelta = 15;
    // 5-bit two's complement covers [-16, 15], sufficient for [-15, 15].
    static const int SmallPosDeltaBits = 5;
}

int CompressedTransform::GetMaxQuantizationSizePerElement() {
    // Worst case is a relative snapshot with full position and full quaternion:
    // 1 pos flag + 3 pos components + 1 quat flag + largest index + 3 quat components
    return 1 + NumBitsRequiredPos * 3 +
           1 + 2 + NumBitsRequiredQuat * 3;
}

CompressedTransform::CompressedTransform()
{
    // NOTE: old code. Kept for reference.
    /*bitsPosX.fill(0);
    bitsPosY.fill(0);
    bitsPosZ.fill(0);*/
}

void CompressedTransform::Compress(const Transform& transform, WriteStream& writeStream)
{
    // we have 512 steps per meter, with 8 meters total. multiply that
    // against each component, clamp against [-2048, 2047],
    // and then store that into a signed 12-bit integer
    // Another way to quantize is to take the signed position
    // convert to a unsigned max value, then divide by the max
    // range to force it to [0 - 1]. Then multiply that by
    // the number of discretized steps to force that into a fixed
    // integer value. This works, however you lose precision
    // as the max range grows, and requires more operations.
    QuantizeFullTransformValues(transform, quantX, quantY, quantZ, largestIndex, small1, small2, small3);
    
    EncodingType encodingUsed = EncodingType::Full;
    writeStream.SerializeBits((uint32_t)encodingUsed, 1);
    
    // represent with proper number of bits
    // if numb bits is 12, then that's 12*3 = 36 bits, vs
    // 3*32 = 96. A savings of 60 bits per transform.
    writeStream.SerializeBits(quantX, NumBitsRequiredPos);
    writeStream.SerializeBits(quantY, NumBitsRequiredPos);
    writeStream.SerializeBits(quantZ, NumBitsRequiredPos);
    writeStream.SerializeBits(largestIndex, 2);
    writeStream.SerializeBits(small1, NumBitsRequiredQuat);
    writeStream.SerializeBits(small2, NumBitsRequiredQuat);
    writeStream.SerializeBits(small3, NumBitsRequiredQuat);

    // NOTE: old code
    //ConvertToBitsNaive(quantX, bitsPosX.data());
    //ConvertToBitsNaive(quantY, bitsPosY.data());
    //ConvertToBitsNaive(quantZ, bitsPosZ.data());
}

Transform CompressedTransform::Decompress(ReadStream& readStream)
{
    uint32_t encodingType;
    readStream.DeserializeBits(encodingType, 1);
    
    readStream.DeserializeBits(quantX, NumBitsRequiredPos);
    readStream.DeserializeBits(quantY, NumBitsRequiredPos);
    readStream.DeserializeBits(quantZ, NumBitsRequiredPos);

    float dequantX = DequantPosComp(quantX);
    float dequantY = DequantPosComp(quantY);
    float dequantZ = DequantPosComp(quantZ);

    readStream.DeserializeBits(largestIndex, 2);
    readStream.DeserializeBits(small1, NumBitsRequiredQuat);
    readStream.DeserializeBits(small2, NumBitsRequiredQuat);
    readStream.DeserializeBits(small3, NumBitsRequiredQuat);
    
    Transform decompTransform(Vec3f(dequantX, dequantY, dequantZ), DecompressQuaternion());
    return decompTransform;
}

void CompressedTransform::CompressRelative(
                                           const Transform& transform,
                                           const CompressedTransform& relativeTransform,
                                           WriteStream& writeStream) {
    // Quantize current transform directly into member variables,
    // so they are available as the resolved absolute integers for the next snapshot.
    QuantizeFullTransformValues(transform, quantX, quantY, quantZ,
                                largestIndex, small1, small2, small3);

    // Compute signed position deltas in quantized space.
    // Both sides must be sign-extended from 12-bit before subtracting,
    // since the stored uint32_t may have originated from either QuantizePosComp
    // (upper bits set) or DeserializeBits (upper bits zero).
    int32_t deltaX = SignExtend(quantX, NumBitsRequiredPos) - SignExtend(relativeTransform.GetQuantX(), NumBitsRequiredPos);
    int32_t deltaY = SignExtend(quantY, NumBitsRequiredPos) - SignExtend(relativeTransform.GetQuantY(), NumBitsRequiredPos);
    int32_t deltaZ = SignExtend(quantZ, NumBitsRequiredPos) - SignExtend(relativeTransform.GetQuantZ(), NumBitsRequiredPos);

    bool allSmall = std::abs(deltaX) <= SmallPosDelta &&
                    std::abs(deltaY) <= SmallPosDelta &&
                    std::abs(deltaZ) <= SmallPosDelta;
    EncodingType encodingUsed = allSmall ? EncodingType::Delta : EncodingType::Full;

    writeStream.SerializeBits((uint32_t)encodingUsed, 1);

    if (allSmall) {
        // Deltas fit in SmallPosDeltaBits, cast to uint32_t so SerializeBits
        // writes the correct two's complement bit pattern.
        writeStream.SerializeBits((uint32_t)deltaX, SmallPosDeltaBits);
        writeStream.SerializeBits((uint32_t)deltaY, SmallPosDeltaBits);
        writeStream.SerializeBits((uint32_t)deltaZ, SmallPosDeltaBits);
    } else {
        // Delta too large, fall back to full 12-bit quantized values.
        writeStream.SerializeBits(quantX, NumBitsRequiredPos);
        writeStream.SerializeBits(quantY, NumBitsRequiredPos);
        writeStream.SerializeBits(quantZ, NumBitsRequiredPos);
    }

    // Quaternion delta compression: only possible when largestIndex matches,
    // since the three stored components correspond to different axes otherwise.
    int delta1 = (int)small1 - (int)relativeTransform.small1;
    int delta2 = (int)small2 - (int)relativeTransform.small2;
    int delta3 = (int)small3 - (int)relativeTransform.small3;
    bool quatIsDelta = (largestIndex == relativeTransform.largestIndex) &&
                       std::abs(delta1) <= SmallPosDelta &&
                       std::abs(delta2) <= SmallPosDelta &&
                       std::abs(delta3) <= SmallPosDelta;

    writeStream.SerializeBits(quatIsDelta ? (uint32_t)EncodingType::Delta
                                          : (uint32_t)EncodingType::Full, 1);
    if (quatIsDelta) {
        // largestIndex is inferred from the previous snapshot on the read side.
        // Negative deltas are encoded as two's complement and recovered via SignExtend.
        writeStream.SerializeBits((uint32_t)delta1, SmallPosDeltaBits);
        writeStream.SerializeBits((uint32_t)delta2, SmallPosDeltaBits);
        writeStream.SerializeBits((uint32_t)delta3, SmallPosDeltaBits);
    } else {
        // Always write largestIndex in the full case, whether it changed or not.
        // This keeps the read side unambiguous without an extra flag.
        writeStream.SerializeBits(largestIndex, 2);
        writeStream.SerializeBits(small1, NumBitsRequiredQuat);
        writeStream.SerializeBits(small2, NumBitsRequiredQuat);
        writeStream.SerializeBits(small3, NumBitsRequiredQuat);
    }
}

Transform CompressedTransform::DecompressRelative(
                                                  const CompressedTransform& relativeTransform,
                                                  ReadStream& readStream) {
    uint32_t encodingType;
    readStream.DeserializeBits(encodingType, 1);
    bool allSmall = (EncodingType)encodingType == EncodingType::Delta;

    if (allSmall) {
        // Read 5-bit signed deltas and reconstruct absolute quantized values
        // by adding them to the previous snapshot's resolved integers.
        uint32_t rawDeltaX, rawDeltaY, rawDeltaZ;
        readStream.DeserializeBits(rawDeltaX, SmallPosDeltaBits);
        readStream.DeserializeBits(rawDeltaY, SmallPosDeltaBits);
        readStream.DeserializeBits(rawDeltaZ, SmallPosDeltaBits);

        quantX = (uint32_t)(SignExtend(relativeTransform.GetQuantX(), NumBitsRequiredPos) + SignExtend(rawDeltaX, SmallPosDeltaBits));
        quantY = (uint32_t)(SignExtend(relativeTransform.GetQuantY(), NumBitsRequiredPos) + SignExtend(rawDeltaY, SmallPosDeltaBits));
        quantZ = (uint32_t)(SignExtend(relativeTransform.GetQuantZ(), NumBitsRequiredPos) + SignExtend(rawDeltaZ, SmallPosDeltaBits));
    } else {
        // Delta was too large — read full 12-bit quantized values directly.
        readStream.DeserializeBits(quantX, NumBitsRequiredPos);
        readStream.DeserializeBits(quantY, NumBitsRequiredPos);
        readStream.DeserializeBits(quantZ, NumBitsRequiredPos);
    }

    float posX = DequantPosComp(quantX);
    float posY = DequantPosComp(quantY);
    float posZ = DequantPosComp(quantZ);

    uint32_t quatEncodingType;
    readStream.DeserializeBits(quatEncodingType, 1);
    if ((EncodingType)quatEncodingType == EncodingType::Delta) {
        // largestIndex unchanged — inherit from previous snapshot.
        largestIndex = relativeTransform.GetLargestIndex();
        uint32_t rawDelta1, rawDelta2, rawDelta3;
        readStream.DeserializeBits(rawDelta1, SmallPosDeltaBits);
        readStream.DeserializeBits(rawDelta2, SmallPosDeltaBits);
        readStream.DeserializeBits(rawDelta3, SmallPosDeltaBits);
        small1 = (uint32_t)((int)relativeTransform.GetSmall1() + SignExtend(rawDelta1, SmallPosDeltaBits));
        small2 = (uint32_t)((int)relativeTransform.GetSmall2() + SignExtend(rawDelta2, SmallPosDeltaBits));
        small3 = (uint32_t)((int)relativeTransform.GetSmall3() + SignExtend(rawDelta3, SmallPosDeltaBits));
    } else {
        readStream.DeserializeBits(largestIndex, 2);
        readStream.DeserializeBits(small1, NumBitsRequiredQuat);
        readStream.DeserializeBits(small2, NumBitsRequiredQuat);
        readStream.DeserializeBits(small3, NumBitsRequiredQuat);
    }

    return Transform(Vec3f(posX, posY, posZ), DecompressQuaternion());
}

uint32_t CompressedTransform::QuantizePosComp(float comp) const
{
    int posValInSteps = int(comp * UnitsPerMeter);
    // Clamp value in the proper signed range.
    posValInSteps = std::clamp(posValInSteps,
                               -(QuantizedStepsBound >> 1),
                               (QuantizedStepsBound >> 1) - 1);
    return (uint32_t)posValInSteps;
}

float CompressedTransform::DequantPosComp(uint32_t quantVal) const
{
    // The bitstream stores only 12 bits per component, so negative values arrive
    // with bits 12-31 all zero. For example, -1.0m quantizes to -512 steps.
    // QuantizePosComp casts that to uint32_t giving 0xFFFFFE00, but WriteBits32
    // masks it to 12 bits before storing, leaving 0xE00 in the buffer.
    // When read back into a uint32_t, quantVal = 0x00000E00 -- which looks like
    // +3584 and would decode to +7.0m instead of -1.0m.
    //
    // Sign-extending restores the correct 32-bit two's complement value:
    //   Check the 12-bit sign bit:  (1u << 11)        = 0x00000800
    //   0x00000E00 & 0x00000800 != 0, so the value is negative.
    //
    //   Build the upper-bit mask:   (1u << 12) - 1    = 0x00000FFF  (low 12 bits)
    //                               ~0x00000FFF        = 0xFFFFF000  (bits 12-31)
    //   OR into quantVal:           0x00000E00 | 0xFFFFF000 = 0xFFFFFE00
    //   Cast to int32_t:            0xFFFFFE00         = -512  (correct)
    if (quantVal & (1u << (NumBitsRequiredPos - 1))) {
        quantVal |= ~((1u << NumBitsRequiredPos) - 1);
    }
    // Cast to int32_t so that the right-shift below is arithmetic (sign-fills),
    // not logical (zero-fills). Without this, 0xFFFFFE00 (-512 after sign extension)
    // would shift as unsigned: 0xFFFFFE00 >> 9 = 0x007FFFFF = 8,388,607 instead of -1.
    int signedVal = static_cast<int32_t>(quantVal);
    // Arithmetic right-shift extracts the whole-meter portion.
    // e.g. -512 >> 9 = -1  (floor toward -inf, correct for two's complement)
    int wholePortion = signedVal >> UnitsPerMeterBits;
    // get the fractional portion, which is 0-511, via masking
    // 0-511 inclusive represents 512 steps.
    int fractionalPortion = signedVal & (UnitsPerMeter - 1);
    // the fractional portion represents steps, need to
    // conver that to fraction of a meter
    float fractionalFloat = fractionalPortion / (float)UnitsPerMeter;

    return wholePortion + fractionalFloat;
}

float CompressedTransform::DequantQuatComp(uint32_t quantVal) const {
    // The value here needs to be brought back to normalized values first...
    const float scale = (float)((1 << NumBitsRequiredQuat) - 1);
    float dequant = quantVal/scale;
    // then extended to the range we care about
    const float minVal = -1.0f/1.414214f; // 1.0f / sqrt(2)
    const float maxVal = 1.0f/1.414214f;
    //smallVals[i] = (smallVals[i] - minVal) / (maxVal - minVal);
    dequant *= (maxVal - minVal);
    // biased back to min
    dequant += minVal;
    
    return dequant;
}

/*void CompressedTransform::ConvertToBitsNaive(int quantizedValue, unsigned char* bits)
{
    // Extract each bit using a shifting mask, working directly with the signed value.
    // This avoids any conversion and handles two's complement representation naturally.
    for (int bitIndex = 0; bitIndex < NumBitsRequired; bitIndex++) {
        // Extract bit at position bitIndex by shifting and masking
        bits[bitIndex] = (quantizedValue >> bitIndex) & 0x1;
    }
}*/

/*int CompressedTransform::GetIntFromBits(const unsigned char* bits) const
{
    // Reconstruct the signed integer directly from bits.
    // Start with 0 and OR in each bit at its position.
    int result = 0;
    for (int i = 0; i < NumBitsRequired; i++) {
        if (bits[i]) {
            result |= (1 << i);
        }
    }

    // Sign extend if the sign bit (bit 11) is set.
    // At this point, result contains bits 0-11 from the stored value.
    // If bit 11 is set, the value is negative in 12-bit two's complement.
    // Since we're storing in a 32-bit int, we need to extend the sign
    // by setting bits 12-31 to 1 as well (matching bit 11).
    if (result & (1 << (NumBitsRequired - 1))) {
        // Create a mask with 1s in bits 12-31: ~((1 << 12) - 1) = ~0x0FFF = 0xFFFFF000
        // OR this with result to set all upper bits to 1
        result |= ~((1 << NumBitsRequired) - 1);
    }

    return result;
}
*/

Quaternion CompressedTransform::DecompressQuaternion() const {
    float q1 = DequantQuatComp(small1);
    float q2 = DequantQuatComp(small2);
    float q3 = DequantQuatComp(small3);

    float quatX, quatY, quatZ, quatW;
    if (largestIndex == 0) {
        quatY = q1; quatZ = q2; quatW = q3;
        quatX = sqrt(std::max(0.0f, 1.0f - quatY*quatY - quatZ*quatZ - quatW*quatW));
    } else if (largestIndex == 1) {
        quatX = q1; quatZ = q2; quatW = q3;
        quatY = sqrt(std::max(0.0f, 1.0f - quatX*quatX - quatZ*quatZ - quatW*quatW));
    } else if (largestIndex == 2) {
        quatX = q1; quatY = q2; quatW = q3;
        quatZ = sqrt(std::max(0.0f, 1.0f - quatX*quatX - quatY*quatY - quatW*quatW));
    } else {
        quatX = q1; quatY = q2; quatZ = q3;
        quatW = sqrt(std::max(0.0f, 1.0f - quatX*quatX - quatY*quatY - quatZ*quatZ));
    }

    return Quaternion(quatX, quatY, quatZ, quatW);
}

void CompressedTransform::QuantizeQuaternion(const Quaternion& quaternion,
                        uint32_t& largestIndex, uint32_t& small1,
                        uint32_t& small2, uint32_t& small3) {
    // quaternion
    // 1. pick smallest three (or everything besides largest one).
    // encode component with 2 bits (0=x, 1=y, 2=z, 3=w)
    // 2. If component is negative, negate whole quat to force it to be
    // negative, because a (x, y, z, w) represents the same rotation
    // as (-x,-y,-z,-w)
    // 3. If v is the largest, then the next largest possible occurs
    // when we have one component equal to v, and the rest are zero.
    // since this equals 1, then v^2 + v^2 = 1, v=1/sqrt(2).
    // This means v can be between [-0.707107,0.707107].
    // Less range to worry about, giving you more precision per bit.
    // So each component will not be that large.
    // Total bits -- 2 + 9 * 3 = 29 bits! Down from 3*32 = 128 bits,
    // a savings of 99 bits per transform.
    uint32_t biggestIndex = 0;
    float biggestValue = std::abs(quaternion[0]);
    for (int i = 1; i <= 3; i++) {
        if (std::abs(quaternion[i]) > biggestValue) {
            biggestIndex = i;
            biggestValue = std::abs(quaternion[i]);
        }
    }
    
    // cache the other values
    // negate if needed
    float smallVals[3];
    bool shouldNegate = quaternion[biggestIndex] < 0.0f;
    int smallIndex = 0;
    for (int i = 0; i < 4; i++) {
        if (i == biggestIndex) {
            continue;
        }
        smallVals[smallIndex++] = shouldNegate ? -quaternion[i] : quaternion[i];
    }
    
    const float minVal = -1.0f/1.414214f; // 1.0f / sqrt(2)
    const float maxVal = 1.0f/1.414214f;
    // if we have 3 bits of valid range, the bits in that range
    // would be 0-2. So shift by 3 to get 1000, then
    // subtract by 1 to get 0111. That gets you three bits of coverage,
    // or 7 discrete steps.
    const float scale = (float)((1 << NumBitsRequiredQuat) - 1);
    
    // normalize value. 0-1.0
    for (int i = 0; i < 3; i++) {
        smallVals[i] = (smallVals[i] - minVal) / (maxVal - minVal);
    }
    
    // convert to int. so if we have range 0-111, with 3 bits here
    // we can use round directly here, it might be faster on
    // modern hardware, but floor(val + 0.5) seems idiomatic
    // before a standadized round was available.
    // Note that round also rounds half away from zero, while
    // floor(val + 0.5) goes toward +infinity. But this doesn't
    // matter for positive values.
    // Why do we do this? We discretize a continuous range
    // to a discrete one. Before doing a floor operation for 7 steps,
    // we can have say a value between 1 and 2.0. A floor will
    // force all values right before 2 (like a 1.9) to be pulled to
    // the left side or 1, causing a max error of 1.0 unit. In reality
    // we should have 1.5-1.9 go up to 2, and 1-1.49 go down to 1.0,
    // based on the proximity of each float value with its corresponding discrete one.
    // Doing a 0.5 then a floor forces the larger values to
    // result in "2" after a floor, halving the error by a half.
    largestIndex = biggestIndex;
    small1 = floor(smallVals[0] * scale + 0.5f);
    small2 = floor(smallVals[1] * scale + 0.5f);
    small3 = floor(smallVals[2] * scale + 0.5f);
}

void CompressedTransform::QuantizeFullTransformValues(const Transform& transform, uint32_t& quantX, uint32_t& quantY, uint32_t& quantZ, uint32_t &largestIndex, uint32_t &small1, uint32_t &small2, uint32_t &small3) {
    auto position = transform.GetPosition();
    quantX = QuantizePosComp(position[0]);
    quantY = QuantizePosComp(position[1]);
    quantZ = QuantizePosComp(position[2]);
    
    auto rotation = transform.GetRotation();
    QuantizeQuaternion(rotation, largestIndex, small1,
                       small2, small3);
}
