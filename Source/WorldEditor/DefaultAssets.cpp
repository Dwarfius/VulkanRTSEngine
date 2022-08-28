#include "Precomp.h"
#include "DefaultAssets.h"

#include <Engine/Game.h>

#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Resources/Pipeline.h>

#include <Core/Resources/AssetTracker.h>

DefaultAssets::DefaultAssets(Game& aGame)
{
	CreatePlane(aGame);
	CreateSphere(aGame);
	CreateBox(aGame);
	CreateCylinder(aGame);
	CreateCone(aGame);

	AssetTracker& assetTracker = aGame.GetAssetTracker();
	myUVTexture = assetTracker.GetOrCreate<Texture>("UVTest.img");
	myPipeline = assetTracker.GetOrCreate<Pipeline>(
		"Engine/default.ppl"
	);
}

void DefaultAssets::CreatePlane(Game& aGame)
{
	Vertex verts[]{
		{ { -0.5f, 0, -0.5f }, { 0, 0 }, { 0, 1, 0 } },
		{ { 0.5f, 0, -0.5f }, { 1, 0 }, { 0, 1, 0 } },
		{ { -0.5f, 0, 0.5f }, { 0, 1 }, { 0, 1, 0 } },
		{ { 0.5f, 0, 0.5f }, { 1, 1 }, { 0, 1, 0 } }
	};
	Model::IndexType indices[]{
		0, 2, 1,
		2, 3, 1
	};
	myPlane = new Model(
		Model::PrimitiveType::Triangles,
		std::span{ verts },
		std::span{ indices }
	);
	myPlane->SetAABB({ -0.5f, 0.f, -0.5f }, { 0.5f, 0.f, 0.5f });
	aGame.GetAssetTracker().AssignDynamicId(*myPlane.Get());
}

void DefaultAssets::CreateSphere(Game& aGame)
{
	constexpr uint8_t kVertSegments = 32;
	constexpr uint8_t kHorSegments = kVertSegments + 1;

	std::vector<Vertex> vertices;
	constexpr float kOneOverHorSegments = 1.f / kHorSegments;
	for (uint8_t x = 0; x <= kHorSegments; x++)
	{
		const float u = kOneOverHorSegments * x + kOneOverHorSegments / 2.f;
		vertices.emplace_back(
			glm::vec3{ 0, 1, 0 },
			glm::vec2{ 1.f - u, 0.f },
			glm::vec3{ 0, 1, 0 }
		);
	}
	constexpr float kOveOverVertSegments = 1.f / kVertSegments;
	for (uint8_t j = 0; j < kVertSegments - 1; j++)
	{
		// Vertical angle, covers half-circle
		const float alpha = glm::pi<float>() / 2.f
			+ glm::pi<float>() / kVertSegments * (j + 1);
		const float y = glm::sin(alpha);
		const float cosA = glm::cos(alpha);
		for (uint8_t i = 0; i <= kHorSegments; i++)
		{
			// Horizontal angle, covers full circle
			const float beta = glm::two_pi<float>() / kHorSegments * i;
			const float x = cosA * glm::cos(beta);
			const float z = cosA * glm::sin(beta);

			const glm::vec3 pos{ x, y, z };

			const float u = kOneOverHorSegments * i;
			const float v = kOveOverVertSegments * (j + 1);
			const glm::vec2 uv = { 1.f - u, v };
			vertices.emplace_back(
				pos,
				uv,
				pos // since it's a unit sphere, we can reuse pos as normal
			);
		}
	}
	for (uint8_t x = 0; x <= kHorSegments; x++)
	{
		const float u = kOneOverHorSegments * x + kOneOverHorSegments / 2.f;
		vertices.emplace_back(
			glm::vec3{ 0, -1, 0 },
			glm::vec2{ 1.f - u, 1.f },
			glm::vec3{ 0, -1, 0 }
		);
	}

	std::vector<Model::IndexType> indices;
	// top fan
	for (uint8_t x = 0; x < kHorSegments; x++)
	{
		indices.push_back(x);
		indices.push_back(kHorSegments + 1 + (x + 1));
		indices.push_back(kHorSegments + 1 + x);
	}
	// quads
	for (uint8_t y = 0; y < kVertSegments - 2; y++)
	{
		for (uint8_t x = 0; x <= kHorSegments; x++)
		{
			constexpr Model::IndexType kTopOffset = kHorSegments;
			constexpr Model::IndexType width = kHorSegments + 1;
			const Model::IndexType topLeft = kTopOffset + y * width + x;
			const Model::IndexType topRight = kTopOffset + y * width + x + 1;
			const Model::IndexType bottomLeft = kTopOffset + (y + 1) * width + x;
			const Model::IndexType bottomRight = kTopOffset + (y + 1) * width + x + 1;

			// top left triangle
			indices.push_back(topLeft);
			indices.push_back(topRight);
			indices.push_back(bottomLeft);

			// bottom right triangle
			indices.push_back(bottomLeft);
			indices.push_back(topRight);
			indices.push_back(bottomRight);
		}
	}
	// bottom fan
	for (uint8_t x = 0; x < kHorSegments; x++)
	{
		const Model::IndexType lastVert = static_cast<Model::IndexType>(vertices.size() - kHorSegments - 1);
		const Model::IndexType ringStart = lastVert - kHorSegments - 1;
		indices.push_back(lastVert + x);
		indices.push_back(ringStart + x);
		indices.push_back(ringStart + x + 1);
	}

	mySphere = new Model(
		Model::PrimitiveType::Triangles,
		std::span{ vertices },
		std::span{ indices }
	);
	mySphere->SetAABB({ -1.f, -1.f, -1.f }, { 1.f, 1.f, 1.f });
	mySphere->SetSphereRadius(1.f);
	aGame.GetAssetTracker().AssignDynamicId(*mySphere.Get());
}

