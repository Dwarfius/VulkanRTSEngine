#include "Precomp.h"
#include "Gizmos.h"

#include <Engine/Game.h>
#include <Engine/GameObject.h>
#include <Engine/Input.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>

#include <Graphics/Utils.h>
#include <Graphics/Camera.h>

#include <Core/Transform.h>

#include <glm/gtx/vector_angle.hpp>

namespace 
{
	constexpr glm::vec3 kColors[]{
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
	};

	constexpr glm::vec3 kNormals[]{
		{ -1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 }
	};
	constexpr float kTolerance = 0.1f;
}

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
	{
		std::lock_guard lock(aGame.GetImGUISystem().GetMutex());
		if (ImGui::Begin("Gizmo Options"))
		{
			const ImU32 activeColor = ImGui::GetColorU32(ImGuiCol_ButtonActive);

			if (myMode == Mode::Translation)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
			}
			if (ImGui::Button("Translate"))
			{
				myMode = Mode::Translation;
			}
			if (myMode == Mode::Translation)
			{
				ImGui::PopStyleColor();
			}
			ImGui::SameLine();

			if (myMode == Mode::Rotation)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
			}
			if (ImGui::Button("Rotate"))
			{
				myMode = Mode::Rotation;
			}
			if (myMode == Mode::Rotation)
			{
				ImGui::PopStyleColor();
			}
			ImGui::SameLine();

			if (myMode == Mode::Scale)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, activeColor);
			}
			if (ImGui::Button("Scale"))
			{
				myMode = Mode::Scale;
			}
			if (myMode == Mode::Scale)
			{
				ImGui::PopStyleColor();
			}
		}
		ImGui::End();

		if (!ImGui::GetIO().WantCaptureKeyboard)
		{
			if (Input::GetKeyPressed(Input::Keys::T))
			{
				myMode = Mode::Translation;
			}
			if (Input::GetKeyPressed(Input::Keys::R))
			{
				myMode = Mode::Rotation;
			}
			if (Input::GetKeyPressed(Input::Keys::S))
			{
				myMode = Mode::Scale;
			}
		}
	}

	switch (myMode)
	{
	case Mode::Translation:
		return DrawTranslation(aTransf, aGame);
	case Mode::Rotation:
		return DrawRotation(aTransf, aGame);
	case Mode::Scale:
		return DrawScale(aTransf, aGame);
	default:
		ASSERT(false);
		return false;
	}
}

bool Gizmos::DrawTranslation(Transform& aTransf, Game& aGame)
{
	// TODO: simplify this func!
	const glm::vec3 origin = aTransf.GetPos();
	const glm::vec3 right = aTransf.GetRight() * kGizmoRange;
	const glm::vec3 up = aTransf.GetUp() * kGizmoRange;
	const glm::vec3 forward = aTransf.GetForward() * kGizmoRange;

	const glm::vec3 ends[3]{
		origin + right,
		origin + up,
		origin + forward
	};
	const glm::vec2 mousePos = Input::GetMousePos();
	
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
			myOldMousePosOrDir = axisRay.myOrigin + axisRay.myDir * t;
		}

		const glm::vec3 color = isHighlighted 
			|| myPickedAxis == static_cast<Axis>(i)
			? kHighlightColor : kColors[i];
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

		const glm::vec3 mouseDeltaWS = newMappedMouseWS - myOldMousePosOrDir;
		myOldMousePosOrDir = newMappedMouseWS;
		aTransf.Translate(mouseDeltaWS);
	}

	return myPickedAxis != Axis::None;
}

