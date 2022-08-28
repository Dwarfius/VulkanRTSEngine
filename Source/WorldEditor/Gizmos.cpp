#include "Precomp.h"
#include "Gizmos.h"

#include <Engine/Game.h>
#include <Engine/GameObject.h>
#include <Engine/Input.h>

#include <Graphics/Utils.h>
#include <Graphics/Camera.h>

#include <Core/Transform.h>

bool Gizmos::Draw(GameObject& aGameObj, Game& aGame)
{
	Transform transf = aGameObj.GetWorldTransform();
	if (Draw(transf, aGame))
	{
		aGameObj.SetWorldTransform(transf);
		return true;
	}
	return false;
}

bool Gizmos::Draw(Transform& aTransf, Game& aGame)
{
	return DrawTranslation(aTransf, aGame);
}

bool Gizmos::DrawTranslation(Transform& aTransf, Game& aGame)
{
	const glm::vec3 origin = aTransf.GetPos();
	const glm::vec3 right = aTransf.GetRight() * kGizmoRange;
	const glm::vec3 up = aTransf.GetUp() * kGizmoRange;
	const glm::vec3 forward = aTransf.GetForward() * kGizmoRange;

	const glm::vec3 ends[3]{
		origin + right,
		origin + up,
		origin + forward
	};
	const glm::vec3 colors[3]{
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1}
	};
	const glm::vec2 mousePos = Input::GetMousePos();
	constexpr glm::vec3 kHighlightColor{ 1, 1, 0 };
	if (!Input::GetMouseBtn(0))
	{
		myPickedAxis = Axis::None;
	}

	auto GenerateArrow = [](glm::vec3 aStart, glm::vec3 anEnd, glm::vec3(&aVerts)[3]) {
		const glm::vec3 dir = glm::normalize(anEnd - aStart);
		glm::quat origRotation;
		if (glm::abs(dir.y) >= 0.99f)
		{
			origRotation = glm::quatLookAtLH(dir, { 1, 0, 0 });
		}
		else
		{
			origRotation = glm::quatLookAtLH(dir, { 0, 1, 0 });
		}

		constexpr float kYawAngle = glm::radians(60.f);
		constexpr float kRollAngle = glm::radians(120.f);
		glm::quat rotation = glm::quat({ 0, kYawAngle, 0 });
		for (uint8_t i = 0; i < 3; i++)
		{
			glm::vec3 arrowEdge = origRotation * rotation * glm::vec3{ 1, 0, 0 };
			aVerts[i] = anEnd + arrowEdge * 0.1f;
			rotation = glm::quat({ 0, 0, kRollAngle }) * rotation;
		}
	};

	DebugDrawer& drawer = aGame.GetDebugDrawer();
	auto DrawArrow = [&](glm::vec3 aStart, glm::vec3 anEnd, const glm::vec3(&aVerts)[3], glm::vec3 aColor) {
		drawer.AddLine(aStart, anEnd, aColor);
		for (uint8_t i = 0; i < 3; i++)
		{
			drawer.AddLine(anEnd, aVerts[i], aColor);
		}
		drawer.AddLine(aVerts[0], aVerts[1], aColor);
		drawer.AddLine(aVerts[1], aVerts[2], aColor);
		drawer.AddLine(aVerts[2], aVerts[0], aColor);
	};

	const glm::vec2 screenSize{
		aGame.GetGraphics()->GetWidth(),
		aGame.GetGraphics()->GetHeight()
	};
	const Camera& camera = *aGame.GetCamera();
	for (uint8_t i = 0; i < 3; i++)
	{
		glm::vec3 arrowVertices[3];
		GenerateArrow(origin, ends[i], arrowVertices);

		const glm::vec2 projectedEnd = Utils::WorldToScreen(
			ends[i],
			screenSize,
			camera.Get()
		);
		const glm::vec2 projectedArrows[3]{
			Utils::WorldToScreen(arrowVertices[0], screenSize, camera.Get()),
			Utils::WorldToScreen(arrowVertices[1], screenSize, camera.Get()),
			Utils::WorldToScreen(arrowVertices[2], screenSize, camera.Get())
		};
		const bool isHighlighted = !ImGui::GetIO().WantCaptureMouse
			&& (Utils::IsInTriangle(
			mousePos,
			projectedEnd,
			projectedArrows[0],
			projectedArrows[1]
		) || Utils::IsInTriangle(
			mousePos,
			projectedEnd,
			projectedArrows[1],
			projectedArrows[2]
		) || Utils::IsInTriangle(
			mousePos,
			projectedEnd,
			projectedArrows[2],
			projectedArrows[0]
		));
		if (isHighlighted && myPickedAxis == Axis::None)
		{
			myPickedAxis = static_cast<Axis>(i);

			const glm::vec3 start = Utils::ScreenToWorld(mousePos, 0, screenSize, camera.Get());
			const glm::vec3 end = Utils::ScreenToWorld(mousePos, 1, screenSize, camera.Get());
			const Utils::Ray cameraRay{ start, glm::normalize(end - start) };
			const Utils::Ray axisRay{ origin, glm::normalize(ends[i] - origin) };
			float ignore, t;
			Utils::GetClosestTBetweenRays(cameraRay, axisRay, ignore, t);
			myOldMousePosWS = axisRay.myOrigin + axisRay.myDir * t;
		}

		const glm::vec3 color = isHighlighted 
			|| myPickedAxis == static_cast<Axis>(i)
			? kHighlightColor : colors[i];
		DrawArrow(origin, ends[i], arrowVertices, color);
	}

	if (Input::GetMouseBtn(0) && myPickedAxis != Axis::None)
	{
		const glm::vec3 start = Utils::ScreenToWorld(mousePos, 0, screenSize, camera.Get());
		const glm::vec3 end = Utils::ScreenToWorld(mousePos, 1, screenSize, camera.Get());
		const Utils::Ray cameraRay{ start, glm::normalize(end - start) };
		const Utils::Ray axisRay{ 
			origin, 
			glm::normalize(ends[static_cast<uint8_t>(myPickedAxis)] - origin) 
		};
		float ignore, t;
		Utils::GetClosestTBetweenRays(cameraRay, axisRay, ignore, t);
		const glm::vec3 newMappedMouseWS = axisRay.myOrigin + axisRay.myDir * t;

		const glm::vec3 mouseDeltaWS = newMappedMouseWS - myOldMousePosWS;
		myOldMousePosWS = newMappedMouseWS;
		aTransf.Translate(mouseDeltaWS);
		return true;
	}

	return myPickedAxis != Axis::None;
}