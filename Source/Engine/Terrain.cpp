#include "Precomp.h"
#include "Terrain.h"

#include <Physics/PhysicsShapes.h>

#include <Graphics/Resources/Texture.h>
#include <Graphics/AssetTracker.h>

Terrain::Terrain()
	: myModel()
	, myWidth(0)
	, myHeight(0)
	, myStep(0.f)
	, myYScale(0.f)
{
}

void Terrain::Load(AssetTracker& anAssetTracker, const std::string& aName, float aStep, float anYScale, float anUvScale)
{
	myYScale = anYScale;
	myStep = aStep;

	using PixelType = unsigned char;
	PixelType* pixels = Texture::LoadFromDisk(aName, Texture::Format::UNorm_R, myWidth, myHeight);
	ASSERT_STR(pixels, "Failed to load image data!");
	constexpr float kMaxPixelVal = std::numeric_limits<typename PixelType>::max();

	// variables for calculating extents
	float minHeight = std::numeric_limits<float>::max();
	float maxHeight = std::numeric_limits<float>::min();

	// first, creating the vertices
	const size_t vertCount = myHeight * myWidth;
	Vertex* vertices = new Vertex[vertCount];
	const float startX = 0.f;
	const float startY = 0.f;
	for (int y = 0; y < myHeight; y++)
	{
		for (int x = 0; x < myWidth; x++)
		{
			size_t index = y * myWidth + x;
			// TODO: there's no longer any need to store actual vertices.
			// should replace with heightfields
			Vertex v;
			v.myPos.x = startX + x * myStep;
			v.myPos.z = startY + y * myStep;
			
			v.myPos.y = pixels[index] / kMaxPixelVal * myYScale;
			v.myUv.x = Wrap(static_cast<float>(x), anUvScale);
			v.myUv.y = Wrap(static_cast<float>(y), anUvScale);
			vertices[index] = v;

			// tracking min/max heights for AABB
			minHeight = std::min(minHeight, v.myPos.y);
			maxHeight = std::max(maxHeight, v.myPos.y);
		}
	}
	Texture::FreePixels(pixels);

	//now creating indices
	const size_t indexCount = (myHeight - 1) * (myWidth - 1) * 6;
	Model::IndexType* indices = new Model::IndexType[indexCount];
	for (int y = 0; y < myHeight - 1; y++)
	{
		for (int x = 0; x < myWidth - 1; x++)
		{
			// defining 2 triangles - using bottom left as the anchor corner
			size_t triangle = (y * (myWidth - 1) + x) * 6;
			Model::IndexType bl = y * myWidth + x;
			Model::IndexType br = bl + 1;
			Model::IndexType tl = bl + myWidth;
			Model::IndexType tr = tl + 1;

			ASSERT(bl < vertCount);
			ASSERT(br < vertCount);
			ASSERT(tl < vertCount);
			ASSERT(tr < vertCount);

			indices[triangle + 0] = bl;
			indices[triangle + 1] = tl;
			indices[triangle + 2] = tr;
			indices[triangle + 3] = bl;
			indices[triangle + 4] = tr;
			indices[triangle + 5] = br;
		}
	}

	Normalize(vertices, vertCount, indices, indexCount);

	myModel = new Model(PrimitiveType::Triangles, Vertex::Type);

	const float fullWidth = GetWidth();
	const float fullDepth = GetDepth();
	{
		const glm::vec3 aabbMin(startX, minHeight, startY);
		const glm::vec3 aabbMax(startX + fullWidth, maxHeight, startY + fullDepth);
		myModel->SetAABB(aabbMin, aabbMax);
	}
	{
		// the largest dimension is the bounding sphere radius
		const float sphereRadius = std::max(std::max(maxHeight - minHeight, fullDepth), fullWidth);
		myModel->SetSphereRadius(sphereRadius);
	}
	
	Model::UploadDescriptor<Vertex> uploadDesc;
	uploadDesc.myVertices = vertices;
	uploadDesc.myVertCount = vertCount;
	uploadDesc.myIndices = indices;
	uploadDesc.myIndCount = indexCount;
	uploadDesc.myNextDesc = nullptr;
	myModel->Update(uploadDesc);
}