bool Gizmos::DrawRotation(Transform& aTransf, Game& aGame)
{
	const glm::vec3& center = aTransf.GetPos();

	const glm::vec2 mousePos = Input::GetMousePos();
	const glm::vec2 screenSize{
		aGame.GetGraphics()->GetWidth(),
		aGame.GetGraphics()->GetHeight()
	};
	const Camera& camera = *aGame.GetCamera();
	const glm::vec3 start = Utils::ScreenToWorld(mousePos, 0, screenSize, camera.Get());
	const glm::vec3 end = Utils::ScreenToWorld(mousePos, 1, screenSize, camera.Get());
	const Utils::Ray screenRay{ start, glm::normalize(end - start) };
	auto CircleCheck = [](glm::vec3 aCenter, glm::vec3 aNormal, float aRadius, 
		const Utils::Ray& aRay, glm::vec3& anIntersectPoint)
	{
		const Utils::Plane circlePlane{ aCenter, aNormal };
		float rayT;
		if (Utils::Intersects(aRay, circlePlane, rayT))
		{
			anIntersectPoint = aRay.myOrigin + rayT * aRay.myDir;
			const float distance = glm::distance(aCenter, anIntersectPoint);
			return glm::abs(distance - aRadius) < 0.05f;
		}
		return false;
	};

	if (!Input::GetMouseBtn(0))
	{
		myPickedAxis = Axis::None;
	}

	DebugDrawer& drawer = aGame.GetDebugDrawer();
	for (uint8_t i = 0; i < 3; i++)
	{
		glm::vec3 intersectionPoint;
		const bool intersects = CircleCheck(center, kNormals[i],
			kGizmoRange, screenRay, intersectionPoint);
		if (intersects && myPickedAxis == Axis::None)
		{
			myPickedAxis = static_cast<Axis>(i);

			myRotationDirStart = glm::normalize(intersectionPoint - center);
			myOldMousePosOrDir = myRotationDirStart;
		}
		
		const glm::vec3 color = static_cast<Axis>(i) == myPickedAxis ?
			kHighlightColor : kColors[i];
		drawer.AddCircle(center, kNormals[i], kGizmoRange, color);
	}

	if (Input::GetMouseBtn(0) && myPickedAxis != Axis::None)
	{
		// draw out a rotation indicator
		drawer.AddLine(center, center + myRotationDirStart * kGizmoRange, { 1, 1, 1 });

		// going to recheck the plane intersection to get new coordinates
		const glm::vec3& circleNormal = kNormals[static_cast<uint8_t>(myPickedAxis)];
		float rayT;
		Utils::Intersects(screenRay, Utils::Plane{ center, circleNormal }, rayT);
		const glm::vec3 newIntersection = screenRay.myOrigin + rayT * screenRay.myDir;
		const glm::vec3 dirNormalized = glm::normalize(newIntersection - center);

		drawer.AddLine(center, center + dirNormalized * kGizmoRange, { 1, 1, 1 });

		const float angleDelta = glm::orientedAngle(
			myOldMousePosOrDir,
			dirNormalized, 
			circleNormal
		);

		if (glm::abs(angleDelta) >= glm::radians(0.1f))
		{
			aTransf.Rotate({
				myPickedAxis == Axis::Right ? -angleDelta : 0,
				myPickedAxis == Axis::Up ? angleDelta : 0,
				myPickedAxis == Axis::Forward ? angleDelta : 0,
				}
			);
			myOldMousePosOrDir = dirNormalized;
		}
	}

	return myPickedAxis != Axis::None;
}

bool Gizmos::DrawScale(Transform& aTransf, Game& aGame)
{
	const glm::vec3 center = aTransf.GetPos();

	const glm::vec2 mousePos = Input::GetMousePos();
	const glm::vec2 screenSize{
		aGame.GetGraphics()->GetWidth(),
		aGame.GetGraphics()->GetHeight()
	};
	const Camera& camera = *aGame.GetCamera();
	const glm::vec3 start = Utils::ScreenToWorld(mousePos, 0, screenSize, camera.Get());
	const glm::vec3 end = Utils::ScreenToWorld(mousePos, 1, screenSize, camera.Get());
	const Utils::Ray screenRay{ start, glm::normalize(end - start) };

	if (!Input::GetMouseBtn(0))
	{
		myPickedAxis = Axis::None;
	}

	constexpr glm::vec3 kBoxExtents{ 0.1f };
	DebugDrawer& drawer = aGame.GetDebugDrawer();
	for (uint8_t i = 0; i < 3; i++)
	{
		const glm::vec3 endPoint = center + kNormals[i] * kGizmoRange;
		const Utils::AABB box{ 
			endPoint - kBoxExtents, 
			endPoint + kBoxExtents 
		};

		float t;
		const bool isHighlighted = !ImGui::GetIO().WantCaptureMouse
			&& Utils::Intersects(screenRay, box, t);
		if (isHighlighted && myPickedAxis == Axis::None)
		{
			myPickedAxis = static_cast<Axis>(i);

			// we can't use intersection point with AABB because any
			// face of AABB doesn't lie on the plane containing the
			// picked axis (it'll be slightly out of it), so to avoid jumps
			// we need to remap it onto the axis
			const Utils::Ray axisRay{ center, kNormals[i] };
			float ignore;
			Utils::GetClosestTBetweenRays(screenRay, axisRay, ignore, t);
			myOldMousePosOrDir = axisRay.myOrigin + axisRay.myDir * t;
		}

		const glm::vec3 color = isHighlighted ? kHighlightColor : kColors[i];
		drawer.AddLine(center, endPoint, color);
		drawer.AddAABB(box.myMin, box.myMax, color);
	}

	if (Input::GetMouseBtn(0) && myPickedAxis != Axis::None)
	{
		const Utils::Ray axisRay{ center, kNormals[static_cast<uint32_t>(myPickedAxis)] };

		float ignore, t;
		Utils::GetClosestTBetweenRays(screenRay, axisRay, ignore, t);

		const glm::vec3 newMappedMouseWS = axisRay.myOrigin + axisRay.myDir * t;
		const glm::vec3 delta = newMappedMouseWS - myOldMousePosOrDir;
		const glm::vec3 newScale = aTransf.GetScale() + glm::vec3{ 
			-delta.x, delta.y, delta.z 
		};
		aTransf.SetScale(newScale);

		myOldMousePosOrDir = newMappedMouseWS;
	}

	return myPickedAxis != Axis::None;
}