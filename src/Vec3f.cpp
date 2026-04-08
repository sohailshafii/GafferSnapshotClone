#include "Vec3f.h"

Vec3f::Vec3f()
{
    data[0] = 0.0f;
    data[1] = 0.0f;
    data[2] = 0.0f;
}

Vec3f::Vec3f(float x, float y, float z)
{
    data[0] = x;
    data[1] = y;
    data[2] = z;
}

Vec3f& Vec3f::operator+=(const Vec3f& rhs)
{
    data[0] += rhs.data[0];
    data[1] += rhs.data[1];
    data[2] += rhs.data[2];
    return *this;
}

Vec3f& Vec3f::operator-=(const Vec3f& rhs)
{
    data[0] -= rhs.data[0];
    data[1] -= rhs.data[1];
    data[2] -= rhs.data[2];
    return *this;
}

Vec3f& Vec3f::operator*=(float scalar)
{
    data[0] *= scalar;
    data[1] *= scalar;
    data[2] *= scalar;
    return *this;
}

Vec3f Vec3f::operator+(const Vec3f& rhs) const
{
    return Vec3f(data[0] + rhs.data[0],
                 data[1] + rhs.data[1],
                 data[2] + rhs.data[2]);
}

Vec3f Vec3f::operator-(const Vec3f& rhs) const
{
    return Vec3f(data[0] - rhs.data[0],
                 data[1] - rhs.data[1],
                 data[2] - rhs.data[2]);
}
