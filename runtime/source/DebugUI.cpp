#include "DebugUI.h"

#ifdef _DEBUG

#include <SDL3/SDL.h>
#include <cstdio>
#include <chrono>

// Dear ImGui includes
#include "imgui.h"
#ifdef NUCLEAR_BACKEND_SDL3
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#elif NUCLEAR_BACKEND_SDL2
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#endif
void DebugUI::Initialize(SDL_Window* window, SDL_Renderer* renderer) {
	if (initialized) {
		return;
	}

	this->window = window;
	this->renderer = renderer;

	IMGUI_CHECKVERSION();
	context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	#ifdef NUCLEAR_BACKEND_SDL3
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);
	#elif NUCLEAR_BACKEND_SDL2
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);
	#endif
	initialized = true;
}

void DebugUI::Shutdown() {
	if (!initialized) {
		return;
	}
	#ifdef NUCLEAR_BACKEND_SDL3
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	#elif NUCLEAR_BACKEND_SDL2
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	#endif
	ImGui::DestroyContext(context);
	context = nullptr;

	initialized = false;
}

void DebugUI::BeginFrame() {
	if (!initialized || !enabled) {
		return;
	}

	static auto lastFrameTime = std::chrono::high_resolution_clock::now();
	auto currentFrameTime = std::chrono::high_resolution_clock::now();
	frameTime = std::chrono::duration<float, std::chrono::seconds::period>(currentFrameTime - lastFrameTime).count();
	lastFrameTime = currentFrameTime;
	
	fps = 1.0f / frameTime;
	#ifdef NUCLEAR_BACKEND_SDL3
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	#elif NUCLEAR_BACKEND_SDL2
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	#endif
	ImGui::NewFrame();
}

void DebugUI::EndFrame() {
	if (!initialized || !enabled) {
		return;
	}

	RenderWindows();
	RenderMetrics();

	ImGui::Render();
	#ifdef NUCLEAR_BACKEND_SDL3
	ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
	#elif NUCLEAR_BACKEND_SDL2
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
	#endif
}

void DebugUI::AddWindow(const std::string& name, std::function<void()> renderFunction) {
	DebugWindow window;
	window.name = name;
	window.renderFunction = renderFunction;
	window.open = true;
	
	windows.push_back(window);
}

void DebugUI::RenderWindows() {
	for (auto& window : windows) {
		if (window.open) {
			if (ImGui::Begin(window.name.c_str(), &window.open)) {
				window.renderFunction();
			}
			ImGui::End();
		}
	}
}

void DebugUI::RenderMetrics() {
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(170, 80), ImGuiCond_FirstUseEver);
	
	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove);
	ImGui::Text("Frame Time: %.3f ms", frameTime * 1000.0f);
	ImGui::Text("FPS: %.1f", fps);
	ImGui::End();
}

#endif