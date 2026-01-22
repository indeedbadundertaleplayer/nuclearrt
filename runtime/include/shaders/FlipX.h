#pragma once
#include "Shader.h"
#include "compiled/FlipX.frag.dxil.h"
#include "compiled/FlipX.frag.msl.h"
#include "compiled/FlipX.frag.spv.h"
#include "Application.h"
class FlipX : public Shader {
public:
    FlipX() : Shader("FlipX") {
        hasPixelSize = false;
        numSamplers = 1;
        numUniformBuffers = 0;
        if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "direct3d12") == 0) { 
            code = FlipX_frag_dxil;
            codeSize = FlipX_frag_dxil_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "vulkan") == 0) {
            code = FlipX_frag_spv;
            codeSize = FlipX_frag_spv_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "metal") == 0) {
            code = FlipX_frag_msl;
            codeSize = FlipX_frag_msl_len;
        }
    }
};