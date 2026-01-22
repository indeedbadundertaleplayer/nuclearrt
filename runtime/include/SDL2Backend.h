#pragma once

#ifdef NUCLEAR_BACKEND_SDL2

#include "Backend.h"
#include <unordered_map>
#include <set>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#ifdef _DEBUG
#include "DebugUI.h"
#endif

#ifdef __SWITCH__
#define INPUT_TOUCH
#endif
typedef struct SampleFile {
	Uint8 *data = nullptr;
	Uint32 data_len = 0;
	SDL_AudioSpec spec{};
	std::string pathName = "";
} SampleFile;
typedef struct Channel {
	Uint8 *data = nullptr; // No need to make floats, as SDL_AudioStream does convert samples into a format you give via SDL_AudioSpec
	Uint32 data_len = 0;
	SDL_AudioSpec spec{};
	bool uninterruptable = false;
	SDL_AudioStream *stream = nullptr;
	int position = 0;
	bool lock = false;
	bool finished = false;
	int curHandle = -1;
	bool loop = false;
	bool pause = false;
	float volume = 1.0f;
	float pan = 0.0f;
	std::string name = "";
} Channel;
class SDL2Backend : public Backend {
public:
	SDL2Backend();
	~SDL2Backend() override;

	std::string GetName() const override { return "SDL2"; }

	void Initialize() override;
	void Deinitialize() override;

	bool ShouldQuit() override;

	std::string GetPlatformName() override;
	std::string GetAssetsFileName() override;

	void BeginDrawing() override;
	void EndDrawing() override;
	void Clear(int color) override;

	void LoadTexture(int id) override;
	void UnloadTexture(int id) override;
	void DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, int color, int effect, unsigned char effectParameter, float scaleY) override;
	void DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape) override;
	void CreateShader(std::string name, int numSamplers, int numUniformBuffers, const unsigned char* code, int codeSize) override {}
	void ClearShaders() override {}
	void BeginShaderDraw(std::string name) override {}
	void EndShaderDraw() override {}
	bool SetFragmentUniforms(std::string name, uint32_t slotIndex, const void *data, uint32_t length) override {return false;}
	void DrawBGTexture(int x, int y, int width, int height, float scaleX, float scaleY) override {}
	void DrawRectangle(int x, int y, int width, int height, int color) override;
	void DrawRectangleLines(int x, int y, int width, int height, int color) override;
	void DrawLine(int x1, int y1, int x2, int y2, int color) override;
	void DrawPixel(int x, int y, int color) override;

	void LoadFont(int id) override;
	void UnloadFont(int id) override;
	void DrawText(FontInfo* fontInfo, int x, int y, int color, const std::string& text, int objectHandle = -1) override;
	// Sample Start
	static void SDLCALL AudioCallback(void *userdata, Uint8 * stream, int len);
	bool LoadSample(int id, int channel) override {return false;}
	bool LoadSampleFile(std::string path) override {return false;}
	void PlaySample(int id, int channel, int loops, int freq, bool uninterruptable, float volume, float pan) override {}
	void PlaySampleFile(std::string path, int channel, int loops) override {}
	void DiscardSampleFile(std::string path) override {}
	void StopSample(int id, bool channel) override {}
	int FindSample(std::string name) override {return -1;}
	void SetSampleVolume(float volume, int id, bool channel) override {}
	void UpdateSample() override;
	int GetSampleVolume(int id, bool channel) override {return 0;}
	std::string GetChannelName(int channel) override {return "";}
	void LockChannel(int channel, bool unlock) override {}
	void SetSamplePan(float pan, int id, bool channel) override {}
	int GetSamplePan(int id, bool channel) override {return 0;}
	void SetSampleFreq(int freq, int id, bool channel) override {}
	int GetSampleFreq(int id, bool channel) override {return 0;}
	int GetSampleDuration(int id, bool channel) override {return 0;}
	int GetSamplePos(int id, bool channel) override {return 0;}
	void SetSamplePos(int pos, int id, bool channel) override {}
	void UpdateSample() override {}
	bool SampleState(int id, bool channel, bool pauseOrStop) override {return false;}
	// Sample End
	const uint8_t* GetKeyboardState() override;

	int GetMouseX() override;
	int GetMouseY() override;
	int GetMouseWheelMove() override;
	uint32_t GetMouseState() override;
	void HideMouseCursor() override;
	void ShowMouseCursor() override;

	unsigned int GetTicks() override { return SDL_GetTicks(); }
	float GetTimeDelta() override;
	void Delay(unsigned int ms) override;
	bool IsPixelTransparent(int textureId, int x, int y) override;
	void GetTextureDimensions(int textureId, int& width, int& height) override;
	const char* GetRendererName() {return SDL_GetCurrentVideoDriver();}
#ifdef _DEBUG
	void ToggleDebugUI() { DEBUG_UI.ToggleEnabled(); }
	bool IsDebugUIEnabled() { return DEBUG_UI.IsEnabled(); }
#endif

private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_AudioDeviceID audioDevice;
	SDL_AudioSpec spec;
	bool windowFocused = false;
	SDL_Colour RGBToSDLColor(int color);
	SDL_Colour RGBAToSDLColor(int color);
	std::unordered_map<int, SDL_Texture*> mosaics;
	std::unordered_map<int, int> imageToMosaic;
	std::unordered_map<int, std::set<int>> mosaicToImages;
	std::unordered_map<std::string, SampleFile> sampleFiles;
	Channel channels[49];
	std::unordered_map<int, TTF_Font*> fonts;
	std::unordered_map<std::string, std::shared_ptr<std::vector<uint8_t>>> fontBuffers;

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
		SDL_Texture* texture;
		int width;
		int height;
	};
	
	std::unordered_map<TextCacheKey, CachedText, TextCacheKeyHash> textCache;
	int FusionToSDLKey(short key);
	void RemoveOldTextCache();
	void ClearTextCacheForFont(int fontHandle);
	int FusionToSDLKey(short key);
#ifdef INPUT_TOUCH
	int touchX = 0;
	int touchY = 0;
	bool touchDown = false;
#endif
}; 

#endif