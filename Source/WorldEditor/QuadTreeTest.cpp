#include "Precomp.h"
#include "QuadTreeTest.h"

// see note in .h
#ifdef WE_QUADTREE

#include <Engine/Game.h>
#include <Engine/Input.h>

#include <Graphics/Camera.h>
#include <Graphics/Utils.h>

QuadTreeTest::QuadTreeTest()
	: myPointsQuadTree({ 10, 10 }, { 110, 110 }, 6)
{
}

void QuadTreeTest::DrawTab()
{
	if (ImGui::BeginTabItem("QuadTree Test"))
	{
		char buffer[256];
		Utils::StringFormat(buffer, "Points: {}", myPoints.GetCount());
		ImGui::Text(buffer);

		constexpr const char* kNames[]{
			"None", "Add", "Erase"
		};
		if (ImGui::BeginCombo("Mode", kNames[std::to_underlying(myQuadTreeMode)]))
		{
			for (uint8_t i = 0; i < std::size(kNames); i++)
			{
				bool selected = i == std::to_underlying(myQuadTreeMode);
				if (ImGui::Selectable(kNames[i], &selected))
				{
					myQuadTreeMode = static_cast<QuadTreeMode>(i);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SliderFloat("Size", &myCreateSize, 0.5f, 20.f);

		ImGui::EndTabItem();
	}
}

void QuadTreeTest::DrawTree(Game& aGame)
{
	if (myQuadTreeMode == QuadTreeMode::None)
	{
		return;
	}

	DebugDrawer& debugDrawer = aGame.GetDebugDrawer();

	const glm::vec3 rootMin{ 10, 0, 10 };
	const glm::vec3 rootMax{ 110, 0, 110 };
	const glm::vec3 color{ 1, 1, 0 };
	debugDrawer.AddRect(rootMin, { rootMin.x, 0, rootMax.z }, 
		rootMax, { rootMax.x, 0, rootMin.z }, color);

	Camera& cam = *aGame.GetCamera();
	Utils::Ray mouseRay{ cam.GetTransform().GetPos(), cam.GetTransform().GetForward() };
	Utils::Plane quadTreePlane{ rootMin, {0, 1, 0} };
	float dist = 0;
	if (Utils::Intersects(mouseRay, quadTreePlane, dist))
	{
		const glm::vec3 intersectPoint = mouseRay.myOrigin + mouseRay.myDir * dist;
		debugDrawer.AddCircle(intersectPoint, { 0, 1, 0 }, myCreateSize, { 1, 0, 1 });

		if (Input::GetMouseBtnPressed(0)
			&& intersectPoint.x > rootMin.x && intersectPoint.x < rootMax.x
			&& intersectPoint.z > rootMin.z && intersectPoint.z < rootMax.z)
		{
			const glm::vec2 pPos = { intersectPoint.x, intersectPoint.z };
			const glm::vec2 pMin = pPos - glm::vec2{ myCreateSize, myCreateSize };
			const glm::vec2 pMax = pPos + glm::vec2{ myCreateSize, myCreateSize };
			switch (myQuadTreeMode)
			{
			case QuadTreeMode::Add:
			{
				Point& p = myPoints.Allocate();
				p.myPos = pPos;
				p.mySize = myCreateSize;
				p.myQuadInfo = myPointsQuadTree.Add(pMin, pMax, &p);
			}
			break;
			case QuadTreeMode::Erase:
			{
				std::vector<Point*> toDelete;
				myPointsQuadTree.Test(pMin, pMax, [&](Point* aPoint)
				{
					const glm::vec2 diff = aPoint->myPos - pPos;
					const float distSqr = diff.x * diff.x + diff.y * diff.y;
					const float touchDistSqr = (myCreateSize + aPoint->mySize) * (myCreateSize + aPoint->mySize);
					if (distSqr <= touchDistSqr)
					{
						toDelete.emplace_back(aPoint);
					}
					return true; // keep going
				});

				for (Point* p : toDelete)
				{
					myPointsQuadTree.Remove(p->myQuadInfo, p);
					myPoints.Free(*p);
				}
			}
			break;
			}
		}
	}

	constexpr glm::vec3 kDepthColors[]{
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1}
	};
	myPointsQuadTree.ForEachQuad([&](glm::vec2 aMin, glm::vec2 aMax, uint8_t aDepth, std::span<Point*> aPoints)
	{
		const glm::vec3 min{ aMin.x, 0, aMin.y };
		const glm::vec3 max{ aMax.x, 0, aMax.y };
		const glm::vec3 color = kDepthColors[aDepth % std::size(kDepthColors)];
		debugDrawer.AddRect(min, { min.x, 0, max.z }, max, { max.x, 0, min.z }, color);

		for (Point* p : aPoints)
		{
			debugDrawer.AddCircle({ p->myPos.x, 0, p->myPos.y }, { 0, 1, 0 }, p->mySize, color);
		}
	});
}

#endif