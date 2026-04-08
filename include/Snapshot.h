#pragma once

#include <vector>
#include "CompressedTransform.h"

class Snapshot {
public:
    Snapshot() = default;
    
    // compress relative to a previous snapshot
    void CompressTransformsRelative(WriteStream& writeStream,
                                    const Snapshot& previousSnapshot,
                                    const std::vector<Transform>& sourceTransforms);
    
    // compress without a previous snapshot
    void CompressTransformsFull(WriteStream& writeStream,
                            const std::vector<Transform>& sourceTransforms);

    // decompress relative to another snapshot
    void DecompressTransformsRelative(ReadStream& readStream,
                                      const Snapshot& previousSnapshot);
    
    // decompress without a previous snapshot
    void DecompressTransformsFull(ReadStream& readStream);

    std::size_t GetTransformCount() const;
    
    void SetCompressedTransforms(const std::vector<CompressedTransform>& transforms) {
        compressedTransforms = transforms;
    }
    
    CompressedTransform GetCompressedTransform(std::size_t index) const {
        return compressedTransforms[index];
    }
    
    void SetCompressedTransform(const CompressedTransform& compressedTransform, std::size_t index) {
        compressedTransforms[index] = compressedTransform;
    }
    
    Transform GetTransform(std::size_t index) const {
        return transforms[index];
    }

private:
    // These are compressed relative to a previous snapshot
    std::vector<CompressedTransform> compressedTransforms;
    
    // These are obtained from decompression
    std::vector<Transform> transforms;
};