float Terrain::GetHeight(glm::vec3 pos) const
{
	const Model& model = *myModel.Get();

	// finding the relative position
	// + center which will bring it in world space
	float x = (pos.x + model.GetCenter().x) / myStep;
	float y = (pos.z + model.GetCenter().z) / myStep;

	// just to be safe - it's clamped to the edges
	x = glm::clamp(x, 0.f, myWidth - 2.f);
	y = glm::clamp(y, 0.f, myHeight - 2.f);

	// getting the actual vert indices
	int xMin = static_cast<int>(floor(x));
	int xMax = xMin + 1;
	int yMin = static_cast<int>(floor(y));
	int yMax = yMin + 1;

	// getting vertices for lerping
	const Vertex* verts = model.GetVertexStorage<Vertex>()->GetData();
	Vertex v0 = verts[yMin * myWidth + xMin];
	Vertex v1 = verts[yMax * myWidth + xMin];
	Vertex v2 = verts[yMin * myWidth + xMax];
	Vertex v3 = verts[yMax * myWidth + xMax];

	// getting the height
	x -= xMin;
	y -= yMin;
	float botHeight = glm::mix(v0.myPos.y, v2.myPos.y, x);
	float topHeight = glm::mix(v1.myPos.y, v3.myPos.y, x);
	return glm::mix(botHeight, topHeight, y);
}

glm::vec3 Terrain::GetNormal(glm::vec3 pos) const
{
	const Model& model = *myModel.Get();

	// finding the relative position
	// + center cause rendering uses center anchors
	float x = (pos.x + model.GetCenter().x) / myStep;
	float y = (pos.z + model.GetCenter().z) / myStep;

	// just to be safe - it's clamped to the edges
	x = glm::clamp(x, 0.f, myWidth - 2.f); // TODO: figure out why -2
	y = glm::clamp(y, 0.f, myHeight - 2.f);

	// printf("[Info] %f %f for (%f, %f), where start (%f, %f)\n", x, y, pos.x, pos.z, center.x, center.z);

	// getting the actual vert indices
	int xMin = static_cast<int>(floor(x));
	int xMax = xMin + 1;
	int yMin = static_cast<int>(floor(y));
	int yMax = yMin + 1;

	// getting vertices for lerping
	const Vertex* verts = model.GetVertexStorage<Vertex>()->GetData();
	Vertex v0 = verts[yMin * myWidth + xMin];
	Vertex v1 = verts[yMax * myWidth + xMin];
	Vertex v2 = verts[yMin * myWidth + xMax];
	Vertex v3 = verts[yMax * myWidth + xMax];

	// getting the normal
	x -= xMin;
	y -= yMin;
	glm::vec3 botNorm = mix(v0.myNormal, v2.myNormal, x);
	glm::vec3 topNorm = mix(v1.myNormal, v3.myNormal, x);
	return glm::mix(botNorm, topNorm, y);
}

std::shared_ptr<PhysicsShapeHeightfield> Terrain::CreatePhysicsShape()
{
	ASSERT_STR(myHeightsCache.empty(), "Terrain physics entity re-initialization");

	const Model& model = *myModel.Get();

	// need to recollect the vertices in WS, and cache them
	const size_t count = myWidth * myHeight;
	myHeightsCache.reserve(count);
	float minHeight = model.GetAABBMin().y;
	float maxHeight = model.GetAABBMax().y;
	const Vertex* vertBuffer = model.GetVertexStorage<Vertex>()->GetData();
	for (size_t i=0; i<count; i++)
	{
		const Vertex& vert = vertBuffer[i];
		myHeightsCache.push_back(vert.myPos.y);
	}

	std::shared_ptr<PhysicsShapeHeightfield> myShape = std::make_shared<PhysicsShapeHeightfield>(myWidth, myHeight, myHeightsCache, minHeight, maxHeight);
	myShape->SetScale(glm::vec3(myStep, 1.f, myStep));
	return myShape;
}

void Terrain::Normalize(Vertex* aVertices, size_t aVertCount,
	Model::IndexType* aIndices, size_t aIndCount)
{
	ASSERT_STR(aVertCount != 0 && aIndCount != 0, "Missing data!");

	// holds the sum of all surface normals per vertex
	std::vector<glm::vec3> surfNormals(aVertCount, glm::vec3());
	// gotta update the faces
	for (int i = 0; i < aIndCount; i += 3)
	{
		size_t i1 = aIndices[i + 0];
		size_t i2 = aIndices[i + 1];
		size_t i3 = aIndices[i + 2];

		glm::vec3 v1 = aVertices[i1].myPos;
		glm::vec3 v2 = aVertices[i2].myPos;
		glm::vec3 v3 = aVertices[i3].myPos;

		// calculating the surf normal
		glm::vec3 u = v2 - v1;
		glm::vec3 v = v3 - v1;

		glm::vec3 normal = glm::cross(u, v);

		surfNormals[i1] += normal;
		surfNormals[i2] += normal;
		surfNormals[i3] += normal;
	}
	
	for (int vertInd = 0; vertInd < aVertCount; vertInd++)
	{
		aVertices[vertInd].myNormal = normalize(surfNormals[vertInd]);
	}
}

float Terrain::Wrap(float aVal, float aRange) const
{
	float cosA = cos((aVal / aRange) * glm::pi<float>() / 2);
	float normalized = (cosA + 1) / 2;
	return normalized;
}