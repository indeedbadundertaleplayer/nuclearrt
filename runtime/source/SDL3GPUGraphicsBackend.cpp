#ifdef NUCLEAR_BACKEND_SDL3
#include "SDL3GPUGraphicsBackend.h"
#include "SDL3Backend.h"
#include "ImageBank.h"
#include "Application.h"
#include "Frame.h"
void SDL3GPUBackend::Initialize()
{
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();
	std::string windowTitle = Application::Instance().GetAppData()->GetAppName();
	
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
		backend->GetPlatform()->Log("SDL_Init Error: " + std::string(SDL_GetError()));
		return;
	}

	if (!TTF_Init()) {
		backend->GetPlatform()->Log("TTF_Init Error: " + std::string(SDL_GetError()));
		return;
	}

	// Create the window
	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
	window = SDL_CreateWindow(windowTitle.c_str(), windowWidth, windowHeight, flags);
	if (window == nullptr) {
		backend->GetPlatform()->Log("SDL_CreateWindow Error: " + std::string(SDL_GetError()));
		return;
	}

    gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (!gpuDevice) {
		backend->GetPlatform()->Log("SDL_CreateGPUDevice Error: " + std::string(SDL_GetError()));
        return;
    }
    if (!SDL_ClaimWindowForGPUDevice(gpuDevice, window)) {
		backend->GetPlatform()->Log("SDL_ClaimWindowForGPUDevice Error: " + std::string(SDL_GetError()));
        return;
    }
    swapChainFormat = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window);
	defaultVertexShader = LoadShader("shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/standard/default.vert", 0, 0, 0, 1);
	CreatePipeline("shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/standard/normal.frag", "INTERNAL_normal");
	CreatePipeline("shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/standard/normal.frag", "INTERNAL_add");
	CreatePipeline("shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/standard/normal.frag", "INTERNAL_subtract");
	SDL_GPUSamplerCreateInfo samplerInfos[2]{};
	samplerInfos->address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerInfos->address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerInfos->address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	samplerInfos[0].mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	samplerInfos[1].mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
	samplerInfos[0].min_filter = SDL_GPU_FILTER_NEAREST;
	samplerInfos[0].mag_filter = SDL_GPU_FILTER_NEAREST;
	samplerInfos[1].min_filter = SDL_GPU_FILTER_LINEAR;
	samplerInfos[1].mag_filter = SDL_GPU_FILTER_LINEAR;
	samplers[0] = SDL_CreateGPUSampler(gpuDevice, &samplerInfos[0]);
	samplers[1] = SDL_CreateGPUSampler(gpuDevice, &samplerInfos[1]);
	backend->GetPlatform()->Log("Samplers created.");
	SDL_GPUBufferCreateInfo indexBufInfo{};
	SDL_GPUBufferCreateInfo vertexBufInfo{};
	indexBufInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
	indexBufInfo.size = sizeof(Uint16) * 6;
	vertexBufInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	vertexBufInfo.size = sizeof(Vertex) * 4;
	vertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &vertexBufInfo);
	indexBuffer = SDL_CreateGPUBuffer(gpuDevice, &indexBufInfo);
	UpdateBuffer();
	backend->GetPlatform()->Log("Inited Graphics Backend");
}

void SDL3GPUBackend::Deinitialize()
{
	SDL_HideWindow(window);
	SDL_WaitForGPUIdle(gpuDevice);
	for (auto& pair : pipelinesToUse) {
		if (!pair.second) continue;
		SDL_ReleaseGPUGraphicsPipeline(gpuDevice, pair.second);
		pair.second = nullptr;
	}
	if (samplers[0]) {
		SDL_ReleaseGPUSampler(gpuDevice, samplers[0]);
		samplers[0] = nullptr;
	}
	if (samplers[1]) {
		SDL_ReleaseGPUSampler(gpuDevice, samplers[1]);
		samplers[1] = nullptr;
	}
	for (auto& pair : textCache) {
		if (pair.second->texture) {
			SDL_ReleaseGPUTexture(gpuDevice, pair.second->texture);
			pair.second->texture = nullptr;
		}
	}
	textCache.clear();
	for (auto& pair : fonts) {
		TTF_CloseFont(pair.second);
	}
	fonts.clear();
	fontBuffers.clear();
	for (auto& pair : textureSizeMap) {
		pair.second.width = 0;
		pair.second.height = 0;
	}
	textureSizeMap.clear();
	for (auto& pair : textures) {
		if (!pair.second) continue;
		SDL_ReleaseGPUTexture(gpuDevice, pair.second);
		pair.second = nullptr;
	}
	textures.clear();
	if (layerRenderTarget) {
		SDL_ReleaseGPUTexture(gpuDevice, layerRenderTarget);
		layerRenderTarget;
	}
	if (renderTarget) {
		SDL_ReleaseGPUTexture(gpuDevice, renderTarget);
		renderTarget = nullptr;
	}
	renderTargetWidth = 0;
	renderTargetHeight = 0;
	if (defaultVertexShader) {
		SDL_ReleaseGPUShader(gpuDevice, defaultVertexShader);
		defaultVertexShader = nullptr;
	}
	for (auto& pair : pipelinesToUse) {
		if (!pair.second) continue;
		SDL_ReleaseGPUGraphicsPipeline(gpuDevice, pair.second);
		pair.second = nullptr;
	}
	pipelinesToUse.clear();
	if (gpuDevice) {
		SDL_ReleaseWindowFromGPUDevice(gpuDevice, window);
		SDL_DestroyGPUDevice(gpuDevice);
		gpuDevice = nullptr;
	}
	TTF_Quit();
}

