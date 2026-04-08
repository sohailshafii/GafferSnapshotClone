#include <gtest/gtest.h>
#include <cmath>
#include "Quaternion.h"

TEST(QuaternionTest, DefaultConstructorIsIdentity)
{
    Quaternion q;
    EXPECT_FLOAT_EQ(q[0], 0.0f);
    EXPECT_FLOAT_EQ(q[1], 0.0f);
    EXPECT_FLOAT_EQ(q[2], 0.0f);
    EXPECT_FLOAT_EQ(q[3], 1.0f);
}

TEST(QuaternionTest, ValueConstructor)
{
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    EXPECT_FLOAT_EQ(q[0], 1.0f);
    EXPECT_FLOAT_EQ(q[1], 2.0f);
    EXPECT_FLOAT_EQ(q[2], 3.0f);
    EXPECT_FLOAT_EQ(q[3], 4.0f);
}

TEST(QuaternionTest, IndexOperatorMutable)
{
    Quaternion q;
    q[0] = 1.0f;
    q[1] = 2.0f;
    q[2] = 3.0f;
    q[3] = 4.0f;
    EXPECT_FLOAT_EQ(q[0], 1.0f);
    EXPECT_FLOAT_EQ(q[1], 2.0f);
    EXPECT_FLOAT_EQ(q[2], 3.0f);
    EXPECT_FLOAT_EQ(q[3], 4.0f);
}

TEST(QuaternionTest, AdditionOperator)
{
    Quaternion a(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion b(0.5f, 0.5f, 0.5f, 0.5f);
    Quaternion c = a + b;
    EXPECT_FLOAT_EQ(c[0], 1.5f);
    EXPECT_FLOAT_EQ(c[1], 2.5f);
    EXPECT_FLOAT_EQ(c[2], 3.5f);
    EXPECT_FLOAT_EQ(c[3], 4.5f);
}

TEST(QuaternionTest, SubtractionOperator)
{
    Quaternion a(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion b(0.5f, 0.5f, 0.5f, 0.5f);
    Quaternion c = a - b;
    EXPECT_FLOAT_EQ(c[0], 0.5f);
    EXPECT_FLOAT_EQ(c[1], 1.5f);
    EXPECT_FLOAT_EQ(c[2], 2.5f);
    EXPECT_FLOAT_EQ(c[3], 3.5f);
}

TEST(QuaternionTest, PlusEqualsOperator)
{
    Quaternion a(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion b(1.0f, 1.0f, 1.0f, 1.0f);
    a += b;
    EXPECT_FLOAT_EQ(a[0], 2.0f);
    EXPECT_FLOAT_EQ(a[1], 3.0f);
    EXPECT_FLOAT_EQ(a[2], 4.0f);
    EXPECT_FLOAT_EQ(a[3], 5.0f);
}

TEST(QuaternionTest, MinusEqualsOperator)
{
    Quaternion a(2.0f, 3.0f, 4.0f, 5.0f);
    Quaternion b(1.0f, 1.0f, 1.0f, 1.0f);
    a -= b;
    EXPECT_FLOAT_EQ(a[0], 1.0f);
    EXPECT_FLOAT_EQ(a[1], 2.0f);
    EXPECT_FLOAT_EQ(a[2], 3.0f);
    EXPECT_FLOAT_EQ(a[3], 4.0f);
}

TEST(QuaternionTest, ScalarMultiplyEquals)
{
    Quaternion q(1.0f, 2.0f, 3.0f, 4.0f);
    q *= 2.0f;
    EXPECT_FLOAT_EQ(q[0], 2.0f);
    EXPECT_FLOAT_EQ(q[1], 4.0f);
    EXPECT_FLOAT_EQ(q[2], 6.0f);
    EXPECT_FLOAT_EQ(q[3], 8.0f);
}

TEST(QuaternionTest, MultiplyByIdentityIsUnchanged)
{
    Quaternion a(0.5f, 0.5f, 0.5f, 0.5f);
    Quaternion identity;
    Quaternion result = a * identity;
    EXPECT_FLOAT_EQ(result[0], a[0]);
    EXPECT_FLOAT_EQ(result[1], a[1]);
    EXPECT_FLOAT_EQ(result[2], a[2]);
    EXPECT_FLOAT_EQ(result[3], a[3]);
}

TEST(QuaternionTest, MultiplyIsNonCommutative)
{
    Quaternion a(1.0f, 0.0f, 0.0f, 0.0f);
    Quaternion b(0.0f, 1.0f, 0.0f, 0.0f);
    Quaternion ab = a * b;
    Quaternion ba = b * a;
    // i*j = k, j*i = -k
    EXPECT_FLOAT_EQ(ab[2],  1.0f);
    EXPECT_FLOAT_EQ(ba[2], -1.0f);
}

TEST(QuaternionTest, MultiplyTwoUnitQuaternionsProducesUnitQuaternion)
{
    // 90-degree rotation around X axis
    const float s = std::sqrt(2.0f) / 2.0f;
    Quaternion a(s, 0.0f, 0.0f, s);
    Quaternion b(0.0f, s, 0.0f, s);
    Quaternion result = a * b;
    EXPECT_NEAR(result.magnitude(), 1.0f, 1e-6f);
}

TEST(QuaternionTest, MagnitudeOfIdentityIsOne)
{
    Quaternion q;
    EXPECT_FLOAT_EQ(q.magnitude(), 1.0f);
}

TEST(QuaternionTest, MagnitudeComputation)
{
    // magnitude of (0, 0, 0, 2) = 2
    Quaternion q(0.0f, 0.0f, 0.0f, 2.0f);
    EXPECT_FLOAT_EQ(q.magnitude(), 2.0f);
}

TEST(QuaternionTest, IsIdentityForDefaultConstructor)
{
    Quaternion q;
    EXPECT_TRUE(q.isIdentity());
}

TEST(QuaternionTest, IsIdentityFalseForNonIdentity)
{
    Quaternion q(1.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_FALSE(q.isIdentity());
}

TEST(QuaternionTest, IsIdentityRespectsEpsilon)
{
    Quaternion q(1e-7f, 0.0f, 0.0f, 1.0f);
    EXPECT_TRUE(q.isIdentity());   // within default epsilon
    EXPECT_FALSE(q.isIdentity(1e-8f));  // outside tighter epsilon
}
