#pragma once

#include "Extension.h"
#include "shaders/Panorama.h"
class PerspectiveExtension : public Extension {
public:
    PerspectiveExtension(unsigned int objectInfoHandle, int type, std::string name, short width, short height, short effectType) : Extension(objectInfoHandle, type, name), Width(width), Height(height), EffectType(effectType) {}
    ~PerspectiveExtension() = default;
    void Initialize() override;
    void Update(float deltaTime) override {}
    void Draw() override;
private:
    Panorama panorama;
    short Width;
    short Height;
    short EffectType;
};