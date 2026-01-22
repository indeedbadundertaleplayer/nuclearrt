#include "ShaderFactory.h"
std::unordered_map<std::string, std::function<std::unique_ptr<Shader>()>> shaderFactory = {
    {"None", [](){return std::make_unique<None>();}},
    {"RPanorama", [](){return std::make_unique<Panorama>();}},
    {"ChannelOffset", [](){return std::make_unique<ChannelOffset>();}},
    {"FlipX", [](){return std::make_unique<FlipX>();}},
    {"FlipY", [](){return std::make_unique<FlipY>();}},
    {"Display", [](){return std::make_unique<Display>();}},
    {"MonoExample", [](){return std::make_unique<MonoExample>();}}
};