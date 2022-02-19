#include "Precomp.h"
#include "HexSolver.h"

#include <Engine/Game.h>
#include <Engine/Components/VisualComponent.h>
#include <Engine/Systems/ImGUI/ImGUISystem.h>
#include <Engine/Graphics/Adapters/AdapterSourceData.h>

#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/UniformAdapterRegister.h>

#include <random>

void HexSolver::Init(Game& aGame)
{
	InitGrid(aGame);
}

void HexSolver::Update(Game& aGame, float aDeltaTime)
{
	{
		std::lock_guard lock(aGame.GetImGUISystem().GetMutex());
		if (ImGui::Begin("Hex Solver"))
		{
			ImGui::InputInt("Size", &mySize);
			ImGui::InputFloat("Obstacle%", &myObstChance);
			if (ImGui::Button("Update"))
			{
				InitGrid(aGame);
			}
			ImGui::Separator();

			ImGui::InputInt2("Start", glm::value_ptr(myStart));
			ImGui::InputInt2("End", glm::value_ptr(myEnd));
			ImGui::InputInt("Steps", &mySteps);
			if (ImGui::Button("Solve"))
			{
				Solve();
			}
		}
		ImGui::End();
	}
}

void HexSolver::InitGrid(Game& aGame)
{
	std::random_device dev;
	std::mt19937 generator(dev());
	std::uniform_real_distribution<float> sampler;

	myGrid.resize(mySize * mySize);
	for(int i=0; i<myGrid.size(); i++)
	{
		myGrid[i] = sampler(generator) <= myObstChance;
	}

	AssetTracker& assetTracker = aGame.GetAssetTracker();
	Handle<Model> hexModel = assetTracker.GetOrCreate<Model>("Hexagon/HexShape.model");
	Handle<Texture> hexTexture = assetTracker.GetOrCreate<Texture>("Hexagon/HexTexture.img");
	Handle<Pipeline> tintPipeline = assetTracker.GetOrCreate<Pipeline>("Hexagon/Tint.ppl");

	while(myGameObjects.size() > mySize * mySize)
	{
		aGame.RemoveGameObject(myGameObjects[myGameObjects.size() - 1]);
		myGameObjects.pop_back();
	}

	constexpr float kXOffset = 1.75f;
	constexpr float kYOffset = 1.5f;
	const glm::vec3 startOffset{ 2, 0, 2 };
	for (int i = 0; i < mySize * mySize; i++)
	{
		Transform transf;
		const int x = i % mySize;
		const int y = i / mySize;
		const float hexOffset = (y % 2 == 1) ? kXOffset / 2 : 0;
		transf.SetPos(glm::vec3{ x * kXOffset + hexOffset, 0, y * kYOffset });
		transf.Translate(startOffset);

		if (i < myGameObjects.size())
		{
			GameObject* go = myGameObjects[i].Get();
			go->SetWorldTransform(transf);
			HexComponent* hexComp = go->GetComponent<HexComponent>();
			hexComp->myIsWall = myGrid[i];
			hexComp->myIsStart = false;
			hexComp->myIsEnd = false;
			hexComp->myIsPath = false;
		}
		else
		{
			Handle<GameObject> hexGO = new GameObject(transf);

			VisualComponent* visualComp = hexGO->AddComponent<VisualComponent>();
			visualComp->SetModel(hexModel);
			visualComp->SetTextureCount(1);
			visualComp->SetTexture(0, hexTexture);
			visualComp->SetPipeline(tintPipeline);

			HexComponent* hexComp = hexGO->AddComponent<HexComponent>();
			hexComp->myIsWall = myGrid[i];

			myGameObjects.push_back(hexGO);
			aGame.AddGameObject(hexGO);
		}
	}
}

