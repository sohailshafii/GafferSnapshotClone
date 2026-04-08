#pragma once

class Quaternion
{
public:
    Quaternion();
    Quaternion(float x, float y, float z, float w);

    float&       operator[](int index);
    const float& operator[](int index) const;

    Quaternion& operator+=(const Quaternion& rhs);
    Quaternion& operator-=(const Quaternion& rhs);
    Quaternion& operator*=(float scalar);

    Quaternion operator+(const Quaternion& rhs) const;
    Quaternion operator-(const Quaternion& rhs) const;
    Quaternion operator*(const Quaternion& rhs) const;

    float magnitude() const;
    bool  isIdentity(float epsilon = 1e-6f) const;

private:
    float data[4];
};

inline float& Quaternion::operator[](int index)
{
    return data[index];
}

inline const float& Quaternion::operator[](int index) const
{
    return data[index];
}
