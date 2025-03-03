#ifndef WATERDROP_H
#define WATERDROP_H

#include <glm/glm.hpp>

class WaterDrop {
public:
    glm::vec3 position;
    glm::vec3 velocity;
    
    WaterDrop(float x = 0.0f, float y = 0.0f, float z = 0.0f);

    void Update(glm::vec3 accleration);
};

#endif // WATERDROP_H