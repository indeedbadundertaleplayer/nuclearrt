#pragma once
#include "Shader.h"
#include "compiled/RPanorama.frag.dxil.h" // DirectX 12 (Windows, Xbox)
#include "compiled/RPanorama.frag.msl.h" // Metal (Apple Devices)
#include "compiled/RPanorama.frag.spv.h" // Vulkan (Windows, Linux, Android, Nintendo Switch)
#include "Application.h"
struct ContextPanorama {
    float pixelWidth;
    float pixelHeight;
    int zoom = 300;
    int pDir = 0;
    int noWrap = 1;
    int _pad;
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