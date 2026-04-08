#pragma once

#include <array>
#include "Transform.h"
#include "ReadStream.h"
#include "WriteStream.h"

class CompressedTransform {
public:
    CompressedTransform();

    void Compress(const Transform& transform, WriteStream& writeStream);
    Transform Decompress(ReadStream& readStream);
    
    void CompressRelative(const Transform& transform, const CompressedTransform& relativeTransform, WriteStream& writeStream);
    Transform DecompressRelative(const CompressedTransform& relativeTransform, ReadStream& readStream);
    
    static int GetMaxQuantizationSizePerElement();

    uint32_t GetQuantX() const {
        return quantX;
    }
    
    uint32_t GetQuantY() const {
        return quantY;
    }
    
    uint32_t GetQuantZ() const {
       return quantZ;
    }
    
    uint32_t GetLargestIndex() const {
        return largestIndex;
    }
    
    uint32_t GetSmall1() const {
        return small1;
    }
    
    uint32_t GetSmall2() const {
        return small2;
    }
    
    uint32_t GetSmall3() const {
        return small3;
    }

private:
    enum class EncodingType : char {
        Delta = 0,
        Full = 1
    };
    
    // old bitstream code
    /*std::array<unsigned char, NumBitsRequired> bitsPosX;
    std::array<unsigned char, NumBitsRequired> bitsPosY;
    std::array<unsigned char, NumBitsRequired> bitsPosZ;*/

    // Sign-extends an N-bit value stored in a uint32_t to a full int32_t.
    static int32_t SignExtend(uint32_t v, int numBits) {
        if (v & (1u << (numBits - 1)))
            v |= ~((1u << numBits) - 1);
        return static_cast<int32_t>(v);
    }

    uint32_t QuantizePosComp(float comp) const;
    float DequantPosComp(uint32_t quantVal) const;
    float DequantQuatComp(uint32_t quantVal) const;
    //void ConvertToBitsNaive(int quantizedValue, unsigned char* bits);
    //int GetIntFromBits(const unsigned char* bits) const;
    
    void QuantizeQuaternion(const Quaternion& quaternion,
                            uint32_t& largestIndex, uint32_t& small1,
                            uint32_t& small2, uint32_t& small3);
    
    void QuantizeFullTransformValues(const Transform& transform, uint32_t& quantX, uint32_t& quantY, uint32_t& quantZ, uint32_t &largestIndex, uint32_t &small1, uint32_t &small2, uint32_t &small3);

    // Reconstructs a Quaternion from the already-populated largestIndex and small1/2/3
    // member variables. Called after deserializing the quaternion bits.
    Quaternion DecompressQuaternion() const;
    
    uint32_t quantX;
    uint32_t quantY;
    uint32_t quantZ;
    uint32_t largestIndex;
    uint32_t small1, small2, small3;
};
