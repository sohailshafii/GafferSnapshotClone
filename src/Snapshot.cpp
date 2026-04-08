#include "Snapshot.h"

std::size_t Snapshot::GetTransformCount() const {
    return compressedTransforms.size();
}

void Snapshot::CompressTransformsRelative(WriteStream& writeStream,
                                          const Snapshot& previousSnapshot,
                                          const std::vector<Transform>& sourceTransforms) {
    auto numTransforms = sourceTransforms.size();
    if (compressedTransforms.size() != numTransforms) {
        compressedTransforms.resize(numTransforms);
    }
    // Here we want to create each compressed
    // transform relative to the prior snapshots
    for (std::size_t i = 0; i < numTransforms; i++) {
        auto relativeTransform = previousSnapshot.GetCompressedTransform(i);
        compressedTransforms[i].CompressRelative(sourceTransforms[i], relativeTransform, writeStream);
    }
}

// compress without a previous snapshot
void Snapshot::CompressTransformsFull(WriteStream& writeStream,
                        const std::vector<Transform>& sourceTransforms) {
    auto numTransforms = sourceTransforms.size();
    if (compressedTransforms.size() != numTransforms) {
        compressedTransforms.resize(numTransforms);
    }
    for (std::size_t i = 0; i < numTransforms; i++) {
        compressedTransforms[i].Compress(sourceTransforms[i], writeStream);
    }
}

// decompress relative to another snapshot
void Snapshot::DecompressTransformsRelative(ReadStream& readStream,
                                            const Snapshot& previousSnapshot) {
    auto numTransforms = previousSnapshot.GetTransformCount();
    if (transforms.size() != numTransforms) {
        transforms.resize(numTransforms);
    }
    if (compressedTransforms.size() != numTransforms) {
        compressedTransforms.resize(numTransforms);
    }
    for (std::size_t i = 0; i < numTransforms; i++) {
        auto relativeTransform = previousSnapshot.GetCompressedTransform(i);
        transforms[i] = compressedTransforms[i].DecompressRelative(relativeTransform, readStream);
    }
}

// decompress without a previous snapshot
void Snapshot::DecompressTransformsFull(ReadStream& readStream) {
    auto numTransforms = compressedTransforms.size();
    if (transforms.size() != numTransforms) {
        transforms.resize(numTransforms);
    }
    for (std::size_t i = 0; i < numTransforms; i++) {
        transforms[i] = compressedTransforms[i].Decompress(readStream);
    }
}
