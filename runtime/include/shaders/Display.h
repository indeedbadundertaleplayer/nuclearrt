#pragma once
#include "Shader.h"
#include "compiled/Display.frag.dxil.h"
#include "compiled/Display.frag.msl.h"
#include "compiled/Display.frag.spv.h"
#include "Application.h"
struct DisplayContext {
    float fAmplitude = .15f;
    float fPeriods = 1000.0f;
    float fOffset = 0.0f;
};
class Display : public Shader {
public:
    Display() : Shader("Display") {
        hasPixelSize = false;
        numSamplers = 1;
        numUniformBuffers = 1;
        if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "direct3d12") == 0) {
            code = Display_frag_dxil;
            codeSize = Display_frag_dxil_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "vulkan") == 0) {
            code = Display_frag_spv;
            codeSize = Display_frag_spv_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "metal") == 0) {
            code = Display_frag_msl;
            codeSize = Display_frag_msl_len;
        }
        else std::cout << "UnknownRendererName\n";
    }
    float GetParameter(std::string name) override {
        if (name == "fAmplitude") return context.fAmplitude;
        else if (name == "fPeriods") return context.fPeriods;
        else if (name == "fOffset") return context.fOffset;
        else return .0f;
    }
    void SetParameter(std::string name, float value) override {
        if (name == "fAmplitude") context.fAmplitude = value;
        else if (name == "fPeriods") context.fPeriods = value;
        else if (name == "fOffset") context.fOffset = value;
    }
    const void* GetData() const override { return &context; }
    DisplayContext context{};
};