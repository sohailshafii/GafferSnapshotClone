#pragma once

class Vec3f
{
public:
    Vec3f();
    Vec3f(float x, float y, float z);

    float&       operator[](int index);
    const float& operator[](int index) const;

    Vec3f& operator+=(const Vec3f& rhs);
    Vec3f& operator-=(const Vec3f& rhs);
    Vec3f& operator*=(float scalar);

    Vec3f operator+(const Vec3f& rhs) const;
    Vec3f operator-(const Vec3f& rhs) const;

private:
    float data[3];
};

inline float& Vec3f::operator[](int index)
{
    return data[index];
}

inline const float& Vec3f::operator[](int index) const
{
    return data[index];
}
