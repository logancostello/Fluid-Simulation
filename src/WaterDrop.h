#ifndef WATERDROP_H
#define WATERDROP_H

#include <glm/glm.hpp>

class WaterDrop {
public:
    glm::vec3 position;
    
    WaterDrop(float x = 0.0f, float y = 0.0f, float z = 0.0f);
};

#endif // WATERDROP_H