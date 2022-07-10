#include "main.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <SDL.h>
#include <glad/gles2.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <proj_imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>

namespace app
{
	void Init(SDL_Window* window, SDL_GLContext glContext, glm::ivec2 viewSize, float dpiScale);
	bool IsRunning();
	void MainLoop();
	void Deinit();

#ifndef NDEBUG

	void GlDebugCallback(GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar* message,
		const void* userParam)
	{
		if (severity != GL_DEBUG_SEVERITY_NOTIFICATION_KHR)
		{
			SDL_TriggerBreakpoint();
		}
	}
#endif

	std::string PlatformGetFile(const std::string& name)
	{
		std::string res{};
		std::string fullPath = "./res/" + name;
		if (auto* file = std::fopen(fullPath.c_str(), "rb"))
		{
			fseek(file, 0, SEEK_END);
			auto maxSize = ftell(file);
			auto* buff = new char[maxSize + 1];
			memset(buff, '\0', maxSize + 1);
			fseek(file, 0, SEEK_SET);
			fread(buff, sizeof(char), maxSize, file);
			res = std::string(buff);
			delete[] buff;
			fclose(file);
		}
		return res;
	}
}

int main(int argc, char** argv)
{
	SetProcessDPIAware();
	{
		SDL_InitSubSystem(SDL_INIT_VIDEO);
		SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	}
	glm::ivec2 viewSize{};
	{
		SDL_DisplayMode dm = {};
		SDL_GetCurrentDisplayMode(0, &dm);
		viewSize.x = dm.w;
		viewSize.y = dm.h;
	}

	constexpr float ddpi = 96.f;
	float dpi = ddpi;
	SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr);
	dpi = dpi / ddpi;

	auto* window = SDL_CreateWindow(
		"App", 0, 0,
		viewSize.x, viewSize.y,
		SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_GL_GetDrawableSize(window, &viewSize.x, &viewSize.y);

	auto glContext = SDL_GL_CreateContext(window);
	gladLoadGLES2((GLADloadfunc)SDL_GL_GetProcAddress);
	SDL_GL_GetDrawableSize(window, &viewSize.x, &viewSize.y);

#ifndef NDEBUG
	glEnable(GL_DEBUG_OUTPUT_KHR);
	glDebugMessageCallbackKHR(app::GlDebugCallback, nullptr);
#endif

	app::Init(window, glContext, viewSize, dpi);
	ImGui_ImplOpenGL3_Init();
	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	SDL_GL_SetSwapInterval(1);

	SDL_ShowWindow(window);
	while (app::IsRunning())
	{
		app::MainLoop();
	}

	app::Deinit();
	ImGui_ImplSDL2_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}