#include "WaterDrop.h"
#include <glm/glm.hpp>


WaterDrop::WaterDrop(float x, float y, float z) : position(x, y, z) {
    velocity = glm::vec3(0, 0, 0);
}

void WaterDrop::Update(glm::vec3 acceleration) {
    velocity += acceleration * 0.016f; // 60 fps
    position += velocity * 0.016f;
}