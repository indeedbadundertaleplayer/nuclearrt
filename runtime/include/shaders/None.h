#pragma once
#include "Shader.h"
class None : public Shader {
public:
    None() : Shader("None") {
        numSamplers = 0;
        numUniformBuffers = 0;
    }
};