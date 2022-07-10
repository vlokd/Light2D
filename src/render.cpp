#include "render.h"

#ifdef _WIN32
#include <glad/gles2.h>
#elif defined __EMSCRIPTEN__
#include <GLES3/gl3.h>
#endif

#include "main.h"
#include "utils.h"

#include "imgui.h"

namespace app
{
	struct RenderTarget
	{
		GLuint texture;
		GLuint framebuffer;
	};
	struct Tile
	{
		glm::ivec2 origin{};
		glm::ivec2 size{};
	};
	struct TileGridInfo
	{
		glm::ivec2 tileCount{};
		glm::ivec2 padTileSize{};
		glm::ivec2 tileSize{};
	};
	struct Render
	{
		RenderTarget tracePreviewRT;
		RenderTarget traceRT;
		RenderTarget accumulateRT;
		RenderTarget presentRT;

		glm::ivec2 renderResolution{ 0, 0 };

		int maxRaysPerSample = 16;
		int samplesPerPixel = 4;
		float previewScale = 1.f / 4.f;
		float renderScale = 1.f;
		float rayHitDst = 1e-4f;
		float rayMissDst = 10.f;

		float exposure = 1.f;
		float gamma = 2.2f;

		int traceStepsCurrent = 0;
		int traceStepsTarget = 1024;

		glm::ivec2 tileSize{ 64, 64 };
		TileGridInfo tileInfo;
		int tilesRendered = 0;
#ifdef __EMSCRIPTEN__
		int tilesPerFrame = 20;
#else
		int tilesPerFrame = 30;
#endif
		int currentColor = 0;

		GLuint programTrace;
		GLuint programPresent;
		GLuint programAccumulate;

		bool needRebuildTargets = false;
		bool needRebuildTraceProgram = false;

		bool skipFrame = false;
		bool isInPreview = false;
		bool needClearTargets = false;

		std::string shaderContent;

#ifdef PROJECT_BUILD_DEV
		std::string shaderBuildErrors;
		FileWatch traceFragWatch{ "trace_frag.glsl" };
#endif
	};
	struct UniformFillRequest
	{
		GLuint program;
		RenderStage stage;
	};

	GLuint CompileShader(const std::string& src, GLenum type)
	{
		auto shader = glCreateShader(type);
		auto srcC = src.c_str();
		glShaderSource(shader, 1, &srcC, nullptr);
		glCompileShader(shader);
		return shader;
	}

