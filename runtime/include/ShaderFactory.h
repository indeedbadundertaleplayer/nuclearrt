#pragma once

#include <unordered_map>
#include <functional>
#include <memory>
#include "Shader.h"
#include "shaders/None.h"
#include "shaders/Panorama.h"
#include "shaders/ChannelOffset.h"
#include "shaders/FlipX.h"
#include "shaders/FlipY.h"
#include "shaders/Display.h"
#include "shaders/MonoExample.h"
extern std::unordered_map<std::string, std::function<std::unique_ptr<Shader>()>> shaderFactory;