#include "Common.h"
#include "Terrain.h"

#include "Graphics.h"
#include "PhysicsEntity.h"
#include "PhysicsShapes.h"

#include <BulletCollision\CollisionDispatch\btCollisionObject.h>

void Terrain::Generate(string aName, float aStep, float anYScale, float anUvScale)
{
	myHeightsCache.clear();
	myVerts.clear();

	myStep = aStep;
	unsigned char* pixels;
	int channels;
	pixels = Graphics::LoadTexture(aName, &myWidth, &myHeight, &channels, 1);

	//first, creating the vertices
	myStart = glm::vec3(0);
	myEnd = myStart + glm::vec3((myWidth - 1) * myStep, 0, (myHeight - 1) * myStep);
	myVerts.reserve(myHeight * myWidth);
	for (int y = 0; y < myHeight; y++)
	{
		for (int x = 0; x < myWidth; x++)
		{
			Vertex v;
			v.myPos.x = x * myStep;
			v.myPos.z = y * myStep;
			v.myPos.y = pixels[y * myWidth + x] / 255.f * anYScale;
			v.myUv.x = Wrap(static_cast<float>(x), anUvScale);
			v.myUv.y = Wrap(static_cast<float>(y), anUvScale);
			myVerts.push_back(v);
		}
	}
	Graphics::FreeTexture(pixels);

	myCenter = (myEnd - myStart) / 2.f;
	myRange = static_cast<float>(glm::sqrt(myWidth * myWidth + myHeight * myHeight));

	//now creating indices
	myIndices.reserve((myHeight - 1) * (myWidth - 1) * 6);
	for (int y = 0; y < myHeight - 1; y++)
	{
		for (int x = 0; x < myWidth - 1; x++)
		{
			IndexType tl = y * myWidth + x;
			IndexType tr = tl + 1;
			IndexType bl = tl + myWidth;
			IndexType br = bl + 1;
			myIndices.push_back(tl);
			myIndices.push_back(bl);
			myIndices.push_back(tr);
			myIndices.push_back(br);
			myIndices.push_back(tr);
			myIndices.push_back(bl);
		}
	}

	Normalize();
}

float Terrain::GetHeight(glm::vec3 pos) const
{
	// finding the relative position
	// + center which will bring it in world space
	float x = (pos.x + myCenter.x) / myStep;
	float y = (pos.z + myCenter.z) / myStep;

	// just to be safe - it's clamped to the edges
	x = glm::clamp(x, 0.f, myWidth - 2.f);
	y = glm::clamp(y, 0.f, myHeight - 2.f);

	// printf("[Info] %f %f for (%f, %f), where start (%f, %f)\n", x, y, pos.x, pos.z, center.x, center.z);

	// getting the actual vert indices
	int xMin = static_cast<int>(floor(x));
	int xMax = xMin + 1;
	int yMin = static_cast<int>(floor(y));
	int yMax = yMin + 1;

	// getting vertices for lerping
	Vertex v0 = myVerts[yMin * myWidth + xMin];
	Vertex v1 = myVerts[yMax * myWidth + xMin];
	Vertex v2 = myVerts[yMin * myWidth + xMax];
	Vertex v3 = myVerts[yMax * myWidth + xMax];

	// getting the height
	x -= xMin;
	y -= yMin;
	float botHeight = glm::mix(v0.myPos.y, v2.myPos.y, x);
	float topHeight = glm::mix(v1.myPos.y, v3.myPos.y, x);
	return glm::mix(botHeight, topHeight, y);
}

glm::vec3 Terrain::GetNormal(glm::vec3 pos) const
{
	// finding the relative position
	// + center cause rendering uses center anchors
	float x = (pos.x + myCenter.x) / myStep;
	float y = (pos.z + myCenter.z) / myStep;

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
	Vertex v0 = myVerts[yMin * myWidth + xMin];
	Vertex v1 = myVerts[yMax * myWidth + xMin];
	Vertex v2 = myVerts[yMin * myWidth + xMax];
	Vertex v3 = myVerts[yMax * myWidth + xMax];

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

	// need to recollect the vertices in WS, and cache them
	const uint32_t count = myWidth * myHeight;
	myHeightsCache.reserve(count);
	float minHeight = FLT_MAX;
	float maxHeight = FLT_MIN;
	for (const Vertex& vert : myVerts)
	{
		myHeightsCache.push_back(vert.myPos.y);
		minHeight = glm::min(minHeight, vert.myPos.y);
		maxHeight = glm::max(maxHeight, vert.myPos.y);
	}

	shared_ptr<PhysicsShapeHeightfield> myShape = make_shared<PhysicsShapeHeightfield>(myWidth, myHeight, myHeightsCache, minHeight, maxHeight);
	myShape->SetScale(glm::vec3(myStep, 1.f, myStep));
	// Bullet uses AABB center as transform pivot, so have to offset it to center
	glm::mat4 transform = glm::translate(glm::vec3(0.f, (maxHeight + minHeight) / 2.f, 0.f)); 
	shared_ptr<PhysicsEntity> myPhysicsEntity = make_shared<PhysicsEntity>(0.f, myShape, transform);
	myPhysicsEntity->SetCollisionFlags(myPhysicsEntity->GetCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
	return myPhysicsEntity;
}

void Terrain::Normalize()
{
	//holds the sum of all surface normals per vertex
	vector<glm::vec3> surfNormals(myIndices.size(), glm::vec3());
	//gotta update the faces
	for (int i = 0; i < myIndices.size(); i += 3)
	{
		size_t i1 = myIndices.at(i);
		glm::vec3 v1 = myVerts.at(i1).myPos;
		size_t i2 = myIndices.at(i + 1);
		glm::vec3 v2 = myVerts.at(i2).myPos;
		size_t i3 = myIndices.at(i + 2);
		glm::vec3 v3 = myVerts.at(i3).myPos;

		//calculating the surf normal
		glm::vec3 u = v2 - v1;
		glm::vec3 v = v3 - v1;

		glm::vec3 normal;
		normal.x = u.y * v.z - u.z * v.y;
		normal.y = u.z * v.x - u.x * v.z;
		normal.z = u.x * v.y - u.y * v.x;

		surfNormals[i1] += normal;
		surfNormals[i2] += normal;
		surfNormals[i3] += normal;
	}

	for (int vertInd = 0; vertInd < myVerts.size(); vertInd++)
	{
		myVerts.at(vertInd).myNormal = normalize(surfNormals[vertInd]);
	}
}

float Terrain::Wrap(float aVal, float aRange) const
{
	float cosA = cos((aVal / aRange) * glm::pi<float>() / 2);
	float normalized = (cosA + 1) / 2;
	return normalized;
}