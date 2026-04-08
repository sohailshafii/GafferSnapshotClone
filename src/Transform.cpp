#include "Transform.h"

Transform::Transform()
{
}

Transform::Transform(const Vec3f& position, const Quaternion& rotation)
{
    this->position = position;
    this->rotation = rotation;
}
