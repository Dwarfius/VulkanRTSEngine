#include "Precomp.h"
#include "Terrain.h"

#include <Physics/PhysicsEntity.h>
#include <Physics/PhysicsShapes.h>

// TODO: write own header for collision object flags
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>

#include <Core/Graphics/Texture.h>
#include <Core/Graphics/AssetTracker.h>

Terrain::Terrain()
	: myModel()
	, myWidth(0)
	, myHeight(0)
	, myStep(0.f)
{
}

void Terrain::Load(AssetTracker& anAssetTracker, string aName, float aStep, float anYScale, float anUvScale)
{
	myStep = aStep;
	unsigned char* pixels = Texture::LoadFromDisk(aName, Texture::Format::UNorm_R, myWidth, myHeight);

	// variables for calculating extents
	float minHeight = numeric_limits<float>::max();
	float maxHeight = numeric_limits<float>::min();

	//first, creating the vertices
	vector<Vertex> verts;
	verts.resize(myHeight * myWidth);
	for (int y = 0; y < myHeight; y++)
	{
		for (int x = 0; x < myWidth; x++)
		{
			size_t index = y * myWidth + x;
			Vertex v;
			v.myPos.x = x * myStep;
			v.myPos.z = y * myStep;
			v.myPos.y = pixels[index] / 255.f * anYScale;
			v.myUv.x = Wrap(static_cast<float>(x), anUvScale);
			v.myUv.y = Wrap(static_cast<float>(y), anUvScale);
			verts[index] = v;

			// tracking min/max heights for AABB
			minHeight = min(minHeight, v.myPos.y);
			maxHeight = max(maxHeight, v.myPos.y);
		}
	}
	Texture::FreePixels(pixels);

	//now creating indices
	vector<Model::IndexType> indices;
	indices.resize((myHeight - 1) * (myWidth - 1) * 6);
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
			indices[triangle + 0] = bl;
			indices[triangle + 1] = tl;
			indices[triangle + 2] = tr;
			indices[triangle + 3] = bl;
			indices[triangle + 4] = tr;
			indices[triangle + 5] = br;
		}
	}

	Normalize(verts, indices);

	myModel = anAssetTracker.Create<Model>();

	float fullWidth = (myWidth - 1) * myStep;
	float fullDepth = (myHeight - 1) * myStep;
	const glm::vec3 aabbMin(0, minHeight, 0);
	const glm::vec3 aabbMax(fullWidth, maxHeight, fullDepth);
	// the largest dimension is the bounding sphere radius
	const float sphereRadius = max(max(maxHeight - minHeight, fullDepth), fullWidth);

	myModel->SetData(move(verts), move(indices), aabbMin, aabbMax, sphereRadius);
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
	const Vertex* verts = model.GetVertices();
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
	const Vertex* verts = model.GetVertices();
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

shared_ptr<PhysicsEntity> Terrain::CreatePhysics()
{
	ASSERT_STR(myHeightsCache.empty(), "Terrain physics entity re-initialization");

	const Model& model = *myModel.Get();

	// need to recollect the vertices in WS, and cache them
	const size_t count = myWidth * myHeight;
	myHeightsCache.reserve(count);
	float minHeight = model.GetAABBMin().y;
	float maxHeight = model.GetAABBMax().y;
	for (size_t i=0; i<count; i++)
	{
		const Vertex& vert = model.GetVertices()[i];
		myHeightsCache.push_back(vert.myPos.y);
	}

	shared_ptr<PhysicsShapeHeightfield> myShape = make_shared<PhysicsShapeHeightfield>(myWidth, myHeight, myHeightsCache, minHeight, maxHeight);
	myShape->SetScale(glm::vec3(myStep, 1.f, myStep));
	shared_ptr<PhysicsEntity> myPhysicsEntity = make_shared<PhysicsEntity>(0.f, myShape, glm::mat4(1.f));
	myPhysicsEntity->SetCollisionFlags(myPhysicsEntity->GetCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
	return myPhysicsEntity;
}

void Terrain::Normalize(vector<Vertex>& aVertices, const vector<Model::IndexType>& aIndices)
{
	size_t vertCount = aVertices.size();
	size_t indexCount = aIndices.size();
	//holds the sum of all surface normals per vertex
	vector<glm::vec3> surfNormals(vertCount, glm::vec3());
	//gotta update the faces
	for (int i = 0; i < indexCount; i += 3)
	{
		size_t i1 = aIndices[i + 0];
		size_t i2 = aIndices[i + 1];
		size_t i3 = aIndices[i + 2];

		glm::vec3 v1 = aVertices[i1].myPos;
		glm::vec3 v2 = aVertices[i2].myPos;
		glm::vec3 v3 = aVertices[i3].myPos;

		//calculating the surf normal
		glm::vec3 u = v2 - v1;
		glm::vec3 v = v3 - v1;

		glm::vec3 normal = glm::cross(u, v);

		surfNormals[i1] += normal;
		surfNormals[i2] += normal;
		surfNormals[i3] += normal;
	}
	
	for (int vertInd = 0; vertInd < vertCount; vertInd++)
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