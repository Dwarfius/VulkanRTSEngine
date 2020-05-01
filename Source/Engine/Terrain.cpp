#include "Precomp.h"
#include "Terrain.h"

#include <Core/Algos/DiamondSquareAlgo.h>

#include <Physics/PhysicsShapes.h>

#include <Graphics/Resources/Texture.h>

Terrain::Terrain()
	: myModel()
	, myWidth(0)
	, myHeight(0)
	, myStep(0.f)
	, myYScale(0.f)
{
}

void Terrain::Load(Handle<Texture> aTexture, float aStep, float anYScale)
{
	myYScale = anYScale;
	myStep = aStep;
	myTexture = aTexture;

	bool hasLoaded = false;
	myTexture->ExecLambdaOnLoad([=, &hasLoaded](const Resource* aRes) {
		const Texture* texture = static_cast<const Texture*>(aRes);
		ASSERT_STR(texture->GetFormat() == Texture::Format::UNorm_R, "Texture must have single channel format!");
		
		using PixelType = unsigned char;
		const PixelType* pixels = texture->GetPixels();
		constexpr float kMaxPixelVal = std::numeric_limits<typename PixelType>::max();

		myWidth = texture->GetWidth();
		myHeight = texture->GetHeight();

		// variables for calculating extents
		float minHeight;
		float maxHeight;
		// setting up the vertices
		const size_t vertCount = myHeight * myWidth;
		Vertex* vertices = GenerateVerticesFromData(glm::uvec2(myWidth, myHeight), myStep,
			[=](size_t anIndex) {
				return pixels[anIndex] / kMaxPixelVal * myYScale;
			}, minHeight, maxHeight
		);

		//now creating indices
		const size_t indexCount = (myHeight - 1) * (myWidth - 1) * 6;
		Model::IndexType* indices = GenerateIndices(glm::uvec2(myWidth, myHeight));

		Normalize(vertices, vertCount, indices, indexCount);

		myModel = CreateModel(glm::vec2(GetWidth(), GetDepth()), minHeight, maxHeight,
			vertices, vertCount,
			indices, indexCount);
		hasLoaded = true;
	});
	// TODO: this is a hack due to how rendering pipeline relies
	// on myModel's existence. In reality, there's no need for a model,
	// so I need refactor it a bit to selectively check what renderables are ready
	while (!hasLoaded && myTexture->GetState() != Resource::State::Error)
	{
		std::this_thread::yield();
	}
	ASSERT_STR(myTexture->GetState() != Resource::State::Error, "Failed to load terrain!");
}

void Terrain::Generate(glm::uvec2 aSize, float aStep, float anYScale)
{
	ASSERT_STR(Utils::CountSetBits(aSize.x) == 1
		&& Utils::CountSetBits(aSize.y) == 1, "Size must be power of 2!");
	ASSERT_STR(aSize.x > 2 && aSize.y > 2, "Small sizes aren't supported by the texture!");
	myWidth = aSize.x;
	myHeight = aSize.y;

	myYScale = anYScale;
	myStep = aStep;

	const size_t gridSize = std::max<size_t>(myWidth, myHeight) + 1;
	DiamondSquareAlgo dsAlgo(0, gridSize, 0.f, myYScale);
	float* heights = new float[gridSize*gridSize];
   	dsAlgo.Generate(heights);

	{
		myTexture = new Texture();
		myTexture->SetWidth(myWidth);
		myTexture->SetHeight(myHeight);
		// TODO: this isn't the most efficient, but ah well
		// because we store single bytes, we need to 
		// downscale/quantize floats to chars
		myTexture->SetFormat(Texture::Format::UNorm_R);
		
		using PixelType = unsigned char;
		constexpr float kMaxPixelVal = std::numeric_limits<typename PixelType>::max();
		PixelType* pixels = new PixelType[myWidth * myHeight];
		for (uint32_t y = 0; y < myHeight; y++)
		{
			for (uint32_t x = 0; x < myWidth; x++)
			{
				const float floatHeight = heights[y * gridSize + x];
				const PixelType charHeight = static_cast<PixelType>(
					floatHeight / myYScale * kMaxPixelVal
				);
				pixels[y * myWidth + x] = charHeight;
			}
		}
		myTexture->SetPixels(pixels);
	}

	// variables for calculating extents
	float minHeight;
	float maxHeight;
	// setting up the vertices
	const size_t vertCount = myHeight * myWidth;
	ASSERT_STR(vertCount < gridSize * gridSize, "Terrain Mesh will have missing vertices!");
	Vertex* vertices = GenerateVerticesFromData(glm::uvec2(myWidth, myHeight), myStep, [heights, this, gridSize](size_t anIndex) {
		// decompose the index and recalculate it based on the heighfield dimensions
		uint32_t x = anIndex % myWidth;
		uint32_t y = anIndex / myWidth;
		uint32_t newIndex = y * gridSize + x;
		return heights[newIndex];
	}, minHeight, maxHeight);
	delete[] heights;

	//now creating indices
	const size_t indexCount = (myHeight - 1) * (myWidth - 1) * 6;
	Model::IndexType* indices = GenerateIndices(glm::uvec2(myWidth, myHeight));

	Normalize(vertices, vertCount, indices, indexCount);

	myModel = CreateModel(glm::vec2(GetWidth(), GetDepth()), minHeight, maxHeight,
		vertices, vertCount,
		indices, indexCount);
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
	const uint32_t count = myWidth * myHeight;
	myHeightsCache.resize(count);
	
	const Vertex* vertBuffer = model.GetVertexStorage<Vertex>()->GetData();
	for (uint32_t index = 0; index < count; index++)
	{
		const Vertex& vert = vertBuffer[index];
		myHeightsCache[index] = vert.myPos.y;
	}

	const float minHeight = model.GetAABBMin().y;
	const float maxHeight = model.GetAABBMax().y;
	std::shared_ptr<PhysicsShapeHeightfield> myShape = std::make_shared<PhysicsShapeHeightfield>(myWidth, myHeight, myHeightsCache, minHeight, maxHeight);
	myShape->SetScale(glm::vec3(myStep, 1.f, myStep));
	return myShape;
}