void SDL3GPUBackend::BeginDrawing()
{
	if (gpuDevice == nullptr) {
		backend->GetPlatform()->Log("BeginDrawing called with null GPU Device.");
		return;
	}
	
	int newWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	int newHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);
	if (newWidth != renderTargetWidth || newHeight != renderTargetHeight) {
		backend->GetPlatform()->Log("Remake Render Target");
		if (renderTarget) {
			SDL_ReleaseGPUTexture(gpuDevice, renderTarget);
			renderTarget = nullptr;
		}
		renderTarget = CreateRenderTarget(newWidth, newHeight);
	}
	cmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
	beginTargetInfo.texture = renderTarget;
	
	beginTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	beginTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
	beginTargetInfo.clear_color = {1.0f, 1.0f, 1.0f, 1.0f};
	renderPass = SDL_BeginGPURenderPass(cmdBuf, &beginTargetInfo, 1, NULL);
}
void SDL3GPUBackend::EndDrawing() {
	if (gpuDevice == nullptr || !renderPass) {return;}
	SDL_EndGPURenderPass(renderPass);
	SDL_GPUTexture* swapchainTexture = nullptr;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBuf, window, &swapchainTexture, NULL, NULL)) {
		backend->GetPlatform()->Log("SDL_WaitAndAcquireGPUSwapchainTexture Error : " + std::string(SDL_GetError()));
		return;
	}
	if (swapchainTexture != NULL){
		SDL_GPUColorTargetInfo colorTargetInfo{};
		int borderColor = Application::Instance().GetAppData()->GetBorderColor();
		float r = ((borderColor >> 16) & 0xFF) / 255.0f;
		float g = ((borderColor >> 8) & 0xFF) / 255.0f;
		float b = (borderColor & 0xFF) / 255.0f;
		colorTargetInfo.clear_color = {r, g, b, 1.0f};
		colorTargetInfo.texture = swapchainTexture;
		colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
		colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
		renderPass = SDL_BeginGPURenderPass(cmdBuf, &colorTargetInfo, 1, NULL);
		if (renderTarget) {
			SDL_GPUTextureSamplerBinding textureBindings;
			textureBindings.texture = renderTarget;
			textureBindings.sampler = samplers[0];
			SDL_GPUBufferBinding vertBufBinding{vertexBuffer, 0}, indexBufBinding{indexBuffer, 0};
			int windowWidth, windowHeight;
			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			SDL_FRect rect = CalculateRenderTargetRect();
			/*float mvp[16] = {
				2.0f * rect.w / windowWidth, 0.0f, 0.0f, 0.0f,
				0.0f, 2.0f * rect.h / windowHeight, 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f, 0.0f,
				2.0f * rect.x / windowWidth - 1.0f, -(2.0f * rect.y / windowHeight - 1.0f) - 2.0f * rect.h / windowHeight, 0.0f, 1.0f
			};*/
			float mvp[16] = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			};
			ShaderUBO shaderUBO = {
				{},
				{},
				{1.0f,1.0f,1.0f,1.0f},
				0,
				0,
				{}
			};
			int shaderMode = 0;
			SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse["INTERNAL_normal"]);
			SDL_BindGPUVertexBuffers(renderPass, 0, &vertBufBinding, 1);
			SDL_BindGPUIndexBuffer(renderPass, &indexBufBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
			SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBindings, 1);
			SDL_PushGPUVertexUniformData(cmdBuf, 0, mvp, sizeof(mvp));
			SDL_PushGPUFragmentUniformData(cmdBuf, 0, &shaderUBO, sizeof(shaderUBO));
			SDL_PushGPUFragmentUniformData(cmdBuf, 1, &shaderMode, sizeof(shaderMode));
			SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
		}
		SDL_EndGPURenderPass(renderPass);
		SDL_SubmitGPUCommandBuffer(cmdBuf);
	}
	else {
		SDL_CancelGPUCommandBuffer(cmdBuf);
		return;
	}
	if (!renderedFirstFrame) {
		renderedFirstFrame = true;
		SDL_ShowWindow(window);
	}
}
void SDL3GPUBackend::Clear(int color)
{
	float r = ((color >> 16) & 0xFF) / 255.0f;
	float g = ((color >> 8) & 0xFF) / 255.0f;
	float b = (color & 0xFF) / 255.0f;
	beginTargetInfo.clear_color = {r, g, b, 1.0f};
}
void SDL3GPUBackend::BeginLayerDrawing()
{
	layerRenderTarget = CreateRenderTarget(renderTargetWidth, renderTargetHeight);
	if (!layerRenderTarget) return;
	if (renderPass) {SDL_EndGPURenderPass(renderPass);} // Can't have multiple render passes at the same time...
	
	SDL_GPUColorTargetInfo layerTarInfo;
	SDL_zero(layerTarInfo);
	layerTarInfo.clear_color = {0.0f, 0.0f, 0.0f, 0.0f};
	layerTarInfo.store_op = SDL_GPU_STOREOP_STORE;
	layerTarInfo.load_op = SDL_GPU_LOADOP_CLEAR;
	layerTarInfo.texture = layerRenderTarget;
	renderPass = SDL_BeginGPURenderPass(cmdBuf, &layerTarInfo, 1, NULL);
}
void SDL3GPUBackend::EndLayerDrawing(int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance *effectInstance)
{
	SDL_EndGPURenderPass(renderPass);

	renderPass = SDL_BeginGPURenderPass(cmdBuf, &beginTargetInfo, 1, NULL);

	float textureSize[2] = {renderTargetWidth, renderTargetHeight};
	std::string usePipeline = "INTERNAL_normal";
	switch (effect) {
		case 9:
			usePipeline = "INTERNAL_add";
			break;
		case 11:
			usePipeline = "INTERNAL_subtract";
			break;
		default: break;
	}
	if (effectInstance != nullptr) {
		usePipeline = "shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/thirdparty/" + effectInstance->filename;
		if (!pipelinesToUse[usePipeline]) {
			std::string fmt = "shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/thirdparty/" + effectInstance->filename + ".frag";
			CreatePipeline(fmt, usePipeline);
		}
	}
	if (!pipelinesToUse[usePipeline]) return;
	SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse[usePipeline]);
	SDL_GPUTextureSamplerBinding textureSampler = {layerRenderTarget, samplers[0]};
	SDL_BindGPUFragmentSamplers(renderPass, 0, &textureSampler, 1);
	ApplyEffectParameters(textureSize, effectInstance, rgbCoefficient, effect, effectParameter);
	RenderQuad(0, 0, renderTargetWidth, renderTargetHeight);
	drawingLayer = false;
}
void SDL3GPUBackend::LoadTexture(int id)
{
	if (textures.find(id) != textures.end()) {
		return;
	}

	auto imageInfo = ImageBank::Instance().GetImage(id);
	if (!imageInfo) {
		backend->GetPlatform()->Log("ImageBank::GetImage Error: Image with id " + std::to_string(id) + " not found");
		return;
	}

	char imageFileName[32];
	std::snprintf(imageFileName, sizeof(imageFileName), "images/%d.png", id);
	
	if (!backend->platform) return;
	std::vector<uint8_t> data = backend->GetPlatform()->GetPakFile().GetData(imageFileName);
	if (data.empty()) {
		backend->GetPlatform()->Log("PakFile::GetData Error: Image " + std::string(imageFileName) + " not found");
		return;
	}

	SDL_IOStream* stream = SDL_IOFromMem(data.data(), data.size());
	SDL_Surface* surface = IMG_Load_IO(stream, true);
	if (surface == nullptr) {
		backend->GetPlatform()->Log("IMG_Load_IO Error: " + std::string(SDL_GetError()));
		return;
	}
	
	SDL_Surface* rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
	SDL_DestroySurface(surface);
	surface = nullptr;
	if (rgbaSurface == nullptr) {
		backend->GetPlatform()->Log("SDL_ConvertSurface Error: " + std::string(SDL_GetError()));
		return;
	}
	SDL_FlipSurface(rgbaSurface, SDL_FLIP_VERTICAL);
	SDL_GPUTextureCreateInfo textureInfo{};
	textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	textureInfo.width = rgbaSurface->w;
	textureInfo.height = rgbaSurface->h;
	textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
	textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	textureInfo.num_levels = 1;
	textureInfo.layer_count_or_depth = 1;
	SDL_GPUTexture* texture = SDL_CreateGPUTexture(gpuDevice, &textureInfo);
	if (!texture) {
		backend->GetPlatform()->Log("SDL_CreateGPUTexture Error : " + std::string(SDL_GetError()));
		return;
	}
	GPU_TextureSize textureSize = {rgbaSurface->w, rgbaSurface->h};
	textureSizeMap.emplace(id, textureSize);
	std::string IdToString = std::to_string(id);
	SDL_SetGPUTextureName(gpuDevice, texture, IdToString.c_str());
	SDL_GPUTransferBufferCreateInfo bufferInfo{};
	bufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	bufferInfo.size = rgbaSurface->w * rgbaSurface->h * 4;
	SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &bufferInfo);
	Uint8* textureTransferPtr = (Uint8*)SDL_MapGPUTransferBuffer(gpuDevice, bufferTransferBuffer, false);
	SDL_memcpy(textureTransferPtr, rgbaSurface->pixels, rgbaSurface->w * rgbaSurface->h * 4);
	SDL_UnmapGPUTransferBuffer(gpuDevice, bufferTransferBuffer);
	SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);
	SDL_GPUTextureTransferInfo textureTransferInfo{};
	textureTransferInfo.offset = 0;
	textureTransferInfo.transfer_buffer = bufferTransferBuffer;
	SDL_GPUTextureRegion textureRegion{};
	textureRegion.texture = texture;
	textureRegion.w = rgbaSurface->w;
	textureRegion.h = rgbaSurface->h;
	textureRegion.d = 1;
	SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
	SDL_DestroySurface(rgbaSurface);
	rgbaSurface = nullptr;
	SDL_ReleaseGPUTransferBuffer(gpuDevice, bufferTransferBuffer);
	bufferTransferBuffer = nullptr;
	textures.emplace(id, texture);
}
void SDL3GPUBackend::UnloadTexture(int id)
{
	auto it = textures.find(id);
	if (it == textures.end() || !it->second) return;
	SDL_ReleaseGPUTexture(gpuDevice, it->second);
	it->second = nullptr;
	textures.erase(id);
}
void SDL3GPUBackend::DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance *effectInstance)
{
	x += 1;
	y += 1;
	auto imageInfo = ImageBank::Instance().GetImage(id);
	if (!imageInfo) {
		return;
	}
	
	auto texIt = textures.find(id);
	if (texIt == textures.end()) {
		backend->GetPlatform()->Log("Can't find Texture ID : " +  std::to_string(id));
		return;
	}
	SDL_GPUTexture* gpuTexture = texIt->second;

	if (effect == 1) color = 0xFFFFFF;

	float drawX = static_cast<float>(x) - (static_cast<float>(offsetX) * scaleX);
	float drawY = static_cast<float>(y) - (static_cast<float>(offsetY) * scaleY);
	float width = (static_cast<float>(imageInfo->Width) * scaleX) / 2.0f;
	float height = (static_cast<float>(imageInfo->Height) * scaleY) / 2.0f;
	float drawAngle = static_cast<float>(360 - angle);
	std::string usePipeline = "INTERNAL_normal";
	switch (effect) {
		case 9:
			usePipeline = "INTERNAL_add";
			break;
		case 11:
			usePipeline = "INTERNAL_subtract";
			break;
		default: break;
	}
	if (effectInstance != nullptr) {
		usePipeline = "shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/thirdparty/" + effectInstance->filename;
		if (!pipelinesToUse[usePipeline]) {
			std::string fmt = "shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/thirdparty/" + effectInstance->filename + ".frag";
			CreatePipeline(fmt, usePipeline);
		}
	}
	if (!pipelinesToUse[usePipeline]) return;
	SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse[usePipeline]);
	SDL_GPUTextureSamplerBinding textureBindings;
	textureBindings.texture = gpuTexture;
	textureBindings.sampler = samplers[0];
	SDL_GPUBufferBinding vertBufBinding{vertexBuffer, 0}, indexBufBinding{indexBuffer, 0};
	SDL_BindGPUVertexBuffers(renderPass, 0, &vertBufBinding, 1);
	SDL_BindGPUIndexBuffer(renderPass, &indexBufBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
	SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBindings, 1);
	float rad = drawAngle * (3.14159265358979323846f / 180.0f);
	float cosA = cosf(rad);
	float sinA = sinf(rad);
	float pivotX = static_cast<float>(offsetX) * scaleX;
	float pivotY = static_cast<float>(offsetY) * scaleY;
	float transform[16] =
	{
		width * cosA,width * sinA,0,0,
		-height * sinA,height * cosA,0,0,
		0,0,1,0,
		x + pivotX * (1 - cosA) + pivotY * sinA,y + pivotY * (1 - cosA) - pivotX * sinA,0,1
	};
	float orthoW = static_cast<float>(renderTargetWidth);
	float orthoH = static_cast<float>(renderTargetHeight);
	
	float mvp[16] = {
		2.0f / orthoW * transform[0], -2.0f / orthoH * transform[1], 0.0f, 0.0f,
		2.0f / orthoW * transform[4], -2.0f / orthoH * transform[5], 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		2.0f / orthoW * transform[12] - 1.0f, -2.0f / orthoH * transform[13] + 1.0f, 0.0f, 1.0f
	};
	SDL_PushGPUVertexUniformData(cmdBuf, 0, mvp, sizeof(mvp));
	int textureSize[2];
	GetGPUTextureSize(id, textureSize[0], textureSize[1]);
	float textureSizeFloat[2] = {textureSize[0], textureSize[1]};
	ApplyEffectParameters(textureSizeFloat, effectInstance, color, effect, effectParameter);
	SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
}
void SDL3GPUBackend::DrawQuickBackdrop(int x, int y, int width, int height, Shape *shape)
{
	if (shape->ShapeType == 1) { // Line
		int x1 = shape->FlipX ? width - 1 : 0;
		int y1 = shape->FlipY ? height - 1 : 0;
		int x2 = shape->FlipX ? 0 : width - 1;
		int y2 = shape->FlipY ? 0 : height - 1;
		Bitmap line(width, height);
		line.DrawLine(x1, y1, x2, y2, shape->BorderColor | 0xFF000000);
		DrawBitmap(line, x, y);
	}
	else {
		if (shape->FillType == 1) { // Solid Color
			SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse["INTERNAL_normal"]);
			ShaderUBO shaderUBO{};
			
			shaderUBO.circleClip = shape->ShapeType == 3;
			shaderUBO.Color[0] = ((shape->Color1 >> 16) & 0xFF) / 255.0f;
			shaderUBO.Color[1] = ((shape->Color1 >> 8) & 0xFF) / 255.0f;
			shaderUBO.Color[2] = (shape->Color1 & 0xFF) / 255.0f;
			shaderUBO.Color[3] = 1.0f;
			SDL_PushGPUFragmentUniformData(cmdBuf, 0, &shaderUBO, sizeof(shaderUBO));
			SDL_PushGPUFragmentUniformData(cmdBuf, 1, (int*)14, sizeof(int));
			RenderQuad(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
		}
		else if (shape->FillType == 2) { // Gradient
			float r1 = ((shape->Color1 >> 16) & 0xFF) / 255.0f;
			float g1 = ((shape->Color1 >> 8) & 0xFF) / 255.0f;
			float b1 = (shape->Color1 & 0xFF) / 255.0f;
			float r2 = ((shape->Color2 >> 16) & 0xFF) / 255.0f;
			float g2 = ((shape->Color2 >> 8) & 0xFF) / 255.0f;
			float b2 = (shape->Color2 & 0xFF) / 255.0f;
			
			ShaderUBO shaderUBO{};
			shaderUBO.gColor1[0] = r1;
			shaderUBO.gColor1[1] = g1;
			shaderUBO.gColor1[2] = b1;
			shaderUBO.gColor1[3] = 1.0f;

			shaderUBO.gColor2[0] = r2;
			shaderUBO.gColor2[1] = g2;
			shaderUBO.gColor2[2] = b2;
			shaderUBO.gColor2[3] = 1.0f;
			SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse["INTERNAL_normal"]);
			SDL_PushGPUFragmentUniformData(cmdBuf, 0, &shaderUBO, sizeof(shaderUBO));
			SDL_PushGPUFragmentUniformData(cmdBuf, 1, (int*)13, sizeof(int));
			RenderQuad(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
		}
		else if (shape->FillType == 3) { // Motif
			auto texIt = textures.find(shape->Image);
			if (texIt == textures.end()) return;
			SDL_GPUTexture* texture = texIt->second;

			SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse["INTERNAL_normal"]);
			SDL_GPUTextureSamplerBinding textureBinding = {texture, samplers[0]};
			SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBinding, 1);
			SDL_PushGPUFragmentUniformData(cmdBuf, 1, (int*)15, sizeof(int));
			ShaderUBO shaderUBO;
			shaderUBO.circleClip = shape->ShapeType == 3;
			shaderUBO.Color[0] = 1.0f;
			shaderUBO.Color[1] = 1.0f;
			shaderUBO.Color[2] = 1.0f;
			shaderUBO.Color[3] = 1.0f;
			int textureSize[2];
			GetGPUTextureSize(shape->Image, textureSize[0], textureSize[1]);
			int tw = std::max(1, textureSize[0]);
			int th = std::max(1, textureSize[1]);
			shaderUBO.uTileScale[0] = static_cast<float>(width) / static_cast<float>(tw);
			shaderUBO.uTileScale[1] = static_cast<float>(height) / static_cast<float>(th);
			SDL_PushGPUFragmentUniformData(cmdBuf, 1, &shaderUBO, sizeof(shaderUBO));
			float transform[16] = {
				width,width,0,0,
				-height,height,0,0,
				0,0,1,0,
				x * (1),y * (1),0,1
			};
			float orthoW = static_cast<float>(renderTargetWidth);
			float orthoH = static_cast<float>(renderTargetHeight);
			
			float mvp[16] = {
				2.0f / orthoW * transform[0], -2.0f / orthoH * transform[1], 0.0f, 0.0f,
				2.0f / orthoW * transform[4], -2.0f / orthoH * transform[5], 0.0f, 0.0f,
				0.0f, 0.0f, -1.0f, 0.0f,
				2.0f / orthoW * transform[12] - 1.0f, -2.0f / orthoH * transform[13] + 1.0f, 0.0f, 1.0f
			};
			SDL_PushGPUVertexUniformData(cmdBuf, 1, mvp, sizeof(mvp));
			SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
		}
		if (shape->BorderSize > 0) {
			Bitmap border(width, height);
			for (int i = 0; i < shape->BorderSize; i++) {
				int innerW = width - i * 2;
				int innerH = height - i * 2;
				if (innerW <= 0 || innerH <= 0) break;
				if (shape->ShapeType == 2) border.DrawRectangleLines(i, i, innerW, innerH, shape->BorderColor | 0xFF000000);
				else if (shape->ShapeType == 3 && shape->FillType > 1) border.DrawEllipseLines(i, i, innerW, innerH, shape->BorderColor | 0xFF000000);
			}
			DrawBitmap(border, x, y);
		}
	}
}
void SDL3GPUBackend::DrawCounterBar(int x, int y, Counter *counter)
{
	if (counter->Width <= 0 || counter->Height <= 0)
	{
		return;
	}

	bool isVertical = counter->DisplayType == 2;

	float fillPercent = counter->MaxValue > 0 ? static_cast<float>(counter->GetValue()) / static_cast<float>(counter->MaxValue) : 0.0f;

	int fillWidth = counter->Width;
	int fillHeight = counter->Height;
	int fillX = x;
	int fillY = y;

	if (isVertical)
	{
		fillHeight = static_cast<int>(counter->Height * fillPercent);
		if (counter->BarDirection == 1)
		{
			fillY = y + (counter->Height - fillHeight);
		}
	}
	else
	{
		fillWidth = static_cast<int>(counter->Width * fillPercent);
		if (counter->BarDirection == 1)
		{
			fillX = x + (counter->Width - fillWidth);
		}
	}

	int color1 = counter->shape.Color1;
	int color2 = counter->shape.FillType == 2 ? counter->shape.Color2 : color1;

	if (counter->BarDirection)
	{
		int temp = color1;
		color1 = color2;
		color2 = temp;
	}

	float r1 = ((color1 >> 16) & 0xFF) / 255.0f;
	float g1 = ((color1 >> 8) & 0xFF) / 255.0f;
	float b1 = (color1 & 0xFF) / 255.0f;

	float r2 = ((color2 >> 16) & 0xFF) / 255.0f;
	float g2 = ((color2 >> 8) & 0xFF) / 255.0f;
	float b2 = (color2 & 0xFF) / 255.0f;

	float u0 = 0.0f;
	float v0 = 0.0f;
	float u1 = 1.0f;
	float v1 = 1.0f;

	if (isVertical)
	{
		float fillRatio = counter->Height > 0 ? static_cast<float>(fillHeight) / static_cast<float>(counter->Height) : 0.0f;
		if (counter->BarDirection == 1)
		{
			v0 = 1.0f - fillRatio;
		}
		else
		{
			v1 = fillRatio;
		}
	}
	else
	{
		float fillRatio = counter->Width > 0 ? static_cast<float>(fillWidth) / static_cast<float>(counter->Width) : 0.0f;
		if (counter->BarDirection == 1)
		{
			u0 = 1.0f - fillRatio;
		}
		else
		{
			u1 = fillRatio;
		}
	}
	SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse["INTERNAL_normal"]);
	ShaderUBO shaderUBO{};
	shaderUBO.circleClip = 0;
	shaderUBO.gColor1[0] = r1; shaderUBO.gColor1[1] = g1; shaderUBO.gColor1[2] = b1; shaderUBO.gColor1[3] = 1.0f;
	shaderUBO.gColor2[0] = r2; shaderUBO.gColor2[1] = g2; shaderUBO.gColor2[2] = b2; shaderUBO.gColor2[3] = 1.0f;
	SDL_PushGPUFragmentUniformData(cmdBuf, 0, &shaderUBO, sizeof(shaderUBO));
	SDL_PushGPUFragmentUniformData(cmdBuf, 1, (int*)13, sizeof(int));
	RenderQuad(static_cast<float>(fillX), static_cast<float>(fillY), static_cast<float>(fillWidth), static_cast<float>(fillHeight), 0.0f, 0.0f, 0.0f, u0, v0, u1, v1);
}
void SDL3GPUBackend::DrawBitmap(Bitmap &bitmap, int x, int y)
{
	if (bitmapStorage.find(&bitmap) == bitmapStorage.end()) {
		SDL_GPUTextureCreateInfo gpuTextureInfo{};
		gpuTextureInfo.format = SDL_GPU_TEXTUREFORMAT_R32G32B32A32_UINT;
		gpuTextureInfo.height = bitmap.GetHeight();
		gpuTextureInfo.width = bitmap.GetWidth();
		gpuTextureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		gpuTextureInfo.type = SDL_GPU_TEXTURETYPE_2D;
		gpuTextureInfo.num_levels = 1;
		gpuTextureInfo.layer_count_or_depth = 1;
		SDL_GPUTexture* bitmapTexture = SDL_CreateGPUTexture(gpuDevice, &gpuTextureInfo);
		SDL_GPUTransferBufferCreateInfo bufferInfo{};
		bufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		bufferInfo.size = bitmap.GetWidth() * bitmap.GetHeight() * 4;
		SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &bufferInfo);
		Uint8* textureTransferPtr = (Uint8*)SDL_MapGPUTransferBuffer(gpuDevice, bufferTransferBuffer, false);
		SDL_memcpy(textureTransferPtr, bitmap.GetData(), bitmap.GetWidth() * bitmap.GetHeight() * 4);
		SDL_UnmapGPUTransferBuffer(gpuDevice, bufferTransferBuffer);
		SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);
		SDL_GPUTextureTransferInfo textureTransferInfo{};
		textureTransferInfo.offset = 0;
		textureTransferInfo.transfer_buffer = bufferTransferBuffer;
		SDL_GPUTextureRegion textureRegion{};
		textureRegion.texture = bitmapTexture;
		textureRegion.w = bitmap.GetWidth();
		textureRegion.h = bitmap.GetHeight();
		textureRegion.d = 1;
		SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
		SDL_ReleaseGPUTransferBuffer(gpuDevice, bufferTransferBuffer);
		bitmapStorage.emplace(&bitmap, bitmapTexture);
	}
	float transform[16] = {
		bitmap.GetWidth(),bitmap.GetWidth(),0,0,
		-bitmap.GetHeight(),bitmap.GetHeight(),0,0,
		0,0,1,0,
		x * (1),y * (1),0,1
	};
	float orthoW = static_cast<float>(renderTargetWidth);
	float orthoH = static_cast<float>(renderTargetHeight);	
	float mvp[16] = {
		2.0f / orthoW * transform[0], -2.0f / orthoH * transform[1], 0.0f, 0.0f,
		2.0f / orthoW * transform[4], -2.0f / orthoH * transform[5], 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		2.0f / orthoW * transform[12] - 1.0f, -2.0f / orthoH * transform[13] + 1.0f, 0.0f, 1.0f
	};
	ShaderUBO shaderUBO{};
	SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse["INTERNAL_normal"]);
	SDL_PushGPUFragmentUniformData(cmdBuf, 0, &shaderUBO, sizeof(shaderUBO));
	SDL_PushGPUFragmentUniformData(cmdBuf, 1, 0, sizeof(int));
	SDL_PushGPUVertexUniformData(cmdBuf, 0, mvp, sizeof(mvp));
	SDL_GPUTextureSamplerBinding samplerBinding = {bitmapStorage.find(&bitmap)->second, samplers[0]};
	SDL_BindGPUFragmentSamplers(renderPass, 0, &samplerBinding, 1);

	SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
}
void SDL3GPUBackend::DrawEffectRect(int x, int y, int width, int height, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance *effectInstance)
{
	/*if (renderPass) SDL_EndGPURenderPass(renderPass);
	if (width <= 0 || height <= 0)
	{
		return;
	}

	int srcX = std::max(0, x);
	int srcY = std::max(0, y);
	int srcW = std::min(width, static_cast<int>(renderTargetWidth - srcX));
	int srcH = std::min(height, static_cast<int>(renderTargetHeight - srcY));
	float coordScaleX = 1.0f;
	float coordScaleY = 1.0f;
	if (srcW <= 0 || srcH <= 0)
	{
		return;
	}
	int pixelSrcX = static_cast<int>(std::lround(srcX * coordScaleX));
	int pixelSrcY = static_cast<int>(std::lround(srcY * coordScaleY));
	int pixelSrcW = static_cast<int>(std::lround(srcW * coordScaleX));
	int pixelSrcH = static_cast<int>(std::lround(srcH * coordScaleY));
	pixelSrcW = std::min(pixelSrcW, static_cast<int>(renderTargetWidth - pixelSrcX));
	pixelSrcH = std::min(pixelSrcH, static_cast<int>(renderTargetHeight - pixelSrcY));
	if (pixelSrcW <= 0 || pixelSrcH <= 0)
	{
		return;
	}
	std::string usePipeline = "INTERNAL_normal";
	switch (effect) {
		case 9:
			usePipeline = "INTERNAL_add";
			break;
		case 11:
			usePipeline = "INTERNAL_subtract";
			break;
		default: break;
	}
	if (effectInstance != nullptr) {
		usePipeline = "shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/thirdparty/" + effectInstance->filename;
		if (!pipelinesToUse[usePipeline]) {
			std::string fmt = "shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/thirdparty/" + effectInstance->filename + ".frag";
			CreatePipeline(fmt, usePipeline);
		}
	}
	if (!pipelinesToUse[usePipeline]) return;
	SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse[usePipeline]);
	ApplyEffectParameters(NULL, effectInstance, rgbCoefficient, effect, effectParameter);
	RenderQuad(static_cast<float>(srcX), static_cast<float>(srcY), static_cast<float>(srcW), static_cast<float>(srcH), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
FIGURE OUT HOW TO DO THIS LATER    */
}
void SDL3GPUBackend::ApplyEffectParameters(float textureSize[2], EffectInstance *effectInstance, int rgbCoefficient, int effect, unsigned char effectParameter)
{
	float r = ((rgbCoefficient >> 16) & 0xFF) / 255.0f;
	float g = ((rgbCoefficient >> 8) & 0xFF) / 255.0f;
	float b = (rgbCoefficient & 0xFF) / 255.0f;
	float a = (255 - effectParameter) / 255.0f;
	ShaderUBO shaderUBO = {
		{},
		{},
		{r,g,b,a},
		0,
		0,
		{}
	};
	if (effectInstance != nullptr) {
		Uint32 uboLength = 0;
		bool useImageSize = false;
		for (auto param : effectInstance->Parameters) {
			if (param.Type == 0 || param.Type == 1) uboLength += sizeof(float); // int and float have same sizeof value sooooo -indeednotfunny
			else if (param.Type == 2) uboLength += sizeof(float[4]);
		}
		if (uboLength < 4) { // basically must have at least a parameter to push the data in -indeednotfunny
			void* shaderFragmentBuffer = operator new(uboLength);
			Uint32 offset = 0;
			for (auto param : effectInstance->Parameters) {
				if (param.Type == 0) { // int
					int integerParam = std::get<int>(param.Value);
					SDL_memcpy(static_cast<char*>(shaderFragmentBuffer) + offset, &integerParam, sizeof(int));
					offset += sizeof(int);
				}
				else if (param.Type == 1) { // float
					float floatParam = std::get<float>(param.Value);
					SDL_memcpy(static_cast<char*>(shaderFragmentBuffer) + offset, &floatParam, sizeof(float));
					offset += sizeof(float);
				}
				else if (param.Type == 2) { // color
					int c = std::get<int>(param.Value);
					float pr = (c & 0xFF) / 255.0f;
					float pg = ((c >> 8) & 0xFF) / 255.0f;
					float pb = ((c >> 16) & 0xFF) / 255.0f;
					float colorParam[4] = {pr, pg, pb, 1.0f};
					SDL_memcpy(static_cast<char*>(shaderFragmentBuffer) + offset, &colorParam, sizeof(float[4]));
					offset += sizeof(float[4]);
				}
			}
			SDL_PushGPUFragmentUniformData(cmdBuf, 0, shaderUBO.Color, sizeof(shaderUBO.Color));
			SDL_PushGPUFragmentUniformData(cmdBuf, 1, shaderFragmentBuffer, uboLength);
			SDL_PushGPUFragmentUniformData(cmdBuf, 2, textureSize, sizeof(textureSize));
			operator delete(shaderFragmentBuffer);
			shaderFragmentBuffer = nullptr;
		}
	}
	int shaderMode = effect;
	if (effect < 0 || effect > 15) shaderMode = 0; // Out of standard effect range in normal.frag
	if (effectInstance == nullptr) {
		SDL_PushGPUFragmentUniformData(cmdBuf, 1, &shaderMode, sizeof(shaderMode));
		SDL_PushGPUFragmentUniformData(cmdBuf, 0, &shaderUBO, sizeof(shaderUBO));
	}
}
void SDL3GPUBackend::GetGPUTextureSize(int id, int &width, int &height)
{
	auto textureSize = textureSizeMap.find(id);
	if (textureSize == textureSizeMap.end()) return;
	width = textureSize->second.width;
	height = textureSize->second.height;
}
void SDL3GPUBackend::UpdateBuffer(float u0, float v0, float u1, float v1)
{
	SDL_GPUTransferBufferCreateInfo transferBufferInfo{};
	transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	transferBufferInfo.size = (sizeof(Vertex) * 4) + (sizeof(Uint16) * 6);
	SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferBufferInfo);
	Vertex* transferData = (Vertex*)SDL_MapGPUTransferBuffer(gpuDevice, transferBuffer, false);
	//								POSITION		  UV			  COLOR
	transferData[0] = (Vertex){-1.0f, 1.0f, 0.0f, u0, v0, 1.0f, 1.0f, 1.0f, 1.0f};
	transferData[1] = (Vertex){1.0f, 1.0f, 0.0f, u1, v0, 1.0f, 1.0f, 1.0f, 1.0f};
	transferData[2] = (Vertex){1.0f, -1.0f, 0.0f, u1, v1, 1.0f, 1.0f, 1.0f, 1.0f};
	transferData[3] = (Vertex){-1.0f, -1.0f, 0.0f, u0, v1, 1.0f, 1.0f, 1.0f, 1.0f};
	Uint16* indexData = (Uint16*) &transferData[4];
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 0;
	indexData[4] = 2;
	indexData[5] = 3;
	SDL_UnmapGPUTransferBuffer(gpuDevice, transferBuffer);
	SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);
	SDL_GPUTransferBufferLocation transferBufLoc{};
	transferBufLoc.transfer_buffer = transferBuffer;
	transferBufLoc.offset = 0;
	SDL_GPUTransferBufferLocation transferBufLoc1{};
	transferBufLoc1.transfer_buffer = transferBuffer;
	transferBufLoc1.offset = sizeof(Vertex) * 4;
	SDL_GPUBufferRegion vertexBufReg{}, indexBufReg{};
	vertexBufReg.buffer = vertexBuffer;
	vertexBufReg.offset = 0;
	vertexBufReg.size = sizeof(Vertex) * 4;
	indexBufReg.buffer = indexBuffer;
	indexBufReg.offset = 0;
	indexBufReg.size = sizeof(Uint16) * 6;
	SDL_UploadToGPUBuffer(copyPass, &transferBufLoc, &vertexBufReg, false);
	SDL_UploadToGPUBuffer(copyPass, &transferBufLoc1, &indexBufReg, false);
	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
	SDL_WaitForGPUIdle(gpuDevice);
	SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);
	transferBuffer = nullptr;
}
void SDL3GPUBackend::RenderQuad(float x, float y, float w, float h, float angle, float pivotX, float pivotY, float u0, float v0, float u1, float v1)
{
	float drawAngle = angle - 360;
	float rad = drawAngle * (SDL_PI_F / 180.0f);
	float sinA = SDL_sinf(rad), cosA = SDL_cosf(rad);
	float transform[16] =
	{
		w * cosA,w * sinA,0,0,
		-h * sinA,h * cosA,0,0,
		0,0,1,0,
		x + pivotX * (1 - cosA) + pivotY * sinA,y + pivotY * (1 - cosA) - pivotX * sinA,0,1
	};
	float orthoW = static_cast<float>(renderTargetWidth);
	float orthoH = static_cast<float>(renderTargetHeight);
	
	float mvp[16] = {
		2.0f / orthoW * transform[0], -2.0f / orthoH * transform[1], 0.0f, 0.0f,
		2.0f / orthoW * transform[4], -2.0f / orthoH * transform[5], 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		2.0f / orthoW * transform[12] - 1.0f, -2.0f / orthoH * transform[13] + 1.0f, 0.0f, 1.0f
	};
	if (u0 != 0 && u1 != 0 && v0 != 0 && v1 != 0) UpdateBuffer(u0, v0, u1, v1);
	SDL_PushGPUVertexUniformData(cmdBuf, 0, mvp, sizeof(mvp));
	SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
	if (u0 != 0 && u1 != 0 && v0 != 0 && v1 != 0) UpdateBuffer(); // Reset buffer for possible images that will render
}
void SDL3GPUBackend::LoadFont(int id)
{
	//check if font already exists
	if (fonts.find(id) != fonts.end()) {
		return;
	}

	//get font info
	FontInfo* fontInfo = FontBank::Instance().GetFont(id);
	if (fontInfo == nullptr) {
		backend->GetPlatform()->Log("FontBank::GetFont Error: Font with id " + std::to_string(id) + " not found");
		return;
	}

	SDL_IOStream* stream;

	//if buffer is already loaded, use it
	if (fontBuffers.find(fontInfo->FontFileName) != fontBuffers.end()) {
		stream = SDL_IOFromMem(fontBuffers[fontInfo->FontFileName]->data(), fontBuffers[fontInfo->FontFileName]->size());
	}
	else {
		//load buffer from pak file
		if (!backend->platform) return;
		std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(backend->platform->GetPakFile().GetData("fonts/" + fontInfo->FontFileName));
		if (buffer->empty()) {
			backend->GetPlatform()->Log("PakFile::GetData Error: Font with file name " + fontInfo->FontFileName + " not found");
			return;
		}
		stream = SDL_IOFromMem(buffer->data(), buffer->size());
		fontBuffers[fontInfo->FontFileName] = buffer;
	}

	TTF_Font* font = TTF_OpenFontIO(stream, true, static_cast<float>(fontInfo->Height));
	if (!font) {
		backend->GetPlatform()->Log("TTF_OpenFontIO Error: " + std::string(SDL_GetError()));
		return;
	}
	
	//render flags
	int renderFlags = TTF_STYLE_NORMAL;
	if (fontInfo->Weight > 400) {
		renderFlags |= TTF_STYLE_BOLD;
	}
	if (fontInfo->Italic) {
		renderFlags |= TTF_STYLE_ITALIC;
	}
	if (fontInfo->Underline) {
		renderFlags |= TTF_STYLE_UNDERLINE;
	}
	if (fontInfo->Strikeout) {
		renderFlags |= TTF_STYLE_STRIKETHROUGH;
	}	

	TTF_SetFontStyle(font, renderFlags);

	fonts[id] = font;
}
SDL_FRect SDL3GPUBackend::CalculateRenderTargetRect()
{
	// get actual current window size
	int currentWindowWidth, currentWindowHeight;
	SDL_GetWindowSize(window, &currentWindowWidth, &currentWindowHeight);
	
	// get app size
	int renderTargetWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	int renderTargetHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);

	SDL_FRect rect = { 0.0f, 0.0f, static_cast<float>(renderTargetWidth), static_cast<float>(renderTargetHeight) };

	if (Application::Instance().GetAppData()->GetResizeDisplay()) {
		rect.w = static_cast<float>(currentWindowWidth);
		rect.h = static_cast<float>(currentWindowHeight);

		if (Application::Instance().GetAppData()->GetFitInside()) {
			//keeps the aspect ratio of the application and fits inside the window while staying in the center
			float aspectRatio = static_cast<float>(renderTargetWidth) / static_cast<float>(renderTargetHeight);
			if (rect.w / rect.h > aspectRatio) {
				rect.w = rect.h * aspectRatio;
			}
			else {
				rect.h = rect.w / aspectRatio;
			}
			rect.x = static_cast<float>((currentWindowWidth - static_cast<int>(rect.w)) / 2);
			rect.y = static_cast<float>((currentWindowHeight - static_cast<int>(rect.h)) / 2);
		}
	}
	else if (!Application::Instance().GetAppData()->GetDontCenterFrame()) {
		rect.x = static_cast<float>((currentWindowWidth - static_cast<int>(rect.w)) / 2);
		rect.y = static_cast<float>((currentWindowHeight - static_cast<int>(rect.h)) / 2);
	}
	
	return rect;
}
void SDL3GPUBackend::UnloadFont(int id)
{
	auto it = fonts.find(id);
	if (it != fonts.end()) {
		// Find the FontInfo associated with this font id to remove buffer
		FontInfo* fontInfo = FontBank::Instance().GetFont(id);
		if (fontInfo != nullptr) {
			// Check if any other loaded font is using the same buffer
			bool bufferUsedByOtherFont = false;
			for (const auto& pair : fonts) {
				if (pair.first != id) {
					FontInfo* otherFontInfo = FontBank::Instance().GetFont(pair.first);
					if (otherFontInfo && otherFontInfo->FontFileName == fontInfo->FontFileName) {
						bufferUsedByOtherFont = true;
						break;
					}
				}
			}
			if (!bufferUsedByOtherFont) {
				fontBuffers.erase(fontInfo->FontFileName);
			}
		}
		
		ClearTextCacheForFont(id);
		
		TTF_CloseFont(it->second);
		fonts.erase(it);
	}
}
void SDL3GPUBackend::DrawText(FontInfo *fontInfo, int x, int y, int width, int height, unsigned char horizontalAlignment, unsigned char verticalAlignment, int color, const std::string &text, int objectHandle, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance *effectInstance)
{
	if (fontInfo == nullptr)
	{
		return;
	}

	if (fonts.find(fontInfo->Handle) == fonts.end())
	{
		return;
	}

	TTF_Font *font = fonts[fontInfo->Handle];
	if (font == nullptr)
	{
		return;
	}

	TextCacheKey cacheKey{fontInfo->Handle, text, color, objectHandle};
	auto cacheIt = textCache.find(cacheKey);
	SDL_GPUTexture* texture;
	int textureWidth;
	int textureHeight;
	if (cacheIt != textCache.end())
	{
		texture = cacheIt->second->texture;
		textureWidth = cacheIt->second->width;
		textureHeight = cacheIt->second->height;
	}
	else {
		if (objectHandle != -1) {
			auto it = textCache.begin();
			while (it != textCache.end()) {
				if (it->first.objectHandle == objectHandle) {
					if (it->second->texture != 0) {
						SDL_ReleaseGPUTexture(gpuDevice, it->second->texture);
						it->second->texture = nullptr;
					}
					it = textCache.erase(it);
				}
				else {
					++it;
				}
			}
		}
		std::string modifiedText = text;
		modifiedText.erase(std::remove(modifiedText.begin(), modifiedText.end(), '\r'), modifiedText.end());

		for (size_t i = 0; i < modifiedText.size(); i++) {
			if (modifiedText[i] == '\t') {
				modifiedText.replace(i, 1, "    ");
			}
		}
		if (modifiedText.find_first_not_of(" \n\r\t") == std::string::npos) {
			return;
		}
		TTF_SetFontWrapAlignment(font, (TTF_HorizontalAlignment)horizontalAlignment);
		SDL_Surface *surface = TTF_RenderText_Blended_Wrapped(font, modifiedText.c_str(), 0, RGBToSDLColor(color), width);
		if (surface == nullptr) {
			backend->GetPlatform()->Log("TTF_RenderText_Blended_Wrapped Error: " + std::string(SDL_GetError()));
			return;
		}
		SDL_Surface *rgbaSurface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(surface);
		surface = nullptr;
		if (rgbaSurface == nullptr) {
			backend->GetPlatform()->Log("SDL_ConvertSurface Error: " + std::string(SDL_GetError()));
			return;
		}
		SDL_FlipSurface(rgbaSurface, SDL_FLIP_VERTICAL);
		textureWidth = rgbaSurface->w;
		textureHeight = rgbaSurface->h;
		SDL_GPUTextureCreateInfo textureInfo{};
		textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		textureInfo.width = rgbaSurface->w;
		textureInfo.height = rgbaSurface->h;
		textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
		textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
		textureInfo.num_levels = 1;
		textureInfo.layer_count_or_depth = 1;
		texture = SDL_CreateGPUTexture(gpuDevice, &textureInfo);
		SDL_GPUTransferBufferCreateInfo bufferInfo{};
		bufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
		bufferInfo.size = rgbaSurface->w * rgbaSurface->h * 4;
		SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &bufferInfo);
		Uint8* textureTransferPtr = (Uint8*)SDL_MapGPUTransferBuffer(gpuDevice, bufferTransferBuffer, false);
		SDL_memcpy(textureTransferPtr, rgbaSurface->pixels, rgbaSurface->w * rgbaSurface->h * 4);
		SDL_UnmapGPUTransferBuffer(gpuDevice, bufferTransferBuffer);
		SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(gpuDevice);
		SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);
		SDL_GPUTextureTransferInfo textureTransferInfo{};
		textureTransferInfo.offset = 0;
		textureTransferInfo.transfer_buffer = bufferTransferBuffer;
		SDL_GPUTextureRegion textureRegion{};
		textureRegion.texture = texture;
		textureRegion.w = rgbaSurface->w;
		textureRegion.h = rgbaSurface->h;
		textureRegion.d = 1;
		SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
		SDL_EndGPUCopyPass(copyPass);
		SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
		SDL_DestroySurface(rgbaSurface);
		rgbaSurface = nullptr;
		SDL_ReleaseGPUTransferBuffer(gpuDevice, bufferTransferBuffer);
		bufferTransferBuffer = nullptr;
		
		if (textCache.size() >= 256) RemoveOldTextCache();
		CachedText* cached = new CachedText();
		cached->texture = texture;
		cached->width = textureWidth;
		cached->height = textureHeight;
		textCache[cacheKey] = cached;
	}
	if (verticalAlignment == 1) y += (height - textureHeight) / 2;
	else if (verticalAlignment == 2) y += height - textureHeight;
	
	float textureSizes[2] = {static_cast<float>(textureWidth), static_cast<float>(textureHeight)};
	std::string usePipeline = "INTERNAL_normal";
	switch (effect) {
		case 9:
			usePipeline = "INTERNAL_add";
			break;
		case 11:
			usePipeline = "INTERNAL_subtract";
			break;
		default: break;
	}
	if (effectInstance != nullptr) {
		usePipeline = "shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/thirdparty/" + effectInstance->filename;
		if (!pipelinesToUse[usePipeline]) {
			std::string fmt = "shaders/" + std::string(SDL_GetGPUDeviceDriver(gpuDevice)) + "/thirdparty/" + effectInstance->filename + ".frag";
			CreatePipeline(fmt, usePipeline);
		}
	}
	if (!pipelinesToUse[usePipeline]) return;
	SDL_BindGPUGraphicsPipeline(renderPass, pipelinesToUse[usePipeline]);
	SDL_GPUTextureSamplerBinding textureBindings;
	textureBindings.texture = texture;
	textureBindings.sampler = samplers[0];
	SDL_GPUBufferBinding vertBufBinding{vertexBuffer, 0}, indexBufBinding{indexBuffer, 0};
	SDL_BindGPUVertexBuffers(renderPass, 0, &vertBufBinding, 1);
	SDL_BindGPUIndexBuffer(renderPass, &indexBufBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
	SDL_BindGPUFragmentSamplers(renderPass, 0, &textureBindings, 1);
	float transform[16] =
	{
		width,width,0,0,
		-height,height,0,0,
		0,0,1,0,
		x * (1),y * (1),0,1
	};
	float orthoW = static_cast<float>(renderTargetWidth);
	float orthoH = static_cast<float>(renderTargetHeight);
	
	float mvp[16] = {
		2.0f / orthoW * transform[0], -2.0f / orthoH * transform[1], 0.0f, 0.0f,
		2.0f / orthoW * transform[4], -2.0f / orthoH * transform[5], 0.0f, 0.0f,
		0.0f, 0.0f, -1.0f, 0.0f,
		2.0f / orthoW * transform[12] - 1.0f, -2.0f / orthoH * transform[13] + 1.0f, 0.0f, 1.0f
	};
	SDL_PushGPUVertexUniformData(cmdBuf, 0, mvp, sizeof(mvp));
	ApplyEffectParameters(textureSizes, effectInstance, rgbCoefficient, effect, effectParameter);
	SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
}
SDL_Color SDL3GPUBackend::RGBToSDLColor(int color)
{
	return SDL_Color{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		255
	};
}
void SDL3GPUBackend::RemoveOldTextCache()
{
	if (textCache.empty()) {
		return;
	}

	auto oldestIt = textCache.begin();
	if (oldestIt->second->texture) {
		SDL_ReleaseGPUTexture(gpuDevice, oldestIt->second->texture);
		oldestIt->second->texture = nullptr;
	}
	textCache.erase(oldestIt);
}
void SDL3GPUBackend::ClearTextCacheForFont(int fontHandle)
{
	auto it = textCache.begin();
	while (it != textCache.end()) {
		if (it->first.fontHandle == fontHandle) {
			if (it->second->texture) {
				SDL_ReleaseGPUTexture(gpuDevice, it->second->texture);
				it->second->texture = nullptr;
			}
			it = textCache.erase(it);
		} else {
			++it;
		}
	}
}
SDL_GPUShader *SDL3GPUBackend::LoadShader(std::string path, unsigned int samplers, unsigned int storageTextures, unsigned int storageBuffers, unsigned int uniformBuffers)
{
	SDL_GPUShaderCreateInfo shaderCreateInfo{};
	if (SDL_GetGPUShaderFormats(gpuDevice) & SDL_GPU_SHADERFORMAT_MSL) {
		shaderCreateInfo.format = SDL_GPU_SHADERFORMAT_MSL;
		path += ".msl";
		shaderCreateInfo.entrypoint = "main0";
	}
	else if (SDL_GetGPUShaderFormats(gpuDevice) & SDL_GPU_SHADERFORMAT_DXIL) {
		shaderCreateInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
		path += ".dxil";
	}
	else if (SDL_GetGPUShaderFormats(gpuDevice) & SDL_GPU_SHADERFORMAT_SPIRV) {
		path += ".spirv";
		shaderCreateInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
	}
	std::vector<Uint8> data = backend->GetPlatform()->GetPakFile().GetData(path);
	if (!data.data()) {
		backend->GetPlatform()->Log("Bad Data: " + path); // bad. -indeednotfunny
		return nullptr;
	}
	
	shaderCreateInfo.code = data.data();
	shaderCreateInfo.code_size = data.size();
	shaderCreateInfo.stage = (SDL_strstr(path.c_str(), ".vert")) ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
	shaderCreateInfo.num_samplers = samplers;
	shaderCreateInfo.num_storage_textures = storageTextures;
	shaderCreateInfo.num_storage_buffers = storageBuffers;
	shaderCreateInfo.num_uniform_buffers = uniformBuffers;
	shaderCreateInfo.entrypoint = "main";

	SDL_GPUShader* shader = SDL_CreateGPUShader(gpuDevice, &shaderCreateInfo);
	if (!shader) {
		backend->GetPlatform()->Log("SDL_CreateGPUShader Error: " + std::string(SDL_GetError()));
		return nullptr;
	}
	backend->GetPlatform()->Log("Created Shader : " + path);
    return shader;
}
void SDL3GPUBackend::CreatePipeline(std::string fragShader, std::string pipelineName)
{
	auto it = pipelinesToUse.find(fragShader);
	if (it != pipelinesToUse.end()) return;
	SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo{};
	SDL_GPUShader* fragmentShader = LoadShader(fragShader, 1, 0, 0, 2);
	if (!fragmentShader) {return;}
	pipelineCreateInfo.vertex_shader = defaultVertexShader;
	pipelineCreateInfo.fragment_shader = fragmentShader;
	SDL_GPUColorTargetDescription colTargDesc{};
	pipelineCreateInfo.target_info.num_color_targets = 1;
	SDL_GPUColorTargetBlendState blendState = {};
	blendState.enable_blend = true;
	if (SDL_strcmp(pipelineName.c_str(), "INTERNAL_subtract") == 0) {
		blendState.color_blend_op = SDL_GPU_BLENDOP_SUBTRACT;
		blendState.alpha_blend_op = SDL_GPU_BLENDOP_SUBTRACT;
		blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
	}
	else {
		blendState.color_blend_op = SDL_GPU_BLENDOP_ADD;
		blendState.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
		if (SDL_strcmp(pipelineName.c_str(), "INTERNAL_add") == 0 || SDL_strcmp(pipelineName.c_str(), "INTERNAL_subtract") == 0) {
			blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
			blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
			blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
			blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
		}
		else {
			blendState.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
			blendState.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
			blendState.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
			blendState.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		}
	}
	colTargDesc.blend_state = blendState;
	colTargDesc.format = swapChainFormat;
	pipelineCreateInfo.target_info.color_target_descriptions = &colTargDesc;
	SDL_GPUVertexInputState gpuVertexInputState{};
	gpuVertexInputState.num_vertex_buffers = 1;
	SDL_GPUVertexBufferDescription gpuVertexBufferDesc{};
	gpuVertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	gpuVertexBufferDesc.instance_step_rate = 0;
	gpuVertexBufferDesc.slot = 0;
	gpuVertexBufferDesc.pitch = sizeof(Vertex);
	SDL_GPUVertexAttribute vertexAttributes[3]{};
	// Position
	vertexAttributes[0].buffer_slot = 0;
	vertexAttributes[0].location = 0;
	vertexAttributes[0].offset = 0;
	vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	// UV
	vertexAttributes[1].buffer_slot = 0;
	vertexAttributes[1].location = 1;
	vertexAttributes[1].offset = offsetof(Vertex, texCoord);
	vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
	// Color
	vertexAttributes[2].buffer_slot = 0;
	vertexAttributes[2].location = 2;
	vertexAttributes[2].offset = offsetof(Vertex, color);
	vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
	gpuVertexInputState.num_vertex_attributes = 3;
	gpuVertexInputState.vertex_buffer_descriptions = &gpuVertexBufferDesc;
	gpuVertexInputState.vertex_attributes = vertexAttributes;
	pipelineCreateInfo.vertex_input_state = gpuVertexInputState;
	pipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	backend->GetPlatform()->Log("Creating Pipeline...");
	SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineCreateInfo);
	if (!pipeline) {
		backend->GetPlatform()->Log("SDL_CreateGPUGraphicsPipeline Error : " + std::string(SDL_GetError()));
		return;
	}
	backend->GetPlatform()->Log("Pipeline created!");
	SDL_ReleaseGPUShader(gpuDevice, fragmentShader);
	fragmentShader = nullptr;
    pipelinesToUse[pipelineName] = pipeline;
	backend->GetPlatform()->Log("Created GPU Pipeline : " + pipelineName);
}
SDL_GPUTexture* SDL3GPUBackend::CreateRenderTarget(int width, int height)
{
	renderTargetWidth = width;
	renderTargetHeight = height;
	if (width < 0 || height < 0) return nullptr;
	SDL_GPUTextureCreateInfo renderTargetInfo;
	renderTargetInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	renderTargetInfo.type = SDL_GPU_TEXTURETYPE_2D;
	renderTargetInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
	renderTargetInfo.width = width;
	renderTargetInfo.height = height;
	renderTargetInfo.num_levels = 1;
	renderTargetInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
	renderTargetInfo.layer_count_or_depth = 1;
	backend->GetPlatform()->Log("Creating Render Target....");
	SDL_GPUTexture* renderTarget = SDL_CreateGPUTexture(gpuDevice, &renderTargetInfo);
	if (!renderTarget) backend->GetPlatform()->Log("Render Target Texture failed : " + std::string(SDL_GetError()));
	else backend->GetPlatform()->Log("Render Target created.");
	return renderTarget;
}
#endif