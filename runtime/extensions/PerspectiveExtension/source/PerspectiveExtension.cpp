#include "PerspectiveExtension.h"
#include "Application.h"
void PerspectiveExtension::Initialize() {
    
    Application::Instance().GetBackend()->CreateShader(panorama.Name, panorama.numSamplers, panorama.numUniformBuffers, panorama.code, panorama.codeSize);
    panorama.pixelSize.fPixelWidth = 1.0f / static_cast<float>(Width);
    panorama.pixelSize.fPixelHeight = 1.0f / static_cast<float>(Height);
    panorama.context.noWrap = 1;
    panorama.context.zoom = 300;
    panorama.context.pDir = 0;
    Application::Instance().GetBackend()->SetFragmentUniforms(panorama.Name, 1, &panorama.context, sizeof(panorama.context));
}
void PerspectiveExtension::Draw() {
    Application::Instance().GetBackend()->BeginShaderDraw(panorama.Name);
    Application::Instance().GetBackend()->DrawBGTexture(X, Y, Width, Height, 1.0f, 1.0f);
    Application::Instance().GetBackend()->EndShaderDraw();
}