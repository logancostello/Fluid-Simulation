#include "WaterDrop.h"
#include <glm/glm.hpp>
#include <iostream>


WaterDrop::WaterDrop(float x, float y, float z, float radius) : position(x, y, z), radius(radius) {
    velocity = glm::vec3(0, 0, 0);
}

void WaterDrop::Update(glm::vec3 acceleration, float deltaTime) {
    velocity += acceleration * deltaTime; 
    position += velocity * deltaTime;
}

void WaterDrop::ResolveOutOfBounds(float width, float height, float collisionDamping) {
    float top = height / 2;
    float bottom = -height / 2;

    float bottomExcess = bottom - (position.y - radius);
    float topExcess = (position.y + radius) - top;

    if (bottomExcess > 0) {
        if (bottomExcess < 0.1) {
            // Prevents jiggling when should be still
            position.y = bottom + radius;
        } else {
            // Move up 2x however much it moved under
            position.y += 2 * bottomExcess;
        }
        velocity.y *= -1 ;//* collisionDamping;
    } else if (topExcess > 0) {
        if (topExcess < 0.1) {
            position.y = top - radius;
        } else {
            position.y -= 2 * topExcess;
        }
        velocity.y *= -1 ;//* collisionDamping;
    }

    float right = width / 2;
    float left = -width / 2;

    float leftExcess = left - (position.x - radius);
    float rightExcess = (position.x + radius) - right;

    if (leftExcess > 0) {
        if (leftExcess < 0.1) {
            position.x = left + radius;
        } else {
            position.x += 2 * leftExcess;
        }
        velocity.x *= -1 ;//* collisionDamping;
    } else if (rightExcess > 0) {
        if (rightExcess < 0.1) {
            position.x = right - radius;
        } else {
            position.x -= 2 * rightExcess;
        }
        velocity.x *= -1 ;//* collisionDamping;
    }
}