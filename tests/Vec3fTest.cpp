#include <gtest/gtest.h>
#include "Vec3f.h"

TEST(Vec3fTest, DefaultConstructorIsZero)
{
    Vec3f v;
    EXPECT_FLOAT_EQ(v[0], 0.0f);
    EXPECT_FLOAT_EQ(v[1], 0.0f);
    EXPECT_FLOAT_EQ(v[2], 0.0f);
}

TEST(Vec3fTest, ValueConstructor)
{
    Vec3f v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v[0], 1.0f);
    EXPECT_FLOAT_EQ(v[1], 2.0f);
    EXPECT_FLOAT_EQ(v[2], 3.0f);
}

TEST(Vec3fTest, IndexOperatorMutable)
{
    Vec3f v;
    v[0] = 4.0f;
    v[1] = 5.0f;
    v[2] = 6.0f;
    EXPECT_FLOAT_EQ(v[0], 4.0f);
    EXPECT_FLOAT_EQ(v[1], 5.0f);
    EXPECT_FLOAT_EQ(v[2], 6.0f);
}

TEST(Vec3fTest, AdditionOperator)
{
    Vec3f a(1.0f, 2.0f, 3.0f);
    Vec3f b(4.0f, 5.0f, 6.0f);
    Vec3f c = a + b;
    EXPECT_FLOAT_EQ(c[0], 5.0f);
    EXPECT_FLOAT_EQ(c[1], 7.0f);
    EXPECT_FLOAT_EQ(c[2], 9.0f);
}

TEST(Vec3fTest, SubtractionOperator)
{
    Vec3f a(4.0f, 5.0f, 6.0f);
    Vec3f b(1.0f, 2.0f, 3.0f);
    Vec3f c = a - b;
    EXPECT_FLOAT_EQ(c[0], 3.0f);
    EXPECT_FLOAT_EQ(c[1], 3.0f);
    EXPECT_FLOAT_EQ(c[2], 3.0f);
}

TEST(Vec3fTest, PlusEqualsOperator)
{
    Vec3f a(1.0f, 2.0f, 3.0f);
    Vec3f b(1.0f, 1.0f, 1.0f);
    a += b;
    EXPECT_FLOAT_EQ(a[0], 2.0f);
    EXPECT_FLOAT_EQ(a[1], 3.0f);
    EXPECT_FLOAT_EQ(a[2], 4.0f);
}

TEST(Vec3fTest, MinusEqualsOperator)
{
    Vec3f a(3.0f, 3.0f, 3.0f);
    Vec3f b(1.0f, 2.0f, 3.0f);
    a -= b;
    EXPECT_FLOAT_EQ(a[0], 2.0f);
    EXPECT_FLOAT_EQ(a[1], 1.0f);
    EXPECT_FLOAT_EQ(a[2], 0.0f);
}

TEST(Vec3fTest, ScalarMultiplyEquals)
{
    Vec3f v(1.0f, 2.0f, 3.0f);
    v *= 3.0f;
    EXPECT_FLOAT_EQ(v[0], 3.0f);
    EXPECT_FLOAT_EQ(v[1], 6.0f);
    EXPECT_FLOAT_EQ(v[2], 9.0f);
}

TEST(Vec3fTest, ScalarMultiplyByZero)
{
    Vec3f v(1.0f, 2.0f, 3.0f);
    v *= 0.0f;
    EXPECT_FLOAT_EQ(v[0], 0.0f);
    EXPECT_FLOAT_EQ(v[1], 0.0f);
    EXPECT_FLOAT_EQ(v[2], 0.0f);
}

TEST(Vec3fTest, OperatorChaining)
{
    Vec3f a(1.0f, 2.0f, 3.0f);
    Vec3f b(1.0f, 1.0f, 1.0f);
    Vec3f c(0.5f, 0.5f, 0.5f);
    a += b;
    a -= c;
    EXPECT_FLOAT_EQ(a[0], 1.5f);
    EXPECT_FLOAT_EQ(a[1], 2.5f);
    EXPECT_FLOAT_EQ(a[2], 3.5f);
}
