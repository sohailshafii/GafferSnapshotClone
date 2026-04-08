// Snapshot-based compression, based on the Gaffer On Games article:
// https://gafferongames.com/post/snapshot_compression/
// The original reference code: https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6

#include <iostream>
#include <vector>
#include <random>

#include "Vec3f.h"
#include "Quaternion.h"
#include "Transform.h"
#include "CompressedTransform.h"
#include "Snapshot.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "ReadStream.h"
#include "WriteStream.h"

// Maximum position extent in any direction, in meters. Must match the value
// used inside CompressedTransform so generated positions stay within the
// quantizable range.
static const int PositionBoundMeters = 8;

bool ParseArguments(int argc, char* argv[], int& numTransforms, int& numIterations);

void GenerateCurrentFrameValues(
                                const std::vector<Transform>& lastFrameValues,
                                std::vector<Transform>& currentValues);

void ComputeErrorStatistics(const std::vector<Transform>& original,
                           const Snapshot& latestSnapshot,
                           double& sumSquaredError,
                           float& maxError,
                           float& minError,
                           double& sumSquaredErrorQuat,
                           float& maxErrorQuat,
                           float& minErrorQuat,
                           int& sampleCount);

int main(int argc, char* argv[])
{
    std::cout << "GafferSnapshotCompression" << std::endl;
    
    int numTransforms = 100;
    int numIterations = 50;
    ParseArguments(argc, argv, numTransforms, numIterations);
    
    std::vector<Transform> lastFrameTranforms(numTransforms,
                                                Transform(Vec3f(0.0f, 0.0f, 0.0f),
                                                Quaternion()));
    std::vector<Transform> newFrameOriginalTransforms(numTransforms,
                                                      Transform(Vec3f(0.0f, 0.0f, 0.0f),
                                                      Quaternion()));
    std::vector<Transform> reconstructedTransforms(numTransforms,
                                                   Transform(Vec3f(0.0f, 0.0f, 0.0f),
                                                   Quaternion()));
    
    // have snapshots of compressed transforms that get added to.
    int baseLineIndex = -1;
    const int numMaxSnapshots = 255;
    std::vector<Snapshot> snapshots;
    snapshots.resize(numMaxSnapshots);
    std::vector<CompressedTransform> compressedTransforms(numTransforms, CompressedTransform());
    
    // Error tracking variables
    double sumSquaredError = 0.0;
    float maxError = 0.0f;
    float minError = std::numeric_limits<float>::max();
    double sumSquaredErrorQuat = 0.0;
    float maxErrorQuat = 0.0f;
    float minErrorQuat = std::numeric_limits<float>::max();
    int sampleCount = 0;
    
    std::cout << "Running " << numIterations << " iterations on "
        << numTransforms << " transforms.\n";
    
    std::vector<unsigned char> bytesArray;
    int numBitsTotalRequired = CompressedTransform::GetMaxQuantizationSizePerElement();
    numBitsTotalRequired *= numTransforms;
    int numWordsTotal = numBitsTotalRequired/32 + (numBitsTotalRequired % 32 == 0 ? 0 : 1);
    int numBytesTotal = numWordsTotal * 4;
    bytesArray.resize(numBytesTotal);
    
    ReadStream readStream(bytesArray.data(), numBytesTotal);
    WriteStream writeStream(bytesArray.data(), numBytesTotal);
    
    int64_t totalCompressedBits = 0;

    int currentSnapshotIndex = 0;
    const int numSnapshotsToFull = 10;
    int lastFullIndex = 0;
    for (int i = 0; i < numIterations; i++) {
        std::cout << "Iteration " << i << "...\n";
        writeStream.Reset();
        // Per iteration:
        // 1. Generate a new set of transforms based the
        // last frame's values.
        // 2. Compress.
        // 3. Decompress and reconstruct.
        // 4. Compare the reconstructed values against the originals.

        // generate new transforms so that we can compress them
        GenerateCurrentFrameValues(lastFrameTranforms, newFrameOriginalTransforms);

        
        bool needsNewFullSnapshot =
            currentSnapshotIndex == 0 ||
        (currentSnapshotIndex == (lastFullIndex + numSnapshotsToFull));
        std::cout << "Snapshot " << currentSnapshotIndex << std::endl;
        if (needsNewFullSnapshot) {
            snapshots[currentSnapshotIndex].CompressTransformsFull(writeStream, newFrameOriginalTransforms);
            lastFullIndex = currentSnapshotIndex;
        }
        else {
            const auto& relativeSnapshot = snapshots[currentSnapshotIndex-1];
            snapshots[currentSnapshotIndex].CompressTransformsRelative(writeStream, relativeSnapshot, newFrameOriginalTransforms);
        }

        // Track compressed size before flushing resets the count next iteration.
        totalCompressedBits += writeStream.GetBitsProcessed();

        // Flush any partial word still sitting in the BitWriter scratch space.
        // The total bit count (36 bits/transform * N transforms) is rarely a
        // multiple of 32, so the last word is not committed to the buffer until
        // FlushPendingBits() is called. Reading before flushing would pull zeros
        // for those trailing bits, corrupting the last transform(s).
        writeStream.Flush();

        // Reset the reader AFTER writing is complete. BitReader::Reset() eagerly
        // pre-loads data[0] into its scratch register. If Reset() is called before
        // the write, scratch captures stale (or zeroed) data from the previous
        // iteration, and those first 32 bits are decoded incorrectly every frame.
        readStream.Reset();
        
        if (needsNewFullSnapshot) {
            snapshots[currentSnapshotIndex].DecompressTransformsFull(readStream);
        }
        else {
            const auto& relativeSnapshot = snapshots[currentSnapshotIndex-1];
            snapshots[currentSnapshotIndex].DecompressTransformsRelative(readStream, relativeSnapshot);
        }

        // Compare and compute error statistics
        ComputeErrorStatistics(newFrameOriginalTransforms, snapshots[currentSnapshotIndex],
                             sumSquaredError, maxError, minError,
                             sumSquaredErrorQuat, maxErrorQuat, minErrorQuat, sampleCount);
        
        // update last transforms for the next iterations
        lastFrameTranforms = newFrameOriginalTransforms;
        
        currentSnapshotIndex = (currentSnapshotIndex + 1) % numMaxSnapshots;
    }
    
    std::cout << "Done\n";
    
    // Print error statistics
    double rmse = std::sqrt(sumSquaredError / sampleCount);
    double rmseQuat = std::sqrt(sumSquaredErrorQuat / sampleCount);
    std::cout << "\n=== Error Statistics ===\n";
    std::cout << "Total samples: " << sampleCount << "\n";
    std::cout << "Position RMSE: " << rmse << " meters\n";
    std::cout << "Position Max Error: " << maxError << " meters\n";
    std::cout << "Position Min Error: " << minError << " meters\n";
    std::cout << "Quaternion RMSE: " << rmseQuat << " radians\n";
    std::cout << "Quaternion Max Error: " << maxErrorQuat << " radians\n";
    std::cout << "Quaternion Min Error: " << minErrorQuat << " radians\n";

    // 3 position floats + 4 quaternion floats, 32 bits each
    const int bitsPerTransformUncompressed = 7 * 32;
    int64_t totalUncompressedBits = (int64_t)bitsPerTransformUncompressed * numTransforms * numIterations;
    double compressionRatio = (double)totalCompressedBits / (double)totalUncompressedBits;

    std::cout << "\n=== Bandwidth ===\n";
    std::cout << "Uncompressed : " << totalUncompressedBits / 8 << " bytes ("
              << totalUncompressedBits << " bits)\n";
    std::cout << "Compressed   : " << totalCompressedBits / 8 << " bytes ("
              << totalCompressedBits << " bits)\n";
    std::cout << "Ratio        : " << compressionRatio * 100.0 << "% of original size\n";
    std::cout << "Space saved  : " << (1.0 - compressionRatio) * 100.0 << "%\n";
    
    return 0;
}