Vertex* Terrain::GenerateVerticesFromData(const glm::uvec2& aSize, float aStep, const std::function<float(size_t)>& aReadCB,
											float& aMinHeight, float& aMaxHeight)
{
	aMinHeight = std::numeric_limits<float>::max();
	aMaxHeight = std::numeric_limits<float>::min();

	// first, creating the vertices
	const size_t vertCount = aSize.x * aSize.y;
	Vertex* vertices = new Vertex[vertCount];
	for (int y = 0; y < aSize.y; y++)
	{
		for (int x = 0; x < aSize.x; x++)
		{
			size_t index = y * aSize.x + x;
			// TODO: there's no longer any need to store actual vertices.
			// should replace with heightfield
			Vertex v;
			v.myPos.x = x * aStep;
			v.myPos.z = y * aStep;

			v.myPos.y = aReadCB(index);
			v.myUv.x = 0;
			v.myUv.y = 0;
			vertices[index] = v;

			// tracking min/max heights for AABB
			aMinHeight = std::min(aMinHeight, v.myPos.y);
			aMaxHeight = std::max(aMaxHeight, v.myPos.y);
		}
	}
	return vertices;
}

Model::IndexType* Terrain::GenerateIndices(const glm::uvec2& aSize)
{
	const size_t vertCount = aSize.x * aSize.y;
	const size_t indexCount = (aSize.y - 1) * (aSize.x - 1) * 6;
	Model::IndexType* indices = new Model::IndexType[indexCount];
	for (int y = 0; y < aSize.y - 1; y++)
	{
		for (int x = 0; x < aSize.x - 1; x++)
		{
			// defining 2 triangles - using bottom left as the anchor corner
			const size_t triangle = (y * (aSize.x - 1) + x) * 6;
			Model::IndexType bl = y * aSize.x + x;
			Model::IndexType br = bl + 1;
			Model::IndexType tl = bl + aSize.x;
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
	return indices;
}

Handle<Model> Terrain::CreateModel(const glm::vec2& aFullSize, float aMinHeight, float aMaxHeight,
	const Vertex* aVertices, size_t aVertCount,
	const Model::IndexType* aIndices, size_t aIndCount)
{
	Handle<Model> model = new Model(PrimitiveType::Triangles, Vertex::Type);

	const glm::vec3 aabbMin(0, aMinHeight, 0);
	const glm::vec3 aabbMax(aFullSize.x, aMaxHeight, aFullSize.y);
	model->SetAABB(aabbMin, aabbMax);

	// the largest dimension is the bounding sphere radius
	const float sphereRadius = std::max(std::max(aMaxHeight - aMinHeight, aFullSize.y), aFullSize.x);
	model->SetSphereRadius(sphereRadius);

	Model::UploadDescriptor<Vertex> uploadDesc;
	uploadDesc.myVertices = aVertices;
	uploadDesc.myVertCount = aVertCount;
	uploadDesc.myIndices = aIndices;
	uploadDesc.myIndCount = aIndCount;
	uploadDesc.myNextDesc = nullptr;
	model->Update(uploadDesc);
	return model;
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