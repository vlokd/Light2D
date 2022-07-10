#include "main.h"
#include "scene.h"
#include "editor.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace app
{
	REGISTER_SCENE_OBJECT(SceneObjectTransform, Transform);
	REGISTER_SCENE_OBJECT(SceneObjectCircle, Circle);
	REGISTER_SCENE_OBJECT(SceneObjectRectangle, Rectangle);
	REGISTER_SCENE_OBJECT(SceneObjectPolygon, Polygon);
	REGISTER_SCENE_OBJECT(SceneObjectUnion, Union);
	REGISTER_SCENE_OBJECT(SceneObjectDifference, Difference);
	REGISTER_SCENE_OBJECT(SceneObjectIntersection, Intersection);
	REGISTER_SCENE_OBJECT(SceneObjectAnnular, Annular);
	REGISTER_SCENE_OBJECT(SceneObjectMirror, Mirror);

	static const char* SceneObjectDragDropTag = "SCENE_OBJECT_DRAG_DROP";

	bool OnObjectsDragDrop(ISceneObject::Handle firstHandle, ISceneObject::Handle secondHandle, Scene& scene, bool asSwap)
	{
		if (firstHandle.IsValid() && firstHandle == secondHandle) { return false; }
		auto* first = scene.objects.Get(firstHandle);
		auto* firstChildren = first ? first->GetChildren() : &scene.rootObjects;

		auto* second = scene.objects.Get(secondHandle);
		auto* secondParent = second ? scene.objects.Get(second->parent) : nullptr;
		auto* secondParentChildren = secondParent ? secondParent->GetChildren() : &scene.rootObjects;

		if (asSwap)
		{
			auto* firstParent = first ? scene.objects.Get(first->parent) : nullptr;

			if (firstParent != secondParent) { return false; }

			auto* firstParentChildren = firstParent ? firstParent->GetChildren() : &scene.rootObjects;

			auto firstIter = std::find(firstParentChildren->begin(), firstParentChildren->end(), firstHandle);
			auto secondIter = std::find(secondParentChildren->begin(), secondParentChildren->end(), secondHandle);
			std::iter_swap(firstIter, secondIter);
			std::swap(first->parent, second->parent);
		}
		else
		{
			std::erase(*secondParentChildren, secondHandle);
			firstChildren->push_back(secondHandle);
			second->parent = first ? firstHandle : ISceneObject::Handle{};
		}

		return true;
	}
	void OnObjectDragDropSource(ISceneObject::Handle handle, const std::string& label, Scene& scene)
	{
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(SceneObjectDragDropTag, &handle, sizeof(ISceneObject::Handle));
			ImGui::Text("%s", label.c_str());
			ImGui::EndDragDropSource();
		}
	}
	bool OnObjectDragDropTarget(ISceneObject::Handle acceptorHandle, Scene& scene, bool asParent)
	{
		bool changed = false;
		if (ImGui::GetDragDropPayload())
		{
			if (asParent)
			{
				ImGui::Selectable("##DragDropTarget");
			}
			if (ImGui::BeginDragDropTarget())
			{
				if (auto* payload = ImGui::AcceptDragDropPayload(SceneObjectDragDropTag))
				{
					auto otherHandle = *static_cast<const ISceneObject::Handle*>(payload->Data);
					changed |= OnObjectsDragDrop(acceptorHandle, otherHandle, scene, !asParent);
				}
				ImGui::EndDragDropTarget();
			}
		}
		return changed;
	}
	bool EditMaterialHandle(const std::string& label, SceneMaterial::Handle& thisHandle, Scene& scene)
	{
		bool changed = false;
		auto* thisMaterial = scene.materials.Get(thisHandle);
		std::string name = thisMaterial ? thisMaterial->name : std::string{};
		if (ImGui::BeginCombo(label.c_str(), name.c_str()))
		{
			for (const auto& [handle, material] : scene.materials.entries)
			{
				if (ImGui::Selectable(material->name.c_str(), handle == thisHandle))
				{
					changed = thisHandle != handle;
					thisHandle = handle;
				}
			}
			ImGui::EndCombo();
		}
		return changed;
	}
	void MarkObjectForDeletion(ISceneObject* object, Scene& scene)
	{
		if (object)
		{
			object->state = ISceneObject::State::Deleted;
			if (auto* children = object->GetChildren())
			{
				for (auto& childHandle : *children)
				{
					MarkObjectForDeletion(scene.objects.Get(childHandle), scene);
				}
			}
		}
	}

	SceneChange ISceneObject::OnEditor(Scene& scene)
	{
		auto handle = scene.objects.GetHandle(this);
		auto name = std::string(GetName());
		auto change = SceneChange::None;
		ImGui::PushID(handle.value);
		ImGui::AlignTextToFramePadding();
		bool open = ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_AllowItemOverlap);
		OnObjectDragDropSource(handle, name, scene);
		ImGui::SameLine();
		if (ImGui::Button("Delete"))
		{
			MarkObjectForDeletion(this, scene);
			change |= SceneChange::ShaderInvalid;
		}
		change |= SceneChange::ShaderInvalid && OnObjectDragDropTarget(handle, scene, false);
		if (open)
		{
			change |= OnEditorImpl(scene);
			if (auto* children = GetChildren())
			{
				for (auto childHandle : *children)
				{
					if (auto* child = scene.objects.Get(childHandle))
					{
						change |= child->OnEditor(scene);
					}
				}
				change |= SceneChange::ShaderInvalid && OnObjectDragDropTarget(handle, scene, true);
			}
			ImGui::TreePop();
		}
		ImGui::PopID();
		return change;
	}
	glm::mat3 ISceneObject::GetTransform(const Scene& scene) const
	{
		glm::mat3 transform{ 1.f };
		if (auto* parentPtr = scene.objects.Get(parent))
		{
			transform = parentPtr->GetTransform(scene);
		}
		return transform;
	}
	std::string ISceneObject::GetShaderFunctionName(const Scene& scene) const
	{
		return fmt::format("{}_{}", GetName(), scene.objects.GetHandle(this).value);
	}

	std::string GetObjectUniformName(const std::string& baseName, const ISceneObject& object, const Scene& scene)
	{
		return fmt::format("u_{}_{}", baseName, scene.objects.GetHandle(&object).value);
	}
	std::string GetObjectFunctionHeader(const ISceneObject& object, const Scene& scene)
	{
		return fmt::format("TraceResult {}(vec2 pt, vec2 d)", object.GetShaderFunctionName(scene));
	}

	SceneChange SceneObjectTransform::OnEditorImpl(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		change |= SceneChange::IntegrationInvalid && ImGui::DragFloat2("Translation", (float*)&translation, 1.f, -1000.f, 1000.f, "%.6f");
		change |= SceneChange::IntegrationInvalid && ImGui::DragFloat("Rotation", (float*)&rotation, 1.f, -1000.f, 1000.f, "%.6f");
		return change;
	}
	std::string SceneObjectTransform::GetShaderDeclarations(const Scene& scene) const
	{
		std::string res;
		res += fmt::format("uniform vec2 {};\n", GetObjectUniformName("translation", *this, scene));
		res += fmt::format("uniform float {};\n", GetObjectUniformName("rotation", *this, scene));
		res += GetObjectFunctionHeader(*this, scene) + ";\n";
		return res;
	}
	std::string SceneObjectTransform::GetShaderCommands(const Scene& scene) const
	{
		std::string res = R"xxx(
	{codegen_fn}
	{
		pt = pt - {codegen_u_tr};
		pt = Rotate(pt, {codegen_u_rot});
		TraceResult res;
		res.dst = MAX_TRACE_DST;
		{codegen_children}
		return res;
	}	
)xxx";
		std::string childrenStr{};
		for (auto childHandle : children)
		{
			if (auto* child = scene.objects.Get(childHandle))
			{
				auto childFn = child->GetShaderFunctionName(scene);
				childrenStr += fmt::format("res = TraceUnion(res, {}(pt, d));", childFn);
			}
		}
		ReplaceSubstr(res, "{codegen_fn}", GetObjectFunctionHeader(*this, scene));
		ReplaceSubstr(res, "{codegen_u_rot}", GetObjectUniformName("rotation", *this, scene));
		ReplaceSubstr(res, "{codegen_u_tr}", GetObjectUniformName("translation", *this, scene));
		ReplaceSubstr(res, "{codegen_children}", childrenStr);

		return res;
	}
	void SceneObjectTransform::FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const
	{
		FillUniform(req, GetObjectUniformName("rotation", *this, scene), rotation);
		FillUniform(req, GetObjectUniformName("translation", *this, scene), translation);
	}
	glm::mat3 SceneObjectTransform::GetTransform(const Scene& scene) const
	{
		auto transform = ISceneObject::GetTransform(scene);
		auto tr = glm::translate(glm::mat3(1.f), translation);
		auto rot = glm::rotate(glm::mat3(1.f), rotation);
		transform = transform * tr * rot;
		return transform;
	}
	SceneChange SceneObjectTransform::OnGizmos(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		auto* editor = GetEditor();
		auto transform = ISceneObject::GetTransform(scene);
		auto iTransform = glm::inverse(transform);
		glm::vec3 origin{ translation, 1.f };
		glm::vec3 left{ 1.0f, 0.f, 1.f };
		glm::vec3 right{ 0.f, 1.f, 1.f };
		origin = transform * origin;
		left = transform * left;
		right = transform * right;
		auto o2 = glm::vec2(origin);
		ImGui::PushID(scene.objects.GetHandle(this).value);
		change |= SceneChange::IntegrationInvalid && GizmoDragPoint(editor, 0, o2, 0.03f);
		change |= SceneChange::IntegrationInvalid && GizmoRotation(editor, 1, rotation, o2, 0.02f, 0.1f);
		translation = glm::vec2(iTransform * glm::vec3(o2, 1.f));
		ImGui::PopID();
		return change;
	}
	std::string SceneObjectTransform::Serialize(const Scene& scene) const
	{
		std::string res{};
		res += "auto object = new SceneObjectTransform();\n";
		res += fmt::format("object->translation = glm::vec2({:.6f}f, {:.6f}f);\n", translation.x, translation.y);
		res += fmt::format("object->rotation = {:.6f}f;\n", rotation);
		return res;
	}

	SceneChange SceneObjectCircle::OnEditorImpl(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		change |= SceneChange::IntegrationInvalid && ImGui::DragFloat("Radius", (float*)&radius, 1.f, -1000.f, 1000.f, "%.6f");
		change |= SceneChange::IntegrationInvalid && EditMaterialHandle("Material", material, scene);
		return change;
	}
	std::string SceneObjectCircle::GetShaderDeclarations(const Scene& scene) const
	{
		std::string res;
		res += fmt::format("uniform float {};\n", GetObjectUniformName("radius", *this, scene));
		res += fmt::format("uniform int {};\n", GetObjectUniformName("material_id", *this, scene));
		res += GetObjectFunctionHeader(*this, scene) + ";\n";
		return res;
	}
	std::string SceneObjectCircle::GetShaderCommands(const Scene& scene) const
	{
		std::string res = R"xxx(
	{codegen_fn}
	{
		TraceResult res;
		res.dst = CircleSDF(pt, {codegen_u_rad});
		res.emission = u_materials[{codegen_u_mat_id}].emission;
		res.refractionIndex = u_materials[{codegen_u_mat_id}].refraction;
		res.absorption = u_materials[{codegen_u_mat_id}].absorption;
		return res;
	}	
)xxx";
		ReplaceSubstr(res, "{codegen_fn}", GetObjectFunctionHeader(*this, scene));
		ReplaceSubstr(res, "{codegen_u_rad}", GetObjectUniformName("radius", *this, scene));
		ReplaceSubstr(res, "{codegen_u_mat_id}", GetObjectUniformName("material_id", *this, scene));
		return res;
	}
	void SceneObjectCircle::FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const
	{
		FillUniform(req, GetObjectUniformName("radius", *this, scene), radius);
		FillUniform(req, GetObjectUniformName("material_id", *this, scene), int(material.value));
	}
	SceneChange SceneObjectCircle::OnGizmos(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		ImGui::PushID(scene.objects.GetHandle(this).value);
		auto transform = GetTransform(scene);
		auto origin = glm::vec2(transform * glm::vec3(0.f, 0.f, 1.f));
		auto dragPoint = origin + glm::vec2{ radius, 0.f };
		auto* editor = GetEditor();
		GizmoCircle(editor, origin, radius, 0.01f);
		change |= SceneChange::IntegrationInvalid && GizmoDragPoint(editor, 0, dragPoint, 0.02f);
		radius = length(dragPoint - origin);
		radius = radius < 0.f ? 0.f : radius;
		ImGui::PopID();
		return change;
	}
	std::string SceneObjectCircle::Serialize(const Scene& scene) const
	{
		std::string res{};
		res += "auto object = new SceneObjectCircle();\n";
		res += fmt::format("object->radius = {:.6f}f;\n", radius);
		res += fmt::format("object->material = SceneMaterial::Handle({});\n", material.value);
		return res;
	}

	SceneChange SceneObjectRectangle::OnEditorImpl(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		change |= SceneChange::IntegrationInvalid && ImGui::DragFloat2("Size", (float*)&halfSize, 1.f, 0.f, 1000.f, "%.6f");
		change |= SceneChange::IntegrationInvalid && ImGui::DragFloat("Rounding", (float*)&rounding, 1.f, 0.f, 1000.f, "%.6f");
		change |= SceneChange::IntegrationInvalid && EditMaterialHandle("Material", material, scene);
		return change;
	}
	std::string SceneObjectRectangle::GetShaderDeclarations(const Scene& scene) const
	{
		std::string res;
		res += fmt::format("uniform float {};\n", GetObjectUniformName("rounding", *this, scene));
		res += fmt::format("uniform vec2 {};\n", GetObjectUniformName("halfSize", *this, scene));
		res += fmt::format("uniform int {};\n", GetObjectUniformName("material_id", *this, scene));
		res += GetObjectFunctionHeader(*this, scene) + ";\n";
		return res;
	}
	std::string SceneObjectRectangle::GetShaderCommands(const Scene& scene) const
	{
		std::string res = R"xxx(
	{codegen_fn}
	{
		TraceResult res;
		res.dst = RectangleSDF(pt, {codegen_u_halfSize}, {codegen_u_rounding});
		res.emission = u_materials[{codegen_u_mat_id}].emission;
		res.refractionIndex = u_materials[{codegen_u_mat_id}].refraction;
		res.absorption = u_materials[{codegen_u_mat_id}].absorption;
		return res;
	}	
)xxx";
		ReplaceSubstr(res, "{codegen_fn}", GetObjectFunctionHeader(*this, scene));
		ReplaceSubstr(res, "{codegen_u_halfSize}", GetObjectUniformName("halfSize", *this, scene));
		ReplaceSubstr(res, "{codegen_u_rounding}", GetObjectUniformName("rounding", *this, scene));
		ReplaceSubstr(res, "{codegen_u_mat_id}", GetObjectUniformName("material_id", *this, scene));
		return res;
	}
	void SceneObjectRectangle::FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const
	{
		FillUniform(req, GetObjectUniformName("halfSize", *this, scene), halfSize);
		FillUniform(req, GetObjectUniformName("rounding", *this, scene), rounding);
		FillUniform(req, GetObjectUniformName("material_id", *this, scene), int(material.value));
	}
	SceneChange SceneObjectRectangle::OnGizmos(Scene& scene)
	{
		auto* editor = GetEditor();
		SceneChange change = SceneChange::None;
		ImGui::PushID(scene.objects.GetHandle(this).value);
		auto transform = GetTransform(scene);
		auto origin = glm::vec2(transform * glm::vec3(0.f, 0.f, 1.f));
		auto right = glm::vec2(transform * glm::vec3(1.f, 0.f, 1.f));
		right = glm::normalize(right - origin);
		auto top = glm::vec2(transform * glm::vec3(0.f, 1.f, 1.f));
		top = glm::normalize(top - origin);
		change |= SceneChange::IntegrationInvalid && GizmoDragRay(editor, 0, halfSize.x, origin, right, 0.02f, 0.01f);
		change |= SceneChange::IntegrationInvalid && GizmoDragRay(editor, 1, halfSize.y, origin, top, 0.02f, 0.01f);
		halfSize = glm::clamp(halfSize, glm::vec2(0.f), glm::vec2(std::numeric_limits<float>::max()));
		ImGui::PopID();
		return change;
	}
	std::string SceneObjectRectangle::Serialize(const Scene& scene) const
	{
		std::string res{};
		res += "auto object = new SceneObjectRectangle();\n";
		res += fmt::format("object->halfSize = glm::vec2({:.6f}f, {:.6f}f);\n", halfSize.x, halfSize.y);
		res += fmt::format("object->rounding = {:.6f}f;\n", rounding);
		res += fmt::format("object->material = SceneMaterial::Handle({});\n", material.value);
		return res;
	}

	SceneChange SceneObjectPolygon::OnEditorImpl(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		change |= SceneChange::IntegrationInvalid && EditMaterialHandle("Material", material, scene);
		change |= SceneChange::IntegrationInvalid && ImGui::DragFloat("Rounding", (float*)&rounding, 1.f, 0.f, 1000.f, "%.6f");
		if (ImGui::CollapsingHeader("Points"))
		{
			bool isEnabled = points.size() < 16;
			ImGui::BeginDisabled(!isEnabled);
			if (ImGui::Button("Add"))
			{
				points.emplace_back();
				change |= SceneChange::ShaderInvalid;
			}
			ImGui::EndDisabled();
			int i = 0;
			for (auto& p : points)
			{
				ImGui::PushID(i);
				change |= SceneChange::IntegrationInvalid && ImGui::DragFloat2("##PointEd", (float*)(&p), 1.f, -1000.f, 1000.f, "%.6f");
				ImGui::PopID();
				++i;
			}
		}
		return change;
	}
	std::string SceneObjectPolygon::GetShaderDeclarations(const Scene& scene) const
	{
		std::string res;
		res += fmt::format("uniform float {};\n", GetObjectUniformName("rounding", *this, scene));
		res += fmt::format("uniform int {};\n", GetObjectUniformName("material_id", *this, scene));
		res += fmt::format("uniform vec2 {}[POLYGON_POINTS_MAX];\n", GetObjectUniformName("points", *this, scene));
		res += fmt::format("uniform int {};\n", GetObjectUniformName("point_count", *this, scene));
		res += GetObjectFunctionHeader(*this, scene) + ";\n";
		return res;
	}
	std::string SceneObjectPolygon::GetShaderCommands(const Scene& scene) const
	{
		std::string res = R"xxx(
	{codegen_fn}
	{
		TraceResult res;
		res.dst = PolygonSDF(pt, {codegen_points_cnt}, {codegen_points}, {codegen_rounding});
		res.emission = u_materials[{codegen_u_mat_id}].emission;
		res.refractionIndex = u_materials[{codegen_u_mat_id}].refraction;
		res.absorption = u_materials[{codegen_u_mat_id}].absorption;
		return res;
	}	
)xxx";
		ReplaceSubstr(res, "{codegen_fn}", GetObjectFunctionHeader(*this, scene));
		ReplaceSubstr(res, "{codegen_points_cnt}", GetObjectUniformName("point_count", *this, scene));
		ReplaceSubstr(res, "{codegen_points}", GetObjectUniformName("points", *this, scene));
		ReplaceSubstr(res, "{codegen_rounding}", GetObjectUniformName("rounding", *this, scene));
		ReplaceSubstr(res, "{codegen_u_mat_id}", GetObjectUniformName("material_id", *this, scene));
		return res;
	}
	void SceneObjectPolygon::FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const
	{
		FillUniform(req, GetObjectUniformName("rounding", *this, scene), rounding);
		FillUniform(req, GetObjectUniformName("material_id", *this, scene), int(material.value));
		FillUniform(req, GetObjectUniformName("point_count", *this, scene), int(points.size()));
		FillUniformV<float, 2>(req, GetObjectUniformName("points", *this, scene), points.size(), (float*)points.data());
	}
	SceneChange SceneObjectPolygon::OnGizmos(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		auto* editor = GetEditor();
		ImGui::PushID(scene.objects.GetHandle(this).value);
		auto transform = GetTransform(scene);
		auto iTransform = glm::inverse(transform);
		int pidx = 0;
		for (auto& p : points)
		{
			auto pt = glm::vec2(transform * glm::vec3(p, 1.f));
			if (GizmoDragPoint(editor, pidx++, pt, 0.02f))
			{
				change |= SceneChange::IntegrationInvalid;
				p = glm::vec2(iTransform * glm::vec3(pt, 1.f));
			}
		}
		ImGui::PopID();
		return change;
	}
	std::string SceneObjectPolygon::Serialize(const Scene& scene) const
	{
		std::string res{};
		res += "auto object = new SceneObjectPolygon();\n";
		res += fmt::format("object->rounding = {:.6f}f;\n", rounding);
		res += fmt::format("object->material = SceneMaterial::Handle({});\n", material.value);
		for (const auto& pt : points)
		{
			res += fmt::format("object->points.push_back(glm::vec2({:.6f}f, {:.6f}f));\n", pt.x, pt.y);
		}
		return res;
	}

	std::string SceneObjectExactOperator::GetShaderDeclarations(const Scene & scene) const
	{
		return GetObjectFunctionHeader(*this, scene) + ";\n";
	}
	std::string SceneObjectExactOperator::GetShaderCommands(const Scene& scene) const
	{
		std::string res = R"xxx(
	{codegen_fn}
	{
		TraceResult res;
		{codegen_children}
		return res;
	}	
)xxx";
		std::string operatorName = "Trace" + std::string(GetName());
		std::string childrenStr{};
		if (children.size() > 0)
		{
			if (auto* child = scene.objects.Get(children[0]))
			{
				auto childFn = child->GetShaderFunctionName(scene);
				childrenStr += fmt::format("res = {}(pt, d);", childFn);
			}
			for (int i = 1; i < int(children.size()); ++i)
			{
				if (auto* child = scene.objects.Get(children[i]))
				{
					auto childFn = child->GetShaderFunctionName(scene);
					childrenStr += fmt::format("res = {}(res, {}(pt, d));", operatorName, childFn);
				}
			}
		}


		ReplaceSubstr(res, "{codegen_fn}", GetObjectFunctionHeader(*this, scene));
		ReplaceSubstr(res, "{codegen_children}", childrenStr);

		return res;
	}
	std::string SceneObjectExactOperator::Serialize(const Scene& scene) const
	{
		std::string res{};
		res += fmt::format("auto object = new SceneObject{}();\n", GetName());
		return res;
	}

	SceneChange SceneObjectAnnular::OnEditorImpl(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		change |= SceneChange::IntegrationInvalid && ImGui::DragFloat("Radius", (float*)&radius, 1.f, 0.f, 1000.f, "%.6f");
		return change;
	}
	std::string SceneObjectAnnular::GetShaderDeclarations(const Scene& scene) const
	{
		std::string res{};
		res += GetObjectFunctionHeader(*this, scene) + ";\n";
		res += fmt::format("uniform float {};\n", GetObjectUniformName("radius", *this, scene));
		return res;
	}
	std::string SceneObjectAnnular::GetShaderCommands(const Scene& scene) const
	{
		std::string res = R"xxx(
	{codegen_fn}
	{
		TraceResult res;
		res.dst = MAX_TRACE_DST;
		{codegen_children}
		return res;
	}	
)xxx";
		std::string childrenStr{};
		if (children.size() > 0)
		{

			for (auto handle : children)
			{
				if (auto* child = scene.objects.Get(handle))
				{
					auto childFn = child->GetShaderFunctionName(scene);
					childrenStr += "{\n";
					childrenStr += fmt::format("TraceResult r = {}(pt, d);\n", childFn);
					childrenStr += fmt::format("r.dst = AnnularSDF(r.dst, {});\n", GetObjectUniformName("radius", *this, scene));
					childrenStr += "res = TraceUnion(res, r);\n";
					childrenStr += "}\n";
				}
			}
		}


		ReplaceSubstr(res, "{codegen_fn}", GetObjectFunctionHeader(*this, scene));
		ReplaceSubstr(res, "{codegen_children}", childrenStr);

		return res;
	}
	void SceneObjectAnnular::FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const
	{
		FillUniform(req, GetObjectUniformName("radius", *this, scene), radius);
	}
	std::string SceneObjectAnnular::Serialize(const Scene& scene) const
	{
		std::string res{};
		res += fmt::format("auto object = new SceneObjectAnnular();\n");
		res += fmt::format("object->radius = {:.6f}f;\n", radius);
		return res;
	}

	
	SceneChange SceneObjectMirror::OnEditorImpl(Scene& scene)
	{
		SceneChange change = SceneChange::None;
		change |= SceneChange::ShaderInvalid && ImGui::Checkbox("X", &mirrorX);
		change |= SceneChange::ShaderInvalid && ImGui::Checkbox("Y", &mirrorY);
		return change;
	}
	std::string SceneObjectMirror::GetShaderDeclarations(const Scene& scene) const
	{
		return GetObjectFunctionHeader(*this, scene) + ";\n";
	}
	std::string SceneObjectMirror::GetShaderCommands(const Scene& scene) const
	{
		std::string res = R"xxx(
	{codegen_fn}
	{
		TraceResult res;
		res.dst = MAX_TRACE_DST;
		{codegen_children}
		return res;
	}	
)xxx";
		std::string childrenStr{};
		if (children.size() > 0)
		{

			for (auto handle : children)
			{
				if (auto* child = scene.objects.Get(handle))
				{
					auto childFn = child->GetShaderFunctionName(scene);
					childrenStr += "{\n";
					childrenStr += fmt::format("TraceResult r = {}(pt, d);\n", childFn);
					childrenStr += "res = TraceUnion(res, r);\n";
					childrenStr += "}\n";
					if (mirrorX)
					{
						childrenStr += "{\n";
						childrenStr += fmt::format("TraceResult r = {}(pt * vec2(-1.f, 1.f), d);\n", childFn);
						childrenStr += "res = TraceUnion(res, r);\n";
						childrenStr += "}\n";
					}
					if (mirrorY)
					{
						childrenStr += "{\n";
						childrenStr += fmt::format("TraceResult r = {}(pt * vec2(1.f, -1.f), d);\n", childFn);
						childrenStr += "res = TraceUnion(res, r);\n";
						childrenStr += "}\n";
					}
					if (mirrorX && mirrorY)
					{
						childrenStr += "{\n";
						childrenStr += fmt::format("TraceResult r = {}(pt * vec2(-1.f, -1.f), d);\n", childFn);
						childrenStr += "res = TraceUnion(res, r);\n";
						childrenStr += "}\n";
					}
				}
			}
		}
		ReplaceSubstr(res, "{codegen_fn}", GetObjectFunctionHeader(*this, scene));
		ReplaceSubstr(res, "{codegen_children}", childrenStr);
		return res;
	}
	void SceneObjectMirror::FillShaderUniforms(UniformFillRequest* req, const Scene& scene) const
	{

	}
	std::string SceneObjectMirror::Serialize(const Scene& scene) const
	{
		std::string res{};
		res += "auto object = new SceneObjectMirror();\n";
		res += fmt::format("object->mirrorX = {};\n", mirrorX);
		res += fmt::format("object->mirrorY = {};\n", mirrorY);
		return res;
	}

	Scene::Scene()
	{
#ifdef PROJECT_BUILD_DEV
		variantNames.push_back(empyVariantName);
		variantLoaders.push_back([] {});
		#include "scene_variants/sandbox.cpp"
		#include "scene_variants/gems.cpp"
		#include "scene_variants/refraction.cpp"
		variantNames.push_back("random");
		variantLoaders.push_back([this]()
		{
			LoadRandom();
		});

		LoadVariant(currentVariant);
#else
		Reset();
		LoadRandom();
#endif
	}

	bool EditString(const std::string& label, std::string& value)
	{
		bool changed = false;
		std::array<char, 2048> buf{};
		memset(buf.data(), '\0', buf.size());
		memcpy(buf.data(), value.c_str(), value.size());
		if (ImGui::InputText(label.c_str(), buf.data(), buf.size()))
		{
			value = std::string(buf.data());
			changed = true;
		}
		return changed;
	}

	SceneChange Scene::OnEditor()
	{
		SceneChange change = SceneChange::None;
		if (ImGui::BeginTabItem("Materials"))
		{
			if (ImGui::Button("Add", { ImGui::GetContentRegionAvail().x, 0 }))
			{
				auto* material = new SceneMaterial();
				auto handle = materials.Add(material);
				material->name = "Material_" + std::to_string(handle.value);
				change |= SceneChange::ShaderInvalid;
			}
			for (auto& [handle, material] : materials.entries)
			{
				auto fullLabel = fmt::format("{}###{}", material->name, handle.value);
				if (ImGui::TreeNode(fullLabel.c_str()))
				{
					EditString("Name", material->name);
					change |= SceneChange::IntegrationInvalid && ImGui::ColorEdit3("Color", (float*)&material->emission, ImGuiColorEditFlags_Float);
					change |= SceneChange::IntegrationInvalid && ImGui::DragFloat("Intensity", (float*)&material->emission[3], 1.f, 0.f, 1000.f, "%.6f");
					change |= SceneChange::IntegrationInvalid && ImGui::DragFloat3("Refraction", (float*)&material->refractionIndex, 0.1f, 0.0f, 1000.f, "%.6f");
					change |= SceneChange::IntegrationInvalid && ImGui::DragFloat3("Absorption", (float*)&material->absorption, 0.1f, 0.0f, 1000.f, "%.6f");
					ImGui::TreePop();
				}
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Objects"))
		{
			{
				ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::BeginCombo("##PopupAddCombo", "Add", ImGuiComboFlags_NoArrowButton))
				{
					auto& objectFactory = Factory<ISceneObject>::Instance();
					for (const auto& [name, constructor] : objectFactory.entries)
					{
						if (ImGui::Selectable(name, false))
						{
							auto* object = constructor();
							rootObjects.push_back(objects.Add(object));
							change |= SceneChange::ShaderInvalid;
						}
					}
					ImGui::EndCombo();
				}
				ImGui::PopItemWidth();
			}
			for (auto handle : rootObjects)
			{
				if (auto* object = objects.Get(handle))
				{
					change |= object->OnEditor(*this);
				}
			}
			ImGui::PushID(int(rootObjects.size()));
			change |= SceneChange::ShaderInvalid && OnObjectDragDropTarget(ISceneObject::Handle{}, *this, true);
			ImGui::PopID();
			ImGui::EndTabItem();
		}
		std::erase_if(rootObjects, [this](const auto& handle)
		{
			if (auto* object = objects.Get(handle))
			{
				return object->state == ISceneObject::State::Deleted;
			}
			return true;
		});
		std::erase_if(objects.entries, [](const auto& e)
		{
			return e.value->state == ISceneObject::State::Deleted;
		});
		return change;
	}
	SceneChange Scene::OnGizmos()
	{
		SceneChange change = SceneChange::None;
		for (auto& [objectHandle, object] : objects.entries)
		{
			change |= object->OnGizmos(*this);
		}
		return change;
	}
	std::string Scene::GetShaderContent() const
	{
		std::string res{};
		res += fmt::format("uniform Material u_materials[{}];\n", materials.entries.size() + 1);
		for (auto& [objectHandle, object] : objects.entries)
		{
			res += object->GetShaderDeclarations(*this);
		}
		for (auto& [objectHandle, object] : objects.entries)
		{
			res += object->GetShaderCommands(*this);
		}
		std::string mainFN = R"xxx(
		TraceResult TraceScene(vec2 pt, vec2 d)
		{
			TraceResult res;
			res.dst = MAX_TRACE_DST;
			{codegen_roots}
			return res;
		}
)xxx";
		std::string rootsStr{};
		for (auto objectHandle : rootObjects)
		{
			if (auto* object = objects.Get(objectHandle))
			{
				rootsStr += fmt::format("res = TraceUnion(res, {}(pt, d));", object->GetShaderFunctionName(*this));
			}
		}
		ReplaceSubstr(mainFN, "{codegen_roots}", rootsStr);
		res += mainFN;
		return res;
	}
	void Scene::FillShaderUniforms(UniformFillRequest* req) const
	{
		auto stage = GetRenderStage(req);
		if (stage == RenderStage::Common)
		{
			for (auto& [objectHandle, object] : objects.entries)
			{
				object->FillShaderUniforms(req, *this);
			}
		}
		else
		{
			auto iStage = int(stage);
			for (auto& [handle, material] : materials.entries)
			{
				{
					auto name = fmt::format("u_materials[{}].emission", handle.value);
					FillUniform(req, name, material->emission[iStage] * material->emission[3]);
				}
				{
					auto name = fmt::format("u_materials[{}].refraction", handle.value);
					FillUniform(req, name, material->refractionIndex[iStage]);
				}
				{
					auto name = fmt::format("u_materials[{}].absorption", handle.value);
					FillUniform(req, name, material->absorption[iStage]);
				}
			}
		}
	}
	std::string Scene::Serialize() const
	{
		std::string res{};
		res += fmt::format("variantNames.push_back(\"{}\");\n", currentVariant);
		res += "variantLoaders.push_back([this]()\n";
		res += "{\n";
		for (const auto& [handle, material] : materials.entries)
		{
			res += "{\n";
			res += "auto material = new SceneMaterial();";
			res += fmt::format("material->name = \"{}\";\n", material->name),
			res += fmt::format("material->emission = glm::vec4({:.6f}f, {:.6f}f, {:.6f}f, {:.6f}f);\n",
				material->emission.x, material->emission.y, material->emission.z, material->emission.w);
			res += fmt::format("material->refractionIndex = glm::vec3({:.6f}f, {:.6f}f, {:.6f}f);\n",
				material->refractionIndex.x, material->refractionIndex.y, material->refractionIndex.z);
			res += fmt::format("material->absorption = glm::vec3({:.6f}f, {:.6f}f, {:.6f}f);\n",
				material->absorption.x, material->absorption.y, material->absorption.z);
			res += fmt::format("materials.Add(SceneMaterial::Handle({}), material);\n", handle.value);
			res += "}\n";
		}
		res += fmt::format("materials.nextFreeHandleValue = {};\n", materials.nextFreeHandleValue);
		for (const auto& [handle, object] : objects.entries)
		{
			res += "{\n";
			res += object->Serialize(*this);
			if (auto* children = object->GetChildren())
			{
				for (const auto& child : *children)
				{
					res += fmt::format("object->children.push_back(ISceneObject::Handle({}));\n", child.value);
				}
			}
			res += fmt::format("object->parent = ISceneObject::Handle({});\n", object->parent.value);
			res += fmt::format("objects.Add(ISceneObject::Handle({}), object);\n", handle.value);
			res += "}\n";
		}
		res += fmt::format("objects.nextFreeHandleValue = {};\n", objects.nextFreeHandleValue);
		for (const auto& handle : rootObjects)
		{
			res += fmt::format("rootObjects.push_back(ISceneObject::Handle({}));\n", handle.value);
		}
		if (auto* render = GetRender())
		{
			res += fmt::format("if (GetRender()) RenderSetExposure(GetRender(), {:.6f}f);\n", RenderGetExposure(render));
		}
		res += "});\n";
		return res;
	}

	void Scene::Reset()
	{
		objects.entries.clear();
		objects.nextFreeHandleValue = 1;
		materials.entries.clear();
		materials.nextFreeHandleValue = 1;
		rootObjects.clear();
		if (auto* render = GetRender())
		{
			RenderSetExposure(render, 1.f);
		}
	}

	void Scene::LoadVariant(const std::string& name)
	{
		int idx = 0;
		for (const auto& variantName : variantNames)
		{
			if (variantName == name)
			{
				Reset();
				variantLoaders[idx]();
				currentVariant = name;
				break;
			}
			else
			{
				idx++;
			}
		}
	}
	void Scene::LoadRandom()
	{
		Reset();
		bool glow = rand() % 2;
		RandomLight(glow);
		auto mat = RandomMaterial(glow);
		auto shape = rand() % 3;
		if (shape == 0)
		{
			RandomShape0(mat);
		}
		else if (shape == 1)
		{
			RandomShape1(mat);
		}
		else if (shape == 2)
		{
			RandomShape2(mat);
		}
		currentVariant = "random";
	}
	ISceneObject::Handle Scene::RandomLight(bool glow)
	{
		SceneMaterial::Handle lightMatHandle{};
		{
			auto* material = new SceneMaterial();
			material->name = "Light";
			//auto emissionColor = glm::linearRand(glm::vec3(0.9, 0.75f, 0.5f), glm::vec3(1.f));
			auto emissionColor = glm::vec3(1.f, 0.8f, 0.5f);
			material->emission = glm::vec4(emissionColor, 4.f);
			if (!glow)
			{
				material->emission.w *= 1.5f;
			}
			lightMatHandle = materials.Add(material);
		}
		ISceneObject::Handle lightHandle{};
		{
			auto* object = new SceneObjectCircle();
			object->material = lightMatHandle;
			object->radius = 0.2f;
			auto objectHandle = objects.Add(object);
			auto* transform = new SceneObjectTransform();
			auto angle = glm::linearRand(0.f, std::numbers::pi_v<float> *2.f);
			transform->translation = glm::vec2(cos(angle), sin(angle)) * 2.f;
			auto transformHandle = objects.Add(transform);
			object->parent = transformHandle;
			transform->children.push_back(objectHandle);
			rootObjects.push_back(transformHandle);
			lightHandle = transformHandle;
		}
		return lightHandle;
	}
	SceneMaterial::Handle Scene::RandomMaterial(bool glow)
	{
		auto* material = new SceneMaterial();
		material->name = "Mat0";
		auto absColor = glm::linearRand(glm::vec3(0.f), glm::vec3(1.f));
		auto absStr = glm::linearRand(0.f, 5.f);
		//auto absStr = glm::abs(glm::gaussRand(0.0f, 1.f));
		material->absorption = absColor * absStr;
		auto refractionBase = glm::linearRand(1.f, 2.2f);
		auto refractionDiv = glm::linearRand(0.f, 0.05f);
		material->refractionIndex.r = refractionBase + refractionDiv / (0.7f * 0.7f);
		material->refractionIndex.g = refractionBase + refractionDiv / (0.55f * 0.55f);
		material->refractionIndex.b = refractionBase + refractionDiv / (0.45f * 0.45f);
		if (glow)
		{
			material->emission = glm::vec4(glm::vec3(1.f) - absColor, glm::linearRand(0.0f, 0.5f));
		}
		auto matHandle = materials.Add(material);
		return matHandle;
	}
	ISceneObject::Handle Scene::RandomShape0(SceneMaterial::Handle materialHandle)
	{
		ISceneObject::Handle shapeHandle{};
		{
			auto* object = new SceneObjectPolygon();
			if (rand() % 2)
			{
				object->rounding = glm::linearRand(0.f, 0.1f);
			}
			object->material = materialHandle;
			int pointCount = rand() % 6 + 3;
			float angularStep = 2.f * std::numbers::pi_v<float> / pointCount;
			for (int i = 0; i < pointCount; ++i)
			{
				float angle = angularStep * i + glm::linearRand(-0.1f, 0.1f);
				float dst = glm::linearRand(0.2f, 0.9f);
				glm::vec2 dir{ std::cos(angle), std::sin(angle) };
				glm::vec2 pt = dir * dst;
				object->points.push_back(pt);
			}
			auto objectHandle = objects.Add(object);
			rootObjects.push_back(objectHandle);
			shapeHandle = objectHandle;
		}
		return shapeHandle;
	}
	ISceneObject::Handle Scene::RandomShape1(SceneMaterial::Handle materialHandle)
	{
		ISceneObject::Handle shapeHandle{};
		{
			auto* object = new SceneObjectCircle();
			float rad = 0.8f;
			object->radius = rad;
			object->material = materialHandle;
			auto objectHandle = objects.Add(object);
			int pointCount = rand() % 10 + 3;
			float angularStep = 2.f * std::numbers::pi_v<float> / pointCount;
			std::vector<glm::vec2> points;
			for (int i = 0; i < pointCount; ++i)
			{
				float angle = angularStep * i + glm::linearRand(-0.5f, 0.5f);;
				float dst = 1.f;
				glm::vec2 dir{ std::cos(angle), std::sin(angle) };
				glm::vec2 pt = dir * dst;
				points.push_back(pt);
			}

			std::vector<glm::vec2> origins;
			std::vector<float> radiuses;
			bool success = false;
			int tryCount = 16;

			while (!success && tryCount > 0)
			{
				origins.clear();
				radiuses.clear();
				int i = 0;
				int j = int(points.size()) - 1;
				while (i < int(points.size()))
				{
					auto edge = points[i] - points[j];
					auto oe = points[j] + 0.5f * edge;
					auto en = glm::normalize(edge);
					auto enp = glm::vec2(-en.y, en.x);
					auto origin = oe + enp * glm::linearRand(-1.5f, 0.5f);
					auto radius = glm::length(origin - points[j]);

					if (glm::length(origin) - radius < rad)
					{
						origins.push_back(origin);
						radiuses.push_back(radius);
					}
					j = i;
					++i;
			}

				glm::ivec2 tileCount{ 64,64 };
				glm::vec2 tileSize = glm::vec2(2.f * rad) / glm::vec2(tileCount);
				int tilesRemoved = 0;
				int totalTiles = tileCount.x * tileCount.y;
				for (int y = 0; y < tileCount.y; ++y)
				{
					for (int x = 0; x < tileCount.x; ++x)
					{
						glm::vec2 coord(x, y);
						glm::vec2 center = glm::vec2(-rad) + coord * tileSize + tileSize * 1.5f;
						for (int p = 0; p < int(origins.size()); ++p)
						{
							if (glm::length(center - origins[p]) <= radiuses[p])
							{
								tilesRemoved++;
								break;
							}
						}
					}
				}
				float areaRemaining = 1.f - float(tilesRemoved) / totalTiles;
				if (areaRemaining < 0.05f)
				{
					success = false;
					tryCount--;
				}
				else
				{
					success = true;
				}
			}

			auto* diff = new SceneObjectDifference();
			auto diffHandle = objects.Add(diff);
			diff->children.push_back(objectHandle);
			object->parent = diffHandle;
			for (int p = 0; p < int(origins.size()); ++p)
			{
				auto* circle = new SceneObjectCircle();
				circle->radius = radiuses[p];
				circle->material = materialHandle;
				auto ch = objects.Add(circle);

				auto* tr = new SceneObjectTransform();
				tr->translation = origins[p];
				tr->children.push_back(ch);
				auto th = objects.Add(tr);
				circle->parent = th;
				diff->children.push_back(th);
				tr->parent = diffHandle;
			}
			rootObjects.push_back(diffHandle);
			shapeHandle = diffHandle;
		}
		return shapeHandle;
	}
	ISceneObject::Handle Scene::RandomShape2(SceneMaterial::Handle materialHandle)
	{
		float pi = std::numbers::pi_v<float>;
		ISceneObject::Handle shapeHandle{};
		{
			auto* object = new SceneObjectPolygon();
			if (rand() % 2)
			{
				object->rounding = glm::linearRand(0.f, 0.1f);
			}
			object->material = materialHandle;
			int pointCount = rand() % 6 + 3;
			object->points.resize(pointCount);
			object->points[0] = glm::vec2(-0.001f, glm::linearRand(-0.8f, -0.2f));
			object->points[1] = glm::vec2(-0.001f, glm::linearRand(0.2f, 0.8f));
			float angularStep = pi / (pointCount - 2);
			(void)angularStep;
			for (int i = 2; i < pointCount; ++i)
			{
				float amin = angularStep * (i - 2);
				float amax = angularStep * (i - 2 + 1);
				float angle = pi / 2.f - glm::linearRand(amin, amax);
				float dst = glm::linearRand(0.2f, 0.8f);
				glm::vec2 dir{ std::cos(angle), std::sin(angle) };
				glm::vec2 pt = dir * dst;
				object->points[i] = pt;
			}
			auto objectHandle = objects.Add(object);

			auto* mirror = new SceneObjectMirror();
			mirror->mirrorX = true;
			auto mirrorHandle = objects.Add(mirror);
			object->parent = mirrorHandle;
			mirror->children.push_back(objectHandle);
			rootObjects.push_back(mirrorHandle);
			shapeHandle = mirrorHandle;
		}
		return shapeHandle;
	}
}