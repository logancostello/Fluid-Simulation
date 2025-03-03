#include "WaterDrop.h"
#include <glm/glm.hpp>
#include <iostream>


WaterDrop::WaterDrop(float x, float y, float z, float radius) : position(x, y, z), radius(radius) {
    velocity = glm::vec3(0, 0, 0);
}

void WaterDrop::Update(glm::vec3 acceleration) {
    velocity += acceleration * 0.016f; // 60 fps
    position += velocity * 0.016f;
}

void WaterDrop::ResolveOutOfBounds(float width, float height, float collisionDamping) {
    if (position.y - radius < -height / 2) {
        position.y = -height / 2 + radius;
        velocity.y *= -1 * collisionDamping;
    } else if (position.y + radius > height / 2) {
        position.y = height / 2 - radius;
        velocity.y *= -1 * collisionDamping;
    }

    if (position.x - radius < -width / 2) {
        position.x = -width / 2 + radius;
        velocity.x *= -1 * collisionDamping;
    } else if (position.x + radius > width / 2) {
        position.x = width / 2 - radius;
        velocity.x *= -1 * collisionDamping;
    }
}