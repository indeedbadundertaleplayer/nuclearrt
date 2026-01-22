#pragma once
#include "Shader.h"
#include "compiled/FlipY.frag.dxil.h"
#include "compiled/FlipY.frag.msl.h"
#include "compiled/FlipY.frag.spv.h"
#include "Application.h"
class FlipY : public Shader {
public:
    FlipY() : Shader("FlipY") {
        hasPixelSize = false;
        numSamplers = 1;
        numUniformBuffers = 0;
        if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "direct3d12") == 0) { 
            code = FlipY_frag_dxil;
            codeSize = FlipY_frag_dxil_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "vulkan") == 0) {
            code = FlipY_frag_spv;
            codeSize = FlipY_frag_spv_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "metal") == 0) {
            code = FlipY_frag_msl;
            codeSize = FlipY_frag_msl_len;
        }
    }
};