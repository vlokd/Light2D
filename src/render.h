#pragma once

#include "scene_change.h"

namespace app
{
	struct Render;
	Render* RenderInit();
	void RenderFrame(Render* render);
	void RenderDeinit(Render* render);

	struct RenderTextureHandle
	{
		unsigned int data;
	};
	RenderTextureHandle RenderGetFinalImage(Render* render);
	void RenderSetResolution(Render* render, glm::ivec2 resolution);
	void RenderSetShaderContent(Render* render, const std::string& content);
	void RenderInvalidateIntegration(Render* render);
	SceneChange RenderOnEditor(Render* render);

	float RenderGetExposure(const Render* render);
	void RenderSetExposure(Render* render, float value);

#ifdef PROJECT_BUILD_DEV
	std::string RenderGetShaderBuildErrors(const Render* render);
#endif

	enum class RenderStage
	{
		Red,
		Green,
		Blue,
		Common
	};

	struct UniformFillRequest;
	RenderStage GetRenderStage(UniformFillRequest* req);

	template<class T, size_t Components>
	const char* GetUniformTypeName();
	template<class T, size_t Components>
	extern void FillUniformV(UniformFillRequest* req, const std::string& name, size_t elements, const T* value);
	template<class T>
	void FillUniform(UniformFillRequest* req, const std::string& name, T value)
	{
		if constexpr (glm::type<T>::is_vec)
		{
			FillUniformV<typename T::value_type, glm::type<T>::components>(req, name, 1, glm::value_ptr(value));
		}
		else
		{
			FillUniformV<T, 1>(req, name, 1, &value);
		}
	}
}