	GLuint BuildShaderProgram(GLuint* target, GLuint vertexShader, GLuint fragmentShader)
	{
		if (target)
		{
			glDeleteProgram(*target);
		}
		auto program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragmentShader);
		glLinkProgram(program);

		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE)
		{
#ifdef PROJECT_BUILD_DEV
			GLint logLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
			std::vector<GLchar> infoLog(logLength);
			glGetProgramInfoLog(program, logLength, &logLength, &infoLog[0]);
			std::string logStr(infoLog.begin(), infoLog.end());

			if (auto* render = GetRender())
			{
				render->shaderBuildErrors += "\n" + logStr;;
			}
#endif
		}

		if (target)
		{
			*target = program;
		}

		return program;
	}

	GLuint BuildTexture(GLuint* target, glm::ivec2 dimensions, GLenum internalFormat, GLenum filter)
	{
		if (target)
		{
			glDeleteTextures(1, target);
		}

		GLuint tex = 0;

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		GLenum minFilter = GL_NEAREST_MIPMAP_NEAREST;
		if (filter == GL_LINEAR)
		{
			minFilter = GL_LINEAR_MIPMAP_NEAREST;
		}
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, dimensions.x, dimensions.y);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (target)
		{
			*target = tex;
		}

		return tex;
	}

	GLuint BuildFramebuffer(GLuint* target, const std::vector<GLuint>& textures)
	{
		if (target)
		{
			glDeleteFramebuffers(1, target);
		}

		GLuint fb = 0;

		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);
		for (int i = 0; i < int(textures.size()); ++i)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures[i], 0);
		}
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		if (target)
		{
			*target = fb;
		}

		return fb;
	}

	void BuildRenderTarget(RenderTarget& target, glm::ivec2 dimensions, GLenum internalFormat, GLenum filterMode)
	{
		BuildTexture(&target.texture, dimensions, internalFormat, filterMode);
		BuildFramebuffer(&target.framebuffer, { target.texture });
	}

	std::string PatchTraceShader(Render* render, std::string src)
	{
		static const char* ShaderContentStub = R"xxx(
TraceResult TraceScene(vec2 pt, vec2 dir)
{
    TraceResult res;
	res.dst = MAX_TRACE_DST;

    return res;
})xxx";

		ReplaceSubstr(src, "{codegen_scene}", render->shaderContent.empty() ? ShaderContentStub : render->shaderContent);
		ReplaceSubstr(src, "{codegen_uniforms}", "");
		ReplaceSubstr(src, "{codegen_samples_per_pixel}", std::to_string(render->samplesPerPixel));
		ReplaceSubstr(src, "{codegen_max_rays_per_sample}", std::to_string(render->maxRaysPerSample));
		ReplaceSubstr(src, "{codegen_miss_dst}", std::to_string(render->rayMissDst));
		ReplaceSubstr(src, "{codegen_hit_dst}", std::to_string(render->rayHitDst));
		return src;
	}

	Render* RenderInit()
	{
		auto* render = new Render();
#ifdef PROJECT_BUILD_DEV
		render->shaderBuildErrors.clear();
#endif
		render->needRebuildTargets = true;
		render->needRebuildTraceProgram = true;
		{
			auto fsQuadVertexSrc = PlatformGetFile("fsquad_vert.glsl");
			auto traceFragSrc = PlatformGetFile("trace_frag.glsl");
			auto presentFragSrc = PlatformGetFile("present_tex_frag.glsl");
			auto accumulateFragSrc = PlatformGetFile("accumulate_tex_frag.glsl");

			traceFragSrc = PatchTraceShader(render, traceFragSrc);

			auto fsQuad = CompileShader(fsQuadVertexSrc.c_str(), GL_VERTEX_SHADER);
			auto traceFrag = CompileShader(traceFragSrc.c_str(), GL_FRAGMENT_SHADER);
			auto presentFrag = CompileShader(presentFragSrc.c_str(), GL_FRAGMENT_SHADER);
			auto accFrag = CompileShader(accumulateFragSrc.c_str(), GL_FRAGMENT_SHADER);

			render->programTrace = BuildShaderProgram(nullptr, fsQuad, traceFrag);
			render->programPresent = BuildShaderProgram(nullptr, fsQuad, presentFrag);
			render->programAccumulate = BuildShaderProgram(nullptr, fsQuad, accFrag);

			glDeleteShader(fsQuad);
			glDeleteShader(traceFrag);
			glDeleteShader(presentFrag);
			glDeleteShader(accFrag);
		}
		RenderInvalidateIntegration(render);
		return render;
	}

	TileGridInfo GenerateTileGrid(glm::ivec2 resolution, glm::ivec2 tileSize)
	{
		TileGridInfo tileInfo;
		tileInfo.tileCount = resolution / tileSize;
		tileInfo.padTileSize = resolution - tileInfo.tileCount * tileSize;
		if (tileInfo.padTileSize.x > 0)
		{
			tileInfo.tileCount.x += 1;
		}
		if (tileInfo.padTileSize.y > 0)
		{
			tileInfo.tileCount.y += 1;
		}
		tileInfo.tileSize = tileSize;
		return tileInfo;
	}
	Tile GetTile(const TileGridInfo& tileInfo, int index)
	{
		Tile tile;
		glm::ivec2 tileCoord{0, 0};
		tileCoord.y = index / tileInfo.tileCount.x;
		tileCoord.x = index - tileCoord.y * tileInfo.tileCount.x;
		tile.origin = tileCoord * tileInfo.tileSize;
		tile.size = tileInfo.tileSize;
		if (((tileInfo.tileCount.x == 0) || (tileCoord.x == (tileInfo.tileCount.x - 1))) && tileInfo.padTileSize.x > 0)
		{
			tile.size.x = tileInfo.padTileSize.x;
		}
		if (((tileInfo.tileCount.y == 0) || (tileCoord.y == (tileInfo.tileCount.y - 1))) && tileInfo.padTileSize.y > 0)
		{
			tile.size.y = tileInfo.padTileSize.y;
		}
		return tile;
	}
	void RenderInvalidateIntegration(Render* render)
	{
		render->isInPreview = true;
		render->traceStepsCurrent = 0;
		render->tilesRendered = 0;
		render->needClearTargets = true;
		render->currentColor = 0;
	}
	void RenderFrame(Render* render)
	{
		if (render->renderResolution.x == 0 && render->renderResolution.y == 0)
		{
			return;
		}

		auto previewTextureSize = glm::vec2(render->renderResolution) * render->previewScale;
		auto renderTextureSize = glm::vec2(render->renderResolution) * render->renderScale;

#ifdef PROJECT_BUILD_DEV
		if (render->traceFragWatch.IsChanged())
		{
			render->needRebuildTraceProgram = true;
			render->traceFragWatch.MarkAsRead();
		}
#endif

		if (render-> needRebuildTargets)
		{
			BuildRenderTarget(render->traceRT, renderTextureSize, GL_RGBA32F, GL_LINEAR);
			BuildRenderTarget(render->tracePreviewRT, previewTextureSize, GL_RGBA32F, GL_LINEAR);
			BuildRenderTarget(render->accumulateRT, render->renderResolution, GL_RGBA32F, GL_LINEAR);
			BuildRenderTarget(render->presentRT, render->renderResolution, GL_RGBA8, GL_LINEAR);

			render->tileInfo = GenerateTileGrid(renderTextureSize, render->tileSize);
			RenderInvalidateIntegration(render);
			render->needRebuildTargets = false;
		}
		if (render->needRebuildTraceProgram)
		{
#ifdef PROJECT_BUILD_DEV
			GetRender()->shaderBuildErrors.clear();
#endif
			RenderInvalidateIntegration(render);
			auto fsQuadVertexSrc = PlatformGetFile("fsquad_vert.glsl");
			auto traceFragSrc = PlatformGetFile("trace_frag.glsl");
			traceFragSrc = PatchTraceShader(render, traceFragSrc);
			auto fsQuad = CompileShader(fsQuadVertexSrc.c_str(), GL_VERTEX_SHADER);
			auto traceFrag = CompileShader(traceFragSrc.c_str(), GL_FRAGMENT_SHADER);
			BuildShaderProgram(&render->programTrace, fsQuad, traceFrag);
			glDeleteShader(fsQuad);
			glDeleteShader(traceFrag);
			render->needRebuildTraceProgram = false;
		}

		bool isInPreview = render->isInPreview;
		auto& traceRT = isInPreview ? render->tracePreviewRT : render->traceRT;
		auto renderResolution = isInPreview ? previewTextureSize : renderTextureSize;

		int totalTileCount = render->tileInfo.tileCount.x * render->tileInfo.tileCount.y;
		int tilesRendered = render->tilesRendered;
		int tilesToRender = std::min(render->tilesPerFrame, totalTileCount - tilesRendered);
		int tileStartIdx = tilesRendered;
		int tileEndIdx = tileStartIdx + tilesToRender;

		bool presentAllowed = isInPreview;

		if (render->traceStepsCurrent < render->traceStepsTarget)
		{
			{
				glBindFramebuffer(GL_FRAMEBUFFER, traceRT.framebuffer);
				auto program = render-> programTrace;
				glUseProgram(program);
				{
					auto loc = glGetUniformLocation(program, "u_random_seed");
					float v = float(render->traceStepsCurrent) / render->traceStepsTarget;
					glUniform1f(loc, v);
				}
				{
					auto loc = glGetUniformLocation(program, "u_tex0_size");
					glUniform2f(loc, renderResolution.x, renderResolution.y);
				}
				UniformFillRequest req{};
				req.program = program;
				if (auto* scene = GetScene())
				{
					req.stage = RenderStage::Common;
					scene->FillShaderUniforms(&req);
				}
				if (render->needClearTargets)
				{
					glClearColor(0.f, 0.f, 0.f, 1.f);
					glClear(GL_COLOR_BUFFER_BIT);
					glDisable(GL_BLEND);
					render->needClearTargets = false;
				}
				else
				{
					glEnable(GL_BLEND);
					glBlendFunc(GL_ONE, GL_ONE);
				}
				if (isInPreview)
				{
					for (int i = 0; i < 3; ++i)
					{
						if (auto* scene = GetScene())
						{
							req.stage = RenderStage(i);
							scene->FillShaderUniforms(&req);
						}
						GLboolean colorMask[4] = { GL_FALSE, GL_FALSE, GL_FALSE , GL_FALSE };
						colorMask[i] = GL_TRUE;
						glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
						glViewport(0, 0, GLsizei(renderResolution.x), GLsizei(renderResolution.y));
						glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					}
				}
				else
				{
					int i = render->currentColor;
					if (auto* scene = GetScene())
					{
						req.stage = RenderStage(i);
						scene->FillShaderUniforms(&req);
					}
					GLboolean colorMask[4] = { GL_FALSE, GL_FALSE, GL_FALSE , GL_FALSE };
					colorMask[i] = GL_TRUE;
					glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
					for (int t = tileStartIdx; t < tileEndIdx; ++t)
					{
						auto tile = GetTile(render->tileInfo, t);
						glViewport(tile.origin.x, tile.origin.y, tile.size.x, tile.size.y);
						glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					}
				}
				if (isInPreview)
				{
					render->isInPreview = false;
					render->traceStepsCurrent = 0;
					render->currentColor = 0;
					render->needClearTargets = true;
				}
				else
				{
					render->currentColor++;
					if (render->currentColor == 3)
					{
						render->currentColor = 0;
						render->tilesRendered += tilesToRender;
						if (render->tilesRendered == totalTileCount)
						{
							render->traceStepsCurrent++;
							render->tilesRendered = 0;
							presentAllowed = true;
						}
					}
				}
			}
		}
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDisable(GL_BLEND);
		if (presentAllowed)
		{
			glViewport(0, 0, render->renderResolution.x, render->renderResolution.y);
			glBindFramebuffer(GL_FRAMEBUFFER, render->accumulateRT.framebuffer);
			glClearColor(1.f, 0.f, 1.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);
			auto program = render->programAccumulate;
			glUseProgram(program);
			{
				auto loc = glGetUniformLocation(program, "u_sample_count");
				glUniform1f(loc, float(std::max(render->traceStepsCurrent, 1)));
			}
			{
				auto loc = glGetUniformLocation(program, "u_tex0");
				glUniform1i(loc, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, traceRT.texture);
			}
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		if (true)
		{
			glViewport(0, 0, render->renderResolution.x, render->renderResolution.y);
			glBindFramebuffer(GL_FRAMEBUFFER, render->presentRT.framebuffer);
			glClearColor(1.f, 0.f, 1.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);
			auto program = render->programPresent;
			glUseProgram(program);
			{
				auto loc = glGetUniformLocation(program, "u_tex0");
				glUniform1i(loc, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, render->accumulateRT.texture);
				
			}
			{
				auto loc = glGetUniformLocation(program, "u_exposure");
				glUniform1f(loc, render->exposure);
			}
			{
				auto loc = glGetUniformLocation(program, "u_gamma");
				glUniform1f(loc, render->gamma);
			}
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		}
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}
	void RenderDeinit(Render* render)
	{
		glDeleteProgram(render->programTrace);
		glDeleteProgram(render->programPresent);
		std::vector<GLuint> textures = { render->traceRT.texture, render->tracePreviewRT.texture, render->presentRT.texture };
		std::vector<GLuint> fbos = { render->traceRT.framebuffer, render->tracePreviewRT.framebuffer, render->presentRT.framebuffer };
		glDeleteFramebuffers(GLsizei(fbos.size()), fbos.data());
		glDeleteTextures(GLsizei(textures.size()), textures.data());
		delete render;
	}
	RenderTextureHandle RenderGetFinalImage(Render* render)
	{
		RenderTextureHandle handle{};
		handle.data = render->presentRT.texture;
		return handle;
	}
	void RenderSetResolution(Render* render, glm::ivec2 resolution)
	{
		if (render->renderResolution != resolution)
		{
			render->needRebuildTargets = true;
		}
		render->renderResolution = resolution;
	}
	void RenderSetShaderContent(Render* render, const std::string& content)
	{
		render->shaderContent = content;
		render->needRebuildTraceProgram = true;
	}
	RenderStage GetRenderStage(UniformFillRequest* req)
	{
		return req->stage;
	}
#ifdef PROJECT_BUILD_DEV
	std::string RenderGetShaderBuildErrors(const Render* render)
	{
		return render->shaderBuildErrors;
	}
#endif
	SceneChange RenderOnEditor(Render* render)
	{
		SceneChange change = SceneChange::None;
		if (ImGui::BeginTabItem("Render"))
		{
			ImGui::DragFloat("Exposure", &render->exposure, 0.1f, 0.f, 1000.f, "%.6f");
			ImGui::DragFloat("Gamma", &render->gamma, 0.1f, 0.f, 1000.f, "%.6f");
			ImGui::DragInt("Tiles per frame", &render->tilesPerFrame);
			ImGui::Text("Steps: %d/%d", render->traceStepsCurrent, render->traceStepsTarget);
			ImGui::EndTabItem();
		}
		return change;
	}
	float RenderGetExposure(const Render* render)
	{
		return render->exposure;
	}
	void RenderSetExposure(Render* render, float value)
	{
		render->exposure = value;
	}

#define XXX_GL_UNIFORM_TYPE(X) \
	X(float, 1, f, float) \
	X(float, 2, f, vec2) \
	X(float, 3, f, vec3) \
	X(float, 4, f, vec4) \
	X(int, 1, i, int) \
	X(int, 2, i, ivec2) \
	X(int, 3, i, ivec3) \
	X(int, 4, i, ivec4) \

	template<class T, size_t Components>
	const char* GetUniformTypeName()
	{
#define XXX_GL_IMPL_FN(Type, ComponentCount, GLPrefix, UniformType) \
		if constexpr (std::is_same_v<T, Type> && Components == ComponentCount) { return #UniformType ; }
		XXX_GL_UNIFORM_TYPE(XXX_GL_IMPL_FN)
		static_assert("GetUniformTypeName: Unsupported type");
	}
#undef XXX_GL_IMPL_FN

	template<class T, size_t Components>
	void FillUniformV(UniformFillRequest* req, const std::string& name, size_t elements, const T* value)
	{
		auto location = glGetUniformLocation(req->program, name.c_str());
		auto count = GLsizei(elements);
#define XXX_GL_IMPL_FN(Type, ComponentCount, GLPrefix, UniformType) \
		if constexpr (std::is_same_v<T, Type> && Components == ComponentCount) \
		{ \
			glUniform##ComponentCount##GLPrefix##v(location, count, value); \
		}
		XXX_GL_UNIFORM_TYPE(XXX_GL_IMPL_FN)
		static_assert("FillUniformV: Unsupported type");
	}
#undef XXX_GL_IMPL_FN

#define XXX_GL_INST_FN(Type, ComponentCount, GLPrefix, UniformType) \
	template const char* GetUniformTypeName<Type, ComponentCount>();
	XXX_GL_UNIFORM_TYPE(XXX_GL_INST_FN)
#undef XXX_GL_INST_FN
#define XXX_GL_INST_FN(Type, ComponentCount, GLPrefix, UniformType) \
	template void FillUniformV<Type, ComponentCount>(UniformFillRequest* req, const std::string& name, size_t elements, const Type* value);
	XXX_GL_UNIFORM_TYPE(XXX_GL_INST_FN)
#undef XXX_GL_INST_FN

#undef XXX_GL_UNIFORM_TYPE
}