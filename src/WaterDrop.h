#ifndef WATERDROP_H
#define WATERDROP_H

#include <glm/glm.hpp>

class WaterDrop {
public:
    float radius;
    glm::vec3 position;
    glm::vec3 velocity;
    
    WaterDrop(float x = 0.0f, float y = 0.0f, float z = 0.0f, float radius = 0.1);

    void Update(glm::vec3 accleration);

    void ResolveOutOfBounds(float width, float height, float collisionDamping);
};

#endif // WATERDROP_H