void DefaultAssets::CreateBox(Game& aGame)
{
	constexpr float kDiagComp = 0.57735026f;
	Vertex verts[]{
		// front
		{ { -0.5f, -0.5f, 0.5f }, { 0, 1 }, { -kDiagComp, -kDiagComp, kDiagComp } },
		{ { 0.5f, -0.5f, 0.5f }, { 1, 1 }, { kDiagComp, -kDiagComp, kDiagComp } },
		{ { 0.5f, 0.5f, 0.5f }, { 1, 0 }, { kDiagComp, kDiagComp, kDiagComp } },
		{ { -0.5f, 0.5f, 0.5f }, { 0, 0 }, { -kDiagComp, kDiagComp, kDiagComp } },

		// back
		{ { -0.5f, -0.5f, -0.5f }, { 1, 1 }, { -kDiagComp, -kDiagComp, -kDiagComp } },
		{ { 0.5f, -0.5f, -0.5f }, { 0, 1 }, { kDiagComp, -kDiagComp, -kDiagComp } },
		{ { 0.5f, 0.5f, -0.5f }, { 0, 0 }, { kDiagComp, kDiagComp, -kDiagComp } },
		{ { -0.5f, 0.5f, -0.5f }, { 1, 0 }, { -kDiagComp, kDiagComp, -kDiagComp } }
	};
	Model::IndexType indices[]{
		0, 1, 2,
		2, 3, 0,
		1, 5, 6,
		6, 2, 1,
		7, 6, 5,
		5, 4, 7,
		4, 0, 3,
		3, 7, 4,
		4, 5, 1,
		1, 0, 4,
		3, 2, 6,
		6, 7, 3
	};
	myBox = new Model(
		Model::PrimitiveType::Triangles,
		std::span{ verts },
		std::span{ indices }
	);
	myBox->SetAABB({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f });
	aGame.GetAssetTracker().AssignDynamicId(*myBox.Get());
}

void DefaultAssets::CreateCylinder(Game& aGame)
{
	constexpr uint8_t kSides = 32;
	constexpr float kHeight = 1.f;

	std::vector<Vertex> vertices;
	for (uint8_t i = 0; i < kSides; i++)
	{
		const float currAngle = glm::two_pi<float>() / kSides * i;
		const float x = glm::cos(currAngle);
		const float z = glm::sin(currAngle);
		// top
		vertices.emplace_back(
			glm::vec3{ x, kHeight / 2.f, z },
			glm::vec2{ 0, 0 },
			glm::vec3{ 0, 0, 0 } // TODO
		);

		// bottom
		vertices.emplace_back(
			glm::vec3{ x, kHeight / -2.f, z },
			glm::vec2{ 0, 0 },
			glm::vec3{ 0, 0, 0 } // TODO
		);
	}

	std::vector<Model::IndexType> indices;
	// top circle
	for (uint8_t i = 0; i < kSides - 2; i++)
	{
		indices.push_back(0);
		indices.push_back((i + 2) * 2);
		indices.push_back((i + 1) * 2);
	}

	// middle
	for (uint8_t i = 0; i < kSides * 2; i += 2)
	{
		indices.push_back(i);
		indices.push_back((i + 2) % (kSides * 2));
		indices.push_back(i + 1);

		indices.push_back(i + 1);
		indices.push_back((i + 2) % (kSides * 2));
		indices.push_back((i + 3) % (kSides * 2));
	}

	// bottom circle
	for (uint8_t i = 0; i < kSides - 2; i++)
	{
		indices.push_back(1);
		indices.push_back(1 + (i + 1) * 2);
		indices.push_back(1 + (i + 2) * 2);
	}

	myCylinder = new Model(
		Model::PrimitiveType::Triangles,
		std::span{ vertices },
		std::span{ indices }
	);
	myCylinder->SetAABB({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f });
	aGame.GetAssetTracker().AssignDynamicId(*myCylinder.Get());
}

void DefaultAssets::CreateCone(Game& aGame)
{
	constexpr uint8_t kSides = 32;
	constexpr float kHeight = 1.f;

	std::vector<Vertex> vertices;
	vertices.emplace_back(
		glm::vec3{ 0, kHeight / 2.f, 0 },
		glm::vec2{ 0, 0 },
		glm::vec3{ 0, 1, 0 }
	);
	for (uint8_t i = 0; i < kSides; i++)
	{
		const float currAngle = glm::two_pi<float>() / kSides * i;
		const float x = glm::cos(currAngle);
		const float z = glm::sin(currAngle);
		// bottom
		vertices.emplace_back(
			glm::vec3{ x, kHeight / -2.f, z },
			glm::vec2{ 0, 0 },
			glm::vec3{ 0, 0, 0 } // TODO
		);
	}

	std::vector<Model::IndexType> indices;
	// diagonal faces
	for (uint8_t i = 0; i < kSides; i++)
	{
		indices.push_back(0);
		indices.push_back(1 + (i + 1) % kSides);
		indices.push_back(1 + i);
	}

	// bottom circle
	for (uint8_t i = 0; i < kSides - 2; i++)
	{
		indices.push_back(1);
		indices.push_back(1 + (i + 1));
		indices.push_back(1 + (i + 2));
	}

	myCone = new Model(
		Model::PrimitiveType::Triangles,
		std::span{ vertices },
		std::span{ indices }
	);
	myCone->SetAABB({ -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f });
	aGame.GetAssetTracker().AssignDynamicId(*myCone.Get());
}