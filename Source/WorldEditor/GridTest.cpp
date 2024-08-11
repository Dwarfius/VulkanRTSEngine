#include "Precomp.h"
#include "GridTest.h"

#include <Engine/Game.h>
#include <Engine/Input.h>

#include <Graphics/Camera.h>
#include <Graphics/Utils.h>

GridTest::GridTest()
	: myGrid({ 10, 10 }, 100, 5)
{
}

void GridTest::DrawTab()
{
	if (ImGui::BeginTabItem("Grid Test"))
	{
		char buffer[256];
		Utils::StringFormat(buffer, "Points: {}", myPoints.GetCount());
		ImGui::Text(buffer);

		constexpr const char* kNames[]{
			"None", "Add", "Erase"
		};
		if (ImGui::BeginCombo("Mode", kNames[std::to_underlying(myMode)]))
		{
			for (uint8_t i = 0; i < std::size(kNames); i++)
			{
				bool selected = i == std::to_underlying(myMode);
				if (ImGui::Selectable(kNames[i], &selected))
				{
					myMode = static_cast<Mode>(i);
				}
			}
			ImGui::EndCombo();
		}

		ImGui::SliderFloat("Size", &myCreateSize, 0.5f, 20.f);

		ImGui::EndTabItem();
	}
}

void GridTest::DrawGrid(Game& aGame)
{
	if (myMode == Mode::None)
	{
		return;
	}

	DebugDrawer& debugDrawer = aGame.GetDebugDrawer();

	const glm::vec3 rootMin{ 10, 0, 10 };
	const glm::vec3 rootMax{ 110, 0, 110 };

	Camera& cam = *aGame.GetCamera();
	Utils::Ray mouseRay{ cam.GetTransform().GetPos(), cam.GetTransform().GetForward() };
	Utils::Plane gridPlane{ rootMin, {0, 1, 0} };
	float dist = 0;
	if (Utils::Intersects(mouseRay, gridPlane, dist))
	{
		const glm::vec3 intersectPoint = mouseRay.myOrigin + mouseRay.myDir * dist;
		debugDrawer.AddCircle(intersectPoint, { 0, 1, 0 }, myCreateSize, { 1, 0, 1 });

		if (Input::GetMouseBtnPressed(0)
			&& intersectPoint.x > rootMin.x && intersectPoint.x < rootMax.x
			&& intersectPoint.z > rootMin.z && intersectPoint.z < rootMax.z)
		{
			const glm::vec2 pPos = { intersectPoint.x, intersectPoint.z };
			const glm::vec2 pMin = pPos - glm::vec2{ myCreateSize };
			const glm::vec2 pMax = pPos + glm::vec2{ myCreateSize };
			switch (myMode)
			{
			case Mode::Add:
			{
				Point& p = myPoints.Allocate();
				p.myPos = pPos;
				p.mySize = myCreateSize;
				p.myIsBeingDeleted = false;
				myGrid.Add(pMin, pMax, &p);
			}
			break;
			case Mode::Erase:
			{
				std::vector<Point*> toDelete;
				myGrid.ForEachCell(pMin, pMax, [&](std::span<Point*> aPoints)
				{
					for (Point* point : aPoints)
					{
						if (point->myIsBeingDeleted)
						{
							continue;
						}
						const glm::vec2 diff = point->myPos - pPos;
						const float distSqr = diff.x * diff.x + diff.y * diff.y;
						const float touchDistSqr = (myCreateSize + point->mySize) * (myCreateSize + point->mySize);
						if (distSqr <= touchDistSqr)
						{
							toDelete.emplace_back(point);
							point->myIsBeingDeleted = true;
						}
					}
				});

				for (Point* p : toDelete)
				{
					const glm::vec2 min = p->myPos - glm::vec2{ p->mySize };
					const glm::vec2 max = p->myPos + glm::vec2{ p->mySize };
					myGrid.Remove(min, max, p);
					myPoints.Free(*p);
				}
			}
			break;
			}
		}
	}

	constexpr static glm::vec3 kDepthColors[]{
		{1, 0, 0},
		{0, 1, 0},
		{0, 0, 1}
	};
	myGrid.ForEachCell([&](glm::vec2 aMin, glm::vec2 aMax, std::span<Point*> aPoints)
	{
		const glm::vec3 min{ aMin.x, 0, aMin.y };
		const glm::vec3 max{ aMax.x, 0, aMax.y };
		// TODO: make a AddRect call
		const glm::vec3 color = kDepthColors[0];
		debugDrawer.AddLine(min, { min.x, 0, max.z }, color);
		debugDrawer.AddLine(min, { max.x, 0, min.z }, color);
		debugDrawer.AddLine({ min.x, 0, max.z }, max, color);
		debugDrawer.AddLine({ max.x, 0, min.z }, max, color);

		for (Point* p : aPoints)
		{
			debugDrawer.AddCircle({ p->myPos.x, 0, p->myPos.y }, { 0, 1, 0 }, p->mySize, color);
		}
	});
}