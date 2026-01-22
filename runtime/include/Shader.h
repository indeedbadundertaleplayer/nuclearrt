#pragma once
#include <string>
#include <cstring>
#include <memory>
struct PS_PIXELSIZE {
    float fPixelWidth;
    float fPixelHeight;
};
class Shader {
public:
    explicit Shader(std::string name) : Name(std::move(name)) {
        code = nullptr;
        codeSize = 0;
        pixelSize = {0, 0};
    }
    virtual ~Shader() {
        code = nullptr;
        codeSize = 0;
    }
    virtual void SetParameter(std::string name, float value) {}
    virtual float GetParameter(std::string name) {return 0;}
    virtual const void* GetData() const { return 0; };
    const std::string Name;
    PS_PIXELSIZE pixelSize;
    int numSamplers = 0;
    int numUniformBuffers = 0;
    bool hasPixelSize = false;
    const unsigned char* code;
    unsigned int codeSize;
};