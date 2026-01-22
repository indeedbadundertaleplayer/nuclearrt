#pragma once
#include "Shader.h"
#include "compiled/MonoExample.frag.dxil.h"
#include "compiled/MonoExample.frag.msl.h"
#include "compiled/MonoExample.frag.spv.h"
#include "Application.h"
class MonoExample : public Shader {
public:
    MonoExample() : Shader("MonoExample") {
        hasPixelSize = false;
        numSamplers = 1;
        numUniformBuffers = 0;
        if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "direct3d12") == 0) {
            code = MonoExample_frag_dxil;
            codeSize = MonoExample_frag_dxil_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "vulkan") == 0) {
            code = MonoExample_frag_spv;
            codeSize = MonoExample_frag_spv_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "metal") == 0) {
            code = MonoExample_frag_msl;
            codeSize = MonoExample_frag_msl_len;
        }
    }
};