void HexSolver::Solve()
{
	// resetting visuals from last attempt
	for (Handle<GameObject>& go : myGameObjects)
	{
		HexComponent* hexComp = go->GetComponent<HexComponent>();
		hexComp->myIsStart = false;
		hexComp->myIsEnd = false;
		hexComp->myIsPath = false;
	}

	struct FloodFillStep
	{
		int myIndex; // index into our grid
		int myParent; // index of parent in history (see bellow)
		int mySteps; // how many steps are left
	};
	std::queue<FloodFillStep> steps; // tiles to explore
	std::vector<FloodFillStep> history; // history of tiles explored
	// tiles already visited - this can 
	// be merged with above for larger grids
	std::vector<bool> alreadyVisited; 
	alreadyVisited.resize(mySize * mySize);
	// convenience function for flooding into an (aX, aY) tile
	auto TryAdd = [&](int aX, int aY, int aParent, int aEnergy)
	{
		const int index = aY * mySize + aX;
		if (aX >= 0 && aX < mySize
			&& aY >= 0 && aY < mySize
			&& aEnergy >= 0
			&& !alreadyVisited[index]
			&& !myGrid[index]) // don't visit if it's a wall
		{
			steps.push({ index, aParent, aEnergy });
			alreadyVisited[index] = true;
		}
	};
	TryAdd(myStart.x, myStart.y, -1, mySteps);
	bool found = false;
	while (!steps.empty())
	{
		FloodFillStep step = steps.front();
		steps.pop();
		history.push_back(step);

		const int x = step.myIndex % mySize;
		const int y = step.myIndex / mySize;
		const int energy = step.mySteps;

		if (x == myEnd.x && y == myEnd.y)
		{
			found = true;
			break;
		}

		// hex has 6 neighbors
		int parentInd = static_cast<int>(history.size() - 1);
		// horizontal neighbors
		TryAdd(x - 1, y, parentInd, energy - 1);
		TryAdd(x + 1, y, parentInd, energy - 1);
		if (y % 2 == 0) 
		{
			// in hex grid every other row is shifted to the right
			// in this case, neighbors bellow and above are not shifted
			// so look-up in the square grid with offset to left
			TryAdd(x, y - 1, parentInd, energy - 1);
			TryAdd(x - 1, y - 1, parentInd, energy - 1);
			TryAdd(x, y + 1, parentInd, energy - 1);
			TryAdd(x - 1, y + 1, parentInd, energy - 1);
		}
		else
		{
			// above and bellow rows are shifted
			// so adjust the lookup to the right as well
			TryAdd(x, y - 1, parentInd, energy - 1);
			TryAdd(x + 1, y - 1, parentInd, energy - 1);
			TryAdd(x, y + 1, parentInd, energy - 1);
			TryAdd(x + 1, y + 1, parentInd, energy - 1);
		}
	}

	// update visuals - mark start and end tiles for rendering
	HexComponent* hexComp;
	hexComp = myGameObjects[myStart.y * mySize + myStart.x]->GetComponent<HexComponent>();
	hexComp->myIsStart = true;
	hexComp = myGameObjects[myEnd.y * mySize + myEnd.x]->GetComponent<HexComponent>();
	hexComp->myIsEnd = true;

	if (found)
	{
		// reconstruct the path by following
		// the history from end to start
		FloodFillStep step = history[history.size() - 1];
		while (step.myParent != -1)
		{
			hexComp = myGameObjects[step.myIndex]->GetComponent<HexComponent>();
			hexComp->myIsPath = true; // mark tiles as path for rendering
			step = history[step.myParent];
		}
	}
}

void TintAdapter::FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB)
{
	const UniformAdapterSource& data = static_cast<const UniformAdapterSource&>(aData);
	const HexComponent* hexComp = data.myGO->GetComponent<HexComponent>();
	
	constexpr glm::vec3 kFreeColor = glm::vec3{ 1, 1, 1 };
	constexpr glm::vec3 kWallColor = glm::vec3{ 0.2f, 0.2f, 0.2f };
	constexpr glm::vec3 kStartColor = glm::vec3{ 0.2f, 1.f, 0.2f };
	constexpr glm::vec3 kEndColor = glm::vec3{ 0, 1, 0 };
	constexpr glm::vec3 kPathColor = glm::vec3{ 0, 0, 1 };

	glm::vec3 color = kFreeColor;
	if (hexComp->myIsWall)
	{
		color = kWallColor;
	}
	else if (hexComp->myIsStart)
	{
		color = kStartColor;
	}
	else if (hexComp->myIsEnd)
	{
		color = kEndColor;
	}
	else if (hexComp->myIsPath)
	{
		color = kPathColor;
	}

	aUB.SetUniform(0, 0, color);
}