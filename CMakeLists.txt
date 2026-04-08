cmake_minimum_required(VERSION 3.20)

set(CMAKE_OSX_DEPLOYMENT_TARGET "26.3" CACHE STRING "Minimum macOS deployment version")

project(GafferSnapshotClone
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
    src/main.cpp
    src/Vec3f.cpp
    src/Quaternion.cpp
    src/Transform.cpp
    src/CompressedTransform.cpp
    src/Snapshot.cpp
    src/BitWriter.cpp
    src/BitReader.cpp
    src/WriteStream.cpp
    src/ReadStream.cpp
    include/Vec3f.h
    include/Quaternion.h
    include/Transform.h
    include/CompressedTransform.h
    include/Snapshot.h
    include/BitWriter.h
    include/BitReader.h
    include/BitOperationsCommon.h
    include/WriteStream.h
    include/ReadStream.h
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# --- Unit tests ---
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
# Prevent gtest from overriding our compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TEST_SOURCES
    tests/Vec3fTest.cpp
    src/Vec3f.cpp
    include/Vec3f.h
)

add_executable(Vec3fTests ${TEST_SOURCES})

target_include_directories(Vec3fTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Vec3fTests PRIVATE GTest::gtest_main)

add_test(NAME Vec3fTests COMMAND Vec3fTests)

set(QUATERNION_TEST_SOURCES
    tests/QuaternionTest.cpp
    src/Quaternion.cpp
    include/Quaternion.h
)

add_executable(QuaternionTests ${QUATERNION_TEST_SOURCES})

target_include_directories(QuaternionTests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(QuaternionTests PRIVATE GTest::gtest_main)

add_test(NAME QuaternionTests COMMAND QuaternionTests)
