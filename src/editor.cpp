#include "editor.h"
#include "main.h"

#include <imgui.h>
#include <imgui_internal.h>

#include "embedded/droid_sans_ttf.cpp"

namespace app
{
	enum class GizmoColorId
	{
		Btn_Normal,
		Btn_Hovered,
		Btn_Active,
		Line_Normal,
		EnumSize_,
	};

	struct Editor
	{
		inline static std::array<ImU32, size_t(GizmoColorId::EnumSize_)> GizmoColors;
		inline static float GizmoLineThickness = 2.f;
		ImGuiID dragId = 0;
		glm::vec2 dragOffset{ 0.f, 0.f };
		ImGuiID secondaryGizmoID = 0;
		float secondaryGizmoParamF = 0.f;
		bool needRebuildLayout = true;
		bool showGizmos = true;

		glm::ivec2 viewportOffset;
		glm::ivec2 viewportSize;

		RenderTextureHandle renderOutput{};
	};

	Editor* EditorInit()
	{
		auto* editor = new Editor();

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = NULL;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		ImGui::StyleColorsDark();

		auto& style = ImGui::GetStyle();
		for (int colIdx = ImGuiCol_Text; colIdx < ImGuiCol_COUNT; ++colIdx)
		{
			auto& col = style.Colors[colIdx];
			auto gcol = glm::vec4(col);
			float gs = glm::dot(gcol, glm::vec4(0.2125, 0.7154, 0.0721, 0.0f));
			gcol.r = gcol.g = gcol.b = gs;
			col = ImVec4(gcol);
		}
		style.Colors[ImGuiCol_WindowBg].w = 1.f;

		float dpiScale = GetViewInfo().dpiScale;
		float fontSize = floor(16.f * dpiScale);

		ImFontConfig fontCfg{};
		fontCfg.SizePixels = fontSize;
		fontCfg.OversampleH = 2;
		fontCfg.OversampleV = 2;
		io.Fonts->AddFontFromMemoryCompressedTTF(
			DroidSansEmbeddedTTF_compressed_data,
			DroidSansEmbeddedTTF_compressed_size,
			fontSize, &fontCfg);
		style.ScaleAllSizes(dpiScale);

		glm::vec3 baseColor{ 0.5f, 0.5f, 0.5f };
		auto normalColor = baseColor;
		auto hoveredColor = baseColor * 1.25f;
		auto activeColor = baseColor * 1.5f;
		auto lineColor = normalColor * 0.75f;
		Editor::GizmoColors[size_t(GizmoColorId::Btn_Normal)] = ImColor(glm::vec4(normalColor, 1.f));
		Editor::GizmoColors[size_t(GizmoColorId::Btn_Hovered)] = ImColor(glm::vec4(hoveredColor, 1.f));
		Editor::GizmoColors[size_t(GizmoColorId::Btn_Active)] = ImColor(glm::vec4(activeColor, 1.f));
		Editor::GizmoColors[size_t(GizmoColorId::Line_Normal)] = ImColor(glm::vec4(lineColor, 1.f));

#ifndef PROJECT_BUILD_DEV
		editor->showGizmos = false;
#endif

		return editor;
	}