bool GrabIntegerArgument(int argc, char* argv[], int argumentIndex, int& argument, std::string_view switchVal) {
    if (std::string_view(argv[argumentIndex]) == switchVal) {
        if (argumentIndex == argc-1) {
            std::cerr << "You need to specify the value for argument "
                << switchVal << ".\n";
            return false;
        }
        else {
            argument = atoi(argv[argumentIndex + 1]);
            if (argument <= 0) {
                std::cerr << "The value for " << switchVal << " must be > 0.\n";
                return false;
            }
        }
    }
    return true;
}

bool ParseArguments(int argc, char* argv[], int& numTransforms, int& numIterations) {
    for (int i = 0; i < argc; i++) {
        if (!GrabIntegerArgument(argc, argv, i, numTransforms, "--num-transforms")) {
            return false;
        }
        if (!GrabIntegerArgument(argc, argv, i, numTransforms, "--num-iterations")) {
            return false;
        }
    }
    
    return true;
}

void GenerateCurrentFrameValues(
                                const std::vector<Transform>& lastFrameValues,
                                std::vector<Transform>& currentValues) {
    // Mersenne twister along with random_device, which seeds
    // from a hardware entropy source. Static so that we can
    // keep generating across calls. std::mt19937 gives a better
    // distribution compared to rand(). Ultimately this will
    // be a uniform float distribution when used.
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_real_distribution<float> distro(-0.1f, 0.1f);
    static std::uniform_real_distribution<float> distroAxis(-1.0f, 1.0f);
    static std::uniform_real_distribution<float> distroAngle(-0.05f, 0.05f);

    for (int i = 0; i < lastFrameValues.size(); i++) {
        const auto& lastValue = lastFrameValues[i];

        // Position: apply small random offsets, clamped to valid range
        auto oldPosition = lastValue.GetPosition();
        auto modifiedPosition = oldPosition;
        modifiedPosition[0] =
            std::clamp(oldPosition[0] + distro(rng), -PositionBoundMeters*0.5f, PositionBoundMeters*0.5f);
        modifiedPosition[1] =
            std::clamp(oldPosition[1] + distro(rng), -PositionBoundMeters*0.5f, PositionBoundMeters*0.5f);
        modifiedPosition[2] =
            std::clamp(oldPosition[2] + distro(rng), -PositionBoundMeters*0.5f, PositionBoundMeters*0.5f);
        currentValues[i].SetPosition(modifiedPosition);

        // Rotation: apply a small random rotation to the previous quaternion.
        // Build a delta quaternion from a random axis and small angle, multiply,
        // then normalize to keep the result unit-length.
        float ax = distroAxis(rng);
        float ay = distroAxis(rng);
        float az = distroAxis(rng);
        float axisLen = std::sqrt(ax*ax + ay*ay + az*az);
        if (axisLen > 1e-6f) {
            ax /= axisLen; ay /= axisLen; az /= axisLen;
        } else {
            ax = 1.0f; ay = 0.0f; az = 0.0f;
        }
        float halfAngle = distroAngle(rng) * 0.5f;
        float sinHalf = std::sin(halfAngle);
        Quaternion deltaRot(ax * sinHalf, ay * sinHalf, az * sinHalf, std::cos(halfAngle));

        Quaternion oldRot = lastValue.GetRotation();
        Quaternion newRot = deltaRot * oldRot;
        float mag = newRot.magnitude();
        if (mag > 1e-6f) {
            newRot[0] /= mag; newRot[1] /= mag; newRot[2] /= mag; newRot[3] /= mag;
        }
        currentValues[i].SetRotation(newRot);
    }
}

