#pragma once
#ifdef NUCLEAR_BACKEND_SDL3
#include "GraphicsBackend.h"
#include <vector>
#include <unordered_map>
#include <set>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
class SDL3Backend;
typedef struct Vertex {
	float position[3]{};
	float texCoord[2]{};
	float color[4]{};
} Vertex;
typedef struct ShaderUBO {
	float gColor1[4]{};
	float gColor2[4]{};
	float Color[4]{1.0f, 1.0f, 1.0f, 1.0f};
	int circleClip = 0;
	int uVertical = 0;
	float uTileScale[2]{};
} ShaderUBO;
typedef struct GPU_TextureSize {
	int width = 0, height = 0;
} GPU_TextureSize;
class SDL3GPUBackend : public GraphicsBackend {
public:
	void Initialize() override;
	void Deinitialize() override;

	void SetBackend(SDL3Backend* b) { backend = b; }

	void BeginDrawing() override;
	void EndDrawing() override;
	void Clear(int color) override;

	void BeginLayerDrawing() override;
	void EndLayerDrawing(int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance) override;
	void LoadTexture(int id) override;
	void UnloadTexture(int id) override;
	void DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance = nullptr) override;
	void DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape) override;
	void DrawCounterBar(int x, int y, Counter* counter);
	void DrawBitmap(Bitmap& bitmap, int x, int y) override;
	void DrawEffectRect(int x, int y, int width, int height, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance) override;
	void RenderQuad(float x, float y, float w, float h, float angle = 0.0f, float pivotX = 0.0f, float pivotY = 0.0f, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);

	void LoadFont(int id) override;
	void UnloadFont(int id) override;
	void DrawText(FontInfo* fontInfo, int x, int y, int width, int height, unsigned char horizontalAlignment, unsigned char verticalAlignment, int color, const std::string& text, int objectHandle = -1, int rgbCoefficient = 0xFFFFFF, int effect = 0, unsigned char effectParameter = 0, EffectInstance* effectInstance = nullptr) override;
	SDL_FRect CalculateRenderTargetRect();
	SDL_Window* GetSDLWindow() {return window;}

private:
	void ApplyEffectParameters(float textureSize[2], EffectInstance* effectInstance, int rgbCoefficient, int effect, unsigned char effectParameter);
	void GetGPUTextureSize(int id, int& width, int& height);
	void UpdateBuffer(float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);
    SDL3Backend* backend = nullptr;
    SDL_Window* window = nullptr;
    SDL_GPUDevice* gpuDevice = nullptr;
	SDL_GPUCommandBuffer* cmdBuf = nullptr;
	SDL_GPUTexture* renderTarget = nullptr;
	SDL_GPUTexture* layerRenderTarget = nullptr;
	SDL_GPUColorTargetInfo beginTargetInfo{};
	SDL_GPURenderPass* renderPass = nullptr;
	SDL_GPUBuffer* vertexBuffer = nullptr;
	SDL_GPUBuffer* indexBuffer = nullptr;
	bool drawingLayer = false;
	float renderTargetWidth = 0.0f, renderTargetHeight = 0.0f;
	std::unordered_map<int, TTF_Font*> fonts;
	std::unordered_map<std::string, std::shared_ptr<std::vector<uint8_t>>> fontBuffers;
	std::unordered_map<Bitmap*, SDL_GPUTexture*> bitmapStorage;
	struct TextCacheKey {
		unsigned int fontHandle;
		std::string text;
		int color;
		int objectHandle;
		
		bool operator==(const TextCacheKey& other) const {
			return fontHandle == other.fontHandle && text == other.text && color == other.color && objectHandle == other.objectHandle;
		}
	};
	
	struct TextCacheKeyHash {
		std::size_t operator()(const TextCacheKey& key) const {
			std::size_t h1 = std::hash<unsigned int>{}(key.fontHandle);
			std::size_t h2 = std::hash<std::string>{}(key.text);
			std::size_t h3 = std::hash<int>{}(key.color);
			std::size_t h4 = std::hash<int>{}(key.objectHandle);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
		}
	};
	struct CachedText {
		SDL_GPUTexture* texture = nullptr;
		int width = 0;
		int height = 0;
	};
	std::unordered_map<TextCacheKey, CachedText*, TextCacheKeyHash> textCache;
	SDL_Color RGBToSDLColor(int color);
	void RemoveOldTextCache();
	void ClearTextCacheForFont(int fontHandle);
	std::unordered_map<std::string, SDL_GPUGraphicsPipeline*> pipelinesToUse{};
	std::unordered_map<int, GPU_TextureSize> textureSizeMap;
	std::unordered_map<int, SDL_GPUTexture*> textures{};
	bool renderedFirstFrame = false;
    SDL_GPUShader* LoadShader(std::string path, unsigned int samplers, unsigned int storageTextures, unsigned int storageBuffers, unsigned int uniformBuffers);
	SDL_GPUShader* defaultVertexShader = nullptr;
	void CreatePipeline(std::string fragShader, std::string pipelineName);
	SDL_GPUTextureFormat swapChainFormat;
	SDL_GPUTexture* CreateRenderTarget(int width, int height);
	SDL_GPUSampler* samplers[2]{};
};
#endif