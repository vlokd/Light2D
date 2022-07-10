#include <emscripten.h>
#include <emscripten/html5.h>
#include <SDL2/SDL.h>
#include <GLES3/gl3.h>

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl.h"

#include "main.h"

EM_JS(int, GetCanvasWidth, (), {
  return Module.canvas.width;
});

EM_JS(int, GetCanvasHeight, (), {
  return Module.canvas.height;
});

EM_JS(float, GetPixelRatio, (), {
  return window.devicePixelRatio;
});

EM_JS(void, ResizeCanvas, (), {
  js_resizeCanvas();
});

namespace app
{
	void Init(SDL_Window* window, SDL_GLContext glContext, glm::ivec2 viewSize, float dpiScale);
	bool IsRunning();
	void MainLoop();
	void Deinit();

	static std::unordered_map<std::string, std::string> EmbeddedFiles =
	{
	#include "embedded/textfiles.cpp"
	};

	std::string PlatformGetFile(const std::string & name)
	{
		if (EmbeddedFiles.contains(name))
		{
			return EmbeddedFiles.at(name);
		}
		else
		{
			return {};
		}
	}
}

int main(int argc, char** argv)
{
    ResizeCanvas();
	glm::ivec2 viewSize{};
	viewSize.x = GetCanvasWidth();
	viewSize.y = GetCanvasHeight();
	float dpi = GetPixelRatio();

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_InitSubSystem(SDL_INIT_VIDEO);
    auto* window = SDL_CreateWindow(
        "App", 0, 0, 
		viewSize.x, viewSize.y,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    SDL_GL_GetDrawableSize(window, &viewSize.x, &viewSize.y);
	auto glContext = SDL_GL_CreateContext(window);

	auto emGlContext = emscripten_webgl_get_current_context();
	{
		emscripten_webgl_enable_extension(emGlContext, "EXT_color_buffer_float");
	}

	app::Init(window, glContext, viewSize, dpi);
    ImGui_ImplOpenGL3_Init();
    ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	SDL_GL_SetSwapInterval(1);

    emscripten_set_main_loop(app::MainLoop, 0, true);

	app::Deinit();
	ImGui_ImplSDL2_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

    return 0;
}