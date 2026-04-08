#pragma once

#include "Vec3f.h"
#include "Quaternion.h"

class Transform {
public:
    Transform();
    Transform(const Vec3f& position, const Quaternion& rotation);

    Vec3f GetPosition() const {
        return position;
    }
    
    void SetPosition(const Vec3f& newVal) {
        position = newVal;
    }

    Quaternion GetRotation() const {
        return rotation;
    }
    
    void SetRotation(const Quaternion& rotation) {
        this->rotation = rotation;
    }
    
private:
    Vec3f position;
    Quaternion rotation;
};