	void EditorFrame(Editor* editor)
	{
		auto& io = ImGui::GetIO();
		SceneChange sceneChange = SceneChange::None;
		auto* scene = GetScene();

		auto windowFlags =
			//ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBackground |
			//ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoSavedSettings;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(io.DisplaySize);
		if (ImGui::Begin("MainDockWindow", nullptr, windowFlags))
		{
			ImGui::PopStyleVar(3);

			ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), 0);
			if (editor->needRebuildLayout)
			{
				auto* centralNode = ImGui::DockBuilderGetCentralNode(dockspace_id);
				auto cid = centralNode->ID;
				ImGuiID leftId = 0;
				ImGuiID rightId = 0;
				ImGui::DockBuilderSplitNode(cid, ImGuiDir_Right, 0.75f, &rightId, &leftId);
				auto* leftNode = ImGui::DockBuilderGetNode(leftId);
				leftNode->SetLocalFlags(ImGuiDockNodeFlags_NoTabBar);
				auto* rightNode = ImGui::DockBuilderGetNode(rightId);
				rightNode->SetLocalFlags(ImGuiDockNodeFlags_NoTabBar);
				ImGui::DockBuilderDockWindow("SideWindow", leftId);
				ImGui::DockBuilderDockWindow("ViewportWindow", rightId);
				ImGui::DockBuilderFinish(dockspace_id);
				editor->needRebuildLayout = false;
			}
			ImGui::End();
		}
		if (ImGui::Begin("SideWindow", 0, ImGuiWindowFlags_NoTitleBar))
		{
			ImGui::Text("Frametime %f ms", GetTimeInfo().dt * 1000.f);
			if (scene)
			{
#ifdef PROJECT_BUILD_DEV
				const auto& variants = scene->variantNames;
				const auto& currentVariant = scene->currentVariant;
				//ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::BeginCombo("##PopupVariantCombo", currentVariant.c_str()))
				{
					for (const auto& name : variants)
					{
						if (ImGui::Selectable(name.c_str(), name == currentVariant))
						{
							if (name != currentVariant)
							{
								scene->LoadVariant(name);
								sceneChange |= SceneChange::ShaderInvalid;
							}
						}
					}
					ImGui::EndCombo();
				}
				ImGui::BeginDisabled(scene->currentVariant == Scene::empyVariantName);
				if (ImGui::Button("Save"))
				{
					auto fileName = fmt::format("/scene_variants/{}.cpp", scene->currentVariant);
					DevWriteToTextFile(fileName, scene->Serialize());
				}
				ImGui::EndDisabled();
				ImGui::SameLine();
				if (ImGui::Button("New"))
				{
					scene->LoadRandom();
					sceneChange |= SceneChange::ShaderInvalid;
				}
#else
				if (ImGui::Button("New"))
				{
					scene->LoadRandom();
					sceneChange |= SceneChange::ShaderInvalid;
				}
#endif
				ImGui::SameLine();
				if (ImGui::Button("Reset"))
				{
					scene->Reset();
					sceneChange |= SceneChange::ShaderInvalid;
				}
				//ImGui::PopItemWidth();
			}
			ImGui::Checkbox("Show Gizmos", &editor->showGizmos);
			if (ImGui::BeginTabBar("SideWindowBar"))
			{
				if (scene)
				{
					sceneChange |= scene->OnEditor();
				}
				if (auto* render = GetRender())
				{
					sceneChange |= RenderOnEditor(render);
				}
				ImGui::EndTabBar();
			}
			ImGui::End();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		if (ImGui::Begin("ViewportWindow", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar))
		{
			ImGui::PopStyleVar(3);
			auto ws = ImGui::GetContentRegionAvail();
			auto wp = glm::vec2(ImGui::GetWindowPos()) + glm::vec2(ImGui::GetCursorStartPos());

			editor->viewportSize = glm::ivec2(ws.x, ws.y);
			editor->viewportOffset = glm::ivec2(wp.x, wp.y);

			ImGui::Image((ImTextureID*)(uintptr_t)(editor->renderOutput.data), ImGui::GetContentRegionAvail(), { 0, 1 }, { 1, 0 });
			{
#ifdef PROJECT_BUILD_DEV
				ImGui::SetCursorPos({ 0.f, 0.f });
				ImGui::Text("%s", RenderGetShaderBuildErrors(GetRender()).c_str());
#endif
			}
			if (editor->showGizmos)
			{
				if (scene)
				{
					sceneChange |= scene->OnGizmos();
				}
			}
			ImGui::End();
		}
		if (sceneChange & SceneChange::ShaderInvalid)
		{
			if (scene)
			{
				RenderSetShaderContent(GetRender(), scene->GetShaderContent());
			}
		}
		else if (sceneChange & SceneChange::IntegrationInvalid)
		{
			RenderInvalidateIntegration(GetRender());
		}
	}

	bool GizmoDragBehaviour(Editor* editor, int id, glm::vec2& origin, float size)
	{
		auto& io = ImGui::GetIO();
		auto& ed = *editor;

		ImGui::PushID(id);

		glm::vec2 rHalfSize{ size, size };
		glm::vec2 rmin = origin - rHalfSize;

		ImGui::SetCursorScreenPos(rmin);
		ImGui::InvisibleButton("IB", rHalfSize * 2.f);
		auto itemId = ImGui::GetItemID();

		bool hovered = ImGui::IsItemHovered();

		if (hovered && ImGui::IsMouseDown(0) && ed.dragId == 0)
		{
			ed.dragId = itemId;
			ed.dragOffset = glm::vec2(io.MousePos) - origin;
		}
		if (ed.dragId == itemId)
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_None);
			if (!ImGui::IsMouseDown(0))
			{
				ed.dragId = 0;
			}
			else
			{
				origin = glm::vec2(io.MousePos) - ed.dragOffset;
			}
		}
		ImGui::PopID();
		return ed.dragId == itemId;
	}
	bool _GizmoDragPoint(Editor* editor, int id, glm::vec2& origin, float size)
	{
		auto* dl = ImGui::GetWindowDrawList();
		bool changed = GizmoDragBehaviour(editor, id, origin, size);
		bool hovered = ImGui::IsItemHovered();
		bool active = ImGui::IsItemActive();
		ImU32 color = Editor::GizmoColors[size_t(GizmoColorId::Btn_Normal)];
		//if (editor->dragId == itemId)
		if (active)
		{
			color = Editor::GizmoColors[size_t(GizmoColorId::Btn_Active)];
		}
		else if (hovered)
		{
			color = Editor::GizmoColors[size_t(GizmoColorId::Btn_Hovered)];
		}
		dl->AddCircleFilled(origin, size, color);
		return changed;
	}
	bool GizmoDragPoint(Editor* editor, int id, glm::vec2& origin, float size)
	{
		auto o = LogicalToViewport(editor, origin);
		auto changed = _GizmoDragPoint(editor, id, o, LogicalDistanceToViewport(editor, size));
		origin = ViewportToLogical(editor, o);
		return changed;
	}
	bool _GizmoRotation(Editor* editor, int id, float& angle, glm::vec2 origin, float size, float radius)
	{
		bool changed = false;
		//auto& io = ImGui::GetIO();
		//auto* dl = ImGui::GetWindowDrawList(); 
		ImGui::PushID(id);
		{
			auto dir = glm::normalize(glm::vec2(1.f, -1.f));
			auto v = origin + dir * radius;
			if (_GizmoDragPoint(editor, 0, v, size))
			{
				if (editor->secondaryGizmoID == 0)
				{
					editor->secondaryGizmoID = ImGui::GetID("R0");
					editor->secondaryGizmoParamF = angle;
				}
				if (editor->secondaryGizmoID == ImGui::GetID("R0"))
				{
					auto dir2 = normalize(v - origin);
					auto a1 = std::atan2(-dir.y, dir.x);
					auto a2 = std::atan2(-dir2.y, dir2.x);
					angle = a2 - a1 + editor->secondaryGizmoParamF;
					angle = std::fmodf(angle, 2.0 * std::numbers::pi_v<float>);
					changed = true;
				}
			}
			else if (ImGui::GetID("R0") == editor->secondaryGizmoID)
			{
				editor->secondaryGizmoID = 0;
			}
		}
		ImGui::PopID();
		return changed;
	}
	bool GizmoRotation(Editor* editor, int id, float& angle, glm::vec2 origin, float size, float radius)
	{
		auto o = LogicalToViewport(editor, origin);
		auto changed = _GizmoRotation(editor, id, angle, o, 
			LogicalDistanceToViewport(editor, size), LogicalDistanceToViewport(editor, radius));
		return changed;
	}
	void _GizmoCircle(Editor* editor, glm::vec2 origin, float radius, float thickness)
	{
		auto* dl = ImGui::GetWindowDrawList();
		dl->AddCircle(origin, radius, editor->GizmoColors[size_t(GizmoColorId::Line_Normal)], 0, thickness);
	}
	void GizmoCircle(Editor* editor, glm::vec2 origin, float radius, float thickness)
	{
		auto o = LogicalToViewport(editor, origin);
		auto r = LogicalDistanceToViewport(editor, radius);
		auto t = LogicalDistanceToViewport(editor, thickness);
		_GizmoCircle(editor, o, r, t);
	}
	bool _GizmoDragRay(Editor* editor, int id, float& distance, glm::vec2 origin, glm::vec2 direction, float handleSize, float lineThickness)
	{
		bool changed = false;
		(void)lineThickness;
		auto dragPoint = origin + direction * distance;
		ImGui::PushID(id);
		{
			if (_GizmoDragPoint(editor, 0, dragPoint, handleSize))
			{
				if (editor->secondaryGizmoID == 0)
				{
					editor->secondaryGizmoID = ImGui::GetID("R0");
				}
				if (editor->secondaryGizmoID == ImGui::GetID("R0"))
				{
					auto dstVec = dragPoint - origin;
					auto dstProj = glm::dot(dstVec, direction);
					//distance = glm::sign(dstProj) * std::sqrt(std::abs(dstProj));
					distance = dstProj;
					changed = true;
				}
			}
			else if (ImGui::GetID("R0") == editor->secondaryGizmoID)
			{
				editor->secondaryGizmoID = 0;
			}
		}
		auto* dl = ImGui::GetWindowDrawList();
		dl->AddLine(origin, origin + direction * distance, editor->GizmoColors[size_t(GizmoColorId::Line_Normal)], lineThickness);
		dl->AddCircleFilled(origin + direction * distance, handleSize * 0.5f, editor->GizmoColors[size_t(GizmoColorId::Line_Normal)]);
		ImGui::PopID();
		return changed;
	}
	bool GizmoDragRay(Editor* editor, int id, float& distance, glm::vec2 origin, glm::vec2 direction, float handleSize, float lineThickness)
	{
		auto o = LogicalToViewport(editor, origin);
		direction.y = -direction.y;
		auto dst = LogicalDistanceToViewport(editor, distance);
		auto hs = LogicalDistanceToViewport(editor, handleSize);
		auto t = LogicalDistanceToViewport(editor, lineThickness);
		auto changed = _GizmoDragRay(editor, id, dst, o, direction, hs, t);
		distance = ViewportToLogicalDistance(editor, dst);
		return changed;
	}
	glm::vec2 LogicalToViewport(Editor* editor, glm::vec2 pt)
	{
		auto vs = editor->viewportSize;
		auto vo = glm::vec2(editor->viewportOffset);
		return LogicalToSurface(pt, vs) + vo;
	}
	glm::vec2 ViewportToLogical(Editor* editor, glm::vec2 pt)
	{
		auto vs = editor->viewportSize;
		auto vo = glm::vec2(editor->viewportOffset);
		return SurfaceToLogical(pt - vo, vs);
	}
	float LogicalDistanceToViewport(Editor* editor, float val)
	{
		auto vs = editor->viewportSize;
		return val * glm::min(vs.x, vs.y) / 2.f;
	}
	float ViewportToLogicalDistance(Editor* editor, float val)
	{
		auto vs = editor->viewportSize;
		return (val * 2.f / glm::min(vs.x, vs.y));
	}

	void EditorDeinit(Editor* editor)
	{
		delete editor;
	}

	void EditorSetRenderOutput(Editor* editor, RenderTextureHandle tex)
	{
		editor->renderOutput = tex;
	}

	glm::ivec2 EditorGetViewportResolution(Editor* editor)
	{
		return editor->viewportSize;
	}

	glm::ivec2 EditorGetViewportOffset(Editor* editor)
	{
		return editor->viewportOffset;
	}
}