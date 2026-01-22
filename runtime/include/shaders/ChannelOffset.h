#pragma once
#include "Shader.h"
#include "compiled/ChannelOffset.frag.dxil.h"
#include "compiled/ChannelOffset.frag.msl.h"
#include "compiled/ChannelOffset.frag.spv.h"
#include "Application.h"
struct ChannelOffsetContext {
    float Rx;
    float Ry;
    float Gx;
    float Gy;
    float Bx;
    float By;
    float Ax;
    float Ay;
};
class ChannelOffset : public Shader {
public:
    ChannelOffset() : Shader("ChannelOffset") {
        hasPixelSize = true;
        numSamplers = 1;
        numUniformBuffers = 2;
        if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "direct3d12") == 0) {
            code = ChannelOffset_frag_dxil;
            codeSize = ChannelOffset_frag_dxil_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "vulkan") == 0) {
            code = ChannelOffset_frag_spv;
            codeSize = ChannelOffset_frag_spv_len;
        }
        else if (std::strcmp(Application::Instance().GetBackend()->GetRendererName(), "metal") == 0) {
            code = ChannelOffset_frag_msl;
            codeSize = ChannelOffset_frag_msl_len;
        }
    }
    void SetParameter(std::string name, float value) override {
        if (name == "Rx") context.Rx = value;
        else if (name == "Ry") context.Ry = value;
        else if (name == "Gx") context.Gx = value;
        else if (name == "Gy") context.Gy = value;
        else if (name == "Bx") context.Bx = value;
        else if (name == "By") context.By = value;
        else if (name == "Ax") context.Ax = value;
        else if (name == "Ay") context.Ay = value;
    }
    float GetParameter(std::string name) override {
        if (name == "Rx") return context.Rx;
        else if (name == "Ry") return context.Ry;
        else if (name == "Gx") return context.Gx;
        else if (name == "Gy") return context.Gy;
        else if (name == "Bx") return context.Bx;
        else if (name == "By") return context.By;
        else if (name == "Ax") return context.Ax;
        else if (name == "Ay") return context.Ay;
        else return .0f;
    }
    const void* GetData() const override { return &context; }
    ChannelOffsetContext context{};
};