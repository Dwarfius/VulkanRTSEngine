#include "Common.h"
#include "Terrain.h"
#include "Graphics.h"

void Terrain::Generate(string aName, float aStep, glm::vec3 anOffset, float anYScale, float anUvScale)
{
	myStep = aStep;
	unsigned char* pixels;
	int channels;
	pixels = Graphics::LoadTexture(aName, &myWidth, &myHeight, &channels, 1);

	//first, creating the vertices
	myStart = anOffset;
	myEnd = myStart + glm::vec3((myWidth - 1) * myStep, 0, (myHeight - 1) * myStep);
	myVerts.reserve(myHeight * myWidth);
	for (int y = 0; y < myHeight; y++)
	{
		for (int x = 0; x < myWidth; x++)
		{
			Vertex v;
			v.myPos.x = x * myStep + anOffset.x;
			v.myPos.z = y * myStep + anOffset.z;
			v.myPos.y = pixels[y * myWidth + x] / 255.f * anYScale + anOffset.y;
			v.myUv.x = Wrap(static_cast<float>(x), anUvScale);
			v.myUv.y = Wrap(static_cast<float>(y), anUvScale);
			myVerts.push_back(v);
		}
	}

	myCenter = anOffset + (myEnd - myStart) / 2.f;
	myRange = static_cast<float>(glm::sqrt(myWidth * myWidth + myHeight * myHeight));

	//now creating indices
	myIndices.reserve((myHeight - 1) * (myWidth - 1) * 6);
	for (int y = 0; y < myHeight - 1; y++)
	{
		for (int x = 0; x < myWidth - 1; x++)
		{
			uint32_t tl = y * myWidth + x;
			uint32_t tr = tl + 1;
			uint32_t bl = tl + myWidth;
			uint32_t br = bl + 1;
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
	// + center cause rendering uses center anchors
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

	// getting the normal
	x -= xMin;
	y -= yMin;
	glm::vec3 botNorm = mix(v0.myNormal, v2.myNormal, x);
	glm::vec3 topNorm = mix(v1.myNormal, v3.myNormal, x);
	return glm::mix(botNorm, topNorm, y);
}

bool Terrain::Collides(glm::vec3 aPos, float aRange) const
{
	float myHeight = GetHeight(aPos);
	return myHeight > aPos.y - aRange;
}

void Terrain::Normalize()
{
	//holds the sum of all surface normals per vertex
	vector<glm::vec3> surfNormals(myIndices.size(), glm::vec3());
	//gotta update the faces
	for (int i = 0; i < myIndices.size(); i += 3)
	{
		int i1 = myIndices.at(i);
		glm::vec3 v1 = myVerts.at(i1).myPos;
		int i2 = myIndices.at(i + 1);
		glm::vec3 v2 = myVerts.at(i2).myPos;
		int i3 = myIndices.at(i + 2);
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