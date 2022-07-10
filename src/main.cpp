#include <stdio.h>

#include "main.h"

#ifdef _WIN32
#include <SDL.h>
#include <glad/gles2.h>
#elif defined __EMSCRIPTEN__
#include <SDL2/SDL.h>
#include <GLES3/gl3.h>
#endif

#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl.h>

#include "editor.h"
#include "render.h"
#include "utils.h"
#include "scene.h"

namespace app
{
	struct AppData
	{
		SDL_Window* window = nullptr;
		SDL_GLContext glContext{};
		glm::ivec2 viewSize{};
		float dpiScale = 1.f;

		bool isRunning = false;

		std::unique_ptr<Editor, CustomDeleter<Editor, EditorDeinit>> editor = nullptr;
		std::unique_ptr<Render, CustomDeleter<Render, RenderDeinit>> render = nullptr;
		std::unique_ptr<Scene> scene = nullptr;

		Uint64 hpFrequency = 0;
		Uint64 hpCounterPrev = 0;
		float dt = 0.f;
	};

	AppData* App = nullptr;

	ViewInfo GetViewInfo()
	{
		ViewInfo info{};
		info.size = App->viewSize;
		info.dpiScale = App->dpiScale;
		return info;
	}
	TimeInfo GetTimeInfo()
	{
		TimeInfo info{};
		info.dt = App->dt;
		return info;
	}
	Editor* GetEditor()
	{
		return App->editor.get();
	}
	Scene* GetScene()
	{
		return App->scene.get();
	}
	Render* GetRender()
	{
		return App->render.get();
	}

	void Init(SDL_Window* window, SDL_GLContext glContext, glm::ivec2 viewSize, float dpiScale)
	{
		std::srand(uint32_t(std::time(nullptr)));
		App = new AppData();
		{
			App->window = window;
			App->glContext = glContext;
			App->viewSize = viewSize;
			App->dpiScale = dpiScale;
		}
		{
			App->editor.reset(EditorInit());
			App->render.reset(RenderInit());
			App->scene.reset(new Scene());
			RenderSetShaderContent(App->render.get(), App->scene->GetShaderContent());
		}
		{
		}
		{
			App->isRunning = true;
			App->hpFrequency = SDL_GetPerformanceFrequency();
			App->hpCounterPrev = SDL_GetPerformanceCounter();
		}
	}

	void Deinit()
	{
		delete App;
	}

	bool IsRunning()
	{
		return App->isRunning;
	}

	void MainLoop()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				App->isRunning = false;
			}
			ImGui_ImplSDL2_ProcessEvent(&event);
		}
		{
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
		}
		{
			EditorFrame(App->editor.get());
			RenderSetResolution(App->render.get(), EditorGetViewportResolution(App->editor.get()));
			EditorSetRenderOutput(App->editor.get(), RenderGetFinalImage(App->render.get()));
			RenderFrame(App->render.get());
		}
		{
			glViewport(0, 0, App->viewSize.x, App->viewSize.y);
			glClear(GL_COLOR_BUFFER_BIT);
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
		SDL_GL_SwapWindow(App->window);

		auto hpCounterNew = SDL_GetPerformanceCounter();
		auto hpDiff = hpCounterNew - App->hpCounterPrev;
		App->dt = float(hpDiff / double(App->hpFrequency));
		App->hpCounterPrev = hpCounterNew;
	}
}