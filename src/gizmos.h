#pragma once

#include "maths.h"


struct GizmoVertex
{
	Vec3 position;
	Vec3 color;
};

void ResetGizmos();
void DrawGizmos(Mat4 projectionMatrix, Mat4 viewMatrix);

void PushGizmoVertex(GizmoVertex vertex);


void GizmoArrow(Vec3 from, Vec3 to, float neckRadius, float headRadius, float headSegment, Vec3 color);
void GizmoHandle(Vec3 from, Vec3 to, float neckRadius, float headRadius, Vec3 color);

void GizmoDisc(Vec3 origin, Vec3 normal, float innerRadius, float outerRadius, Vec3 color);

void GizmoCube(Vec3 origin, Vec3 halfExtents, Vec3 color);
// void GizmoLine(Vec3 start, Vec3 end, Vec3 color); // ~Todo