/*void CompressTransforms(WriteStream& writeStream,
                        const std::vector<Transform>& uncompressed,
                        Snapshot& snapshot) {
    for (int i = 0; i < uncompressed.size(); i++) {
        CompressedTransform compressedTransform = snapshot.GetCompressedTransform(i);
        compressedTransform.Compress(uncompressed[i], writeStream);
        snapshot.SetCompressedTransform(compressedTransform, i);
    }
}

void DecompressTransforms(ReadStream& readStream,
                          const Snapshot& snapshot,
                          std::vector<Transform>& uncompressed) {
    for (int i = 0; i < uncompressed.size(); i++) {
        auto compressedTransform = snapshot.GetCompressedTransform(i);
        uncompressed[i] = compressedTransform.Decompress(readStream);
    }
}*/

void ComputeErrorStatistics(const std::vector<Transform>& original,
                            const Snapshot& latestSnapshot,
                            double& sumSquaredError,
                            float& maxError,
                            float& minError,
                            double& sumSquaredErrorQuat,
                            float& maxErrorQuat,
                            float& minErrorQuat,
                            int& sampleCount) {
    for (size_t i = 0; i < original.size(); i++) {
        // Position error: Euclidean distance
        Vec3f originalPos = original[i].GetPosition();
        
        auto snapshotUncompTransform = latestSnapshot.GetTransform(i);
        Vec3f reconstructedPos = snapshotUncompTransform.GetPosition();
        Vec3f diff = originalPos - reconstructedPos;
        float posError = std::sqrt(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
        sumSquaredError += posError * posError;
        maxError = std::max(maxError, posError);
        minError = std::min(minError, posError);

        // Quaternion error: angular distance in radians.
        // dot(q1, q2) = cos(theta/2); use absolute value since q and -q are the same rotation.
        Quaternion origRot = original[i].GetRotation();
        Quaternion recRot  = snapshotUncompTransform.GetRotation();
        float dot = origRot[0]*recRot[0] + origRot[1]*recRot[1]
                  + origRot[2]*recRot[2] + origRot[3]*recRot[3];
        dot = std::clamp(std::abs(dot), 0.0f, 1.0f);
        float quatError = 2.0f * std::acos(dot);
        sumSquaredErrorQuat += quatError * quatError;
        maxErrorQuat = std::max(maxErrorQuat, quatError);
        minErrorQuat = std::min(minErrorQuat, quatError);

        sampleCount++;
    }
}

