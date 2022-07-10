#pragma once

#include "scene.h"
#include "editor.h"

namespace app
{
	struct ViewInfo
	{
		glm::ivec2 size{ 0, 0 };
		float dpiScale = 1.f;
	};
	ViewInfo GetViewInfo();

	struct TimeInfo
	{
		float dt = 0.f;
	};
	TimeInfo GetTimeInfo();
	Scene* GetScene();
	Editor* GetEditor();
	Render* GetRender();

	std::string PlatformGetFile(const std::string& name);
}