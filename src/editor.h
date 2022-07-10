#pragma once

#include "render.h"

namespace app
{
	struct Editor;
	Editor* EditorInit();
	void EditorFrame(Editor* editor);
	void EditorDeinit(Editor* editor);

	void EditorSetRenderOutput(Editor* editor, RenderTextureHandle tex);
	glm::ivec2 EditorGetViewportResolution(Editor* editor);
	glm::ivec2 EditorGetViewportOffset(Editor* editor);

	bool GizmoDragPoint(Editor* editor, int id, glm::vec2& origin, float size);
	bool GizmoRotation(Editor* editor, int id, float& angle, glm::vec2 origin, float size, float radius);
	void GizmoCircle(Editor* editor, glm::vec2 origin, float radius, float thickness);
	bool GizmoDragRay(Editor* editor, int id, float& distance, glm::vec2 origin, glm::vec2 direction, float handleSize, float lineThickness);

	glm::vec2 LogicalToViewport(Editor* editor, glm::vec2 pt);
	glm::vec2 ViewportToLogical(Editor* editor, glm::vec2 pt);
	float LogicalDistanceToViewport(Editor* editor, float val);
	float ViewportToLogicalDistance(Editor* editor, float val);
}