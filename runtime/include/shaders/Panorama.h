#pragma once
#include "Shader.h"
#include "compiled/RPanorama.frag.dxil.h"
#include "compiled/RPanorama.frag.msl.h"
#include "compiled/RPanorama.frag.spv.h"
#include "Application.h"
struct ContextPanorama {
    int zoom = 300;
    int pDir = 0;
    int noWrap = 1;
};
class Panorama : public Shader {
public:
    Panorama() : Shader("Panorama") {
        hasPixelSize = true;
        numSamplers = 1;
        numUniformBuffers = 1;
        if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "direct3d12") == 0) {
            code = RPanorama_frag_dxil;
            codeSize = RPanorama_frag_dxil_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "vulkan") == 0) {
            code = RPanorama_frag_spv;
            codeSize = RPanorama_frag_spv_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "direct3d12") == 0) {
            code = RPanorama_frag_msl;
            codeSize = RPanorama_frag_msl_len;
        }
    }
    void SetParameter(std::string name, float value) override {
        if (name == "pDir") context.pDir = (int)value;
        else if (name == "zoom") context.zoom = value;
        else if (name == "noWrap") context.noWrap = (int)value;
    }
    float GetParameter(std::string name) override {
        if (name == "pDir") return context.pDir;
        else if (name == "zoom") return context.zoom;
        else if (name == "noWrap") return context.noWrap;
        else return 0;
    }
    const void* GetData() const override { return &context; }
    ContextPanorama context{};
};