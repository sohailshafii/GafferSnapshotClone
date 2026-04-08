#include "Quaternion.h"
#include <cmath>

Quaternion::Quaternion()
{
    data[0] = 0.0f;
    data[1] = 0.0f;
    data[2] = 0.0f;
    data[3] = 1.0f;  // identity: w = 1, real component
}

Quaternion::Quaternion(float x, float y, float z, float w)
{
    data[0] = x;
    data[1] = y;
    data[2] = z;
    data[3] = w;
}

Quaternion& Quaternion::operator+=(const Quaternion& rhs)
{
    data[0] += rhs.data[0];
    data[1] += rhs.data[1];
    data[2] += rhs.data[2];
    data[3] += rhs.data[3];
    return *this;
}

Quaternion& Quaternion::operator-=(const Quaternion& rhs)
{
    data[0] -= rhs.data[0];
    data[1] -= rhs.data[1];
    data[2] -= rhs.data[2];
    data[3] -= rhs.data[3];
    return *this;
}

Quaternion& Quaternion::operator*=(float scalar)
{
    data[0] *= scalar;
    data[1] *= scalar;
    data[2] *= scalar;
    data[3] *= scalar;
    return *this;
}

Quaternion Quaternion::operator+(const Quaternion& rhs) const
{
    return Quaternion(data[0] + rhs.data[0],
                      data[1] + rhs.data[1],
                      data[2] + rhs.data[2],
                      data[3] + rhs.data[3]);
}

Quaternion Quaternion::operator-(const Quaternion& rhs) const
{
    return Quaternion(data[0] - rhs.data[0],
                      data[1] - rhs.data[1],
                      data[2] - rhs.data[2],
                      data[3] - rhs.data[3]);
}

// Hamilton product: (a, w1) * (b, w2) = (w1*b + w2*a + a x b, w1*w2 - a.b)
Quaternion Quaternion::operator*(const Quaternion& rhs) const
{
    const float x1 = data[0], y1 = data[1], z1 = data[2], w1 = data[3];
    const float x2 = rhs.data[0], y2 = rhs.data[1], z2 = rhs.data[2], w2 = rhs.data[3];

    return Quaternion(
        w1*x2 + x1*w2 + y1*z2 - z1*y2,
        w1*y2 - x1*z2 + y1*w2 + z1*x2,
        w1*z2 + x1*y2 - y1*x2 + z1*w2,
        w1*w2 - x1*x2 - y1*y2 - z1*z2
    );
}

float Quaternion::magnitude() const
{
    return std::sqrt(data[0]*data[0] + data[1]*data[1] +
                     data[2]*data[2] + data[3]*data[3]);
}

bool Quaternion::isIdentity(float epsilon) const
{
    return std::abs(data[0]) <= epsilon &&
           std::abs(data[1]) <= epsilon &&
           std::abs(data[2]) <= epsilon &&
           std::abs(data[3] - 1.0f) <= epsilon;
}
