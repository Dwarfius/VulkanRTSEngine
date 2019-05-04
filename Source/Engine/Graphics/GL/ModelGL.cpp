#include "Precomp.h"
#include "ModelGL.h"

ModelGL::ModelGL()
	: GPUResource()
	, myVAO(0)
	, myVBO(0)
	, myEBO(0)
	, myDrawMode(0)
	, myPrimitiveCount(0)
	, myVertCount(0)
	, myIndexCount(0)
	, myUsage(GPUResource::Usage::Static)
	, myVertType(0)
{
}

ModelGL::~ModelGL()
{
	if (myVAO)
	{
		Unload();
	}
}

void ModelGL::Bind()
{
	ASSERT_STR(myVAO, "Model's GL resources haven't been initialized!");

	glBindVertexArray(myVAO);
}

void ModelGL::Create(any aDescriptor)
{
	const Model::CreateDescriptor& descriptor = 
		any_cast<const Model::CreateDescriptor&>(aDescriptor);

	ASSERT_STR(!myVBO, "Already have a buffer allocated!");

	glGenVertexArrays(1, &myVAO);
	glBindVertexArray(myVAO);

	if (descriptor.myIsIndexed)
	{
		uint32_t buffers[2];
		glGenBuffers(2, buffers);
		myVBO = buffers[0];
		myEBO = buffers[1];

		glBindBuffer(GL_ARRAY_BUFFER, myVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myEBO);
	}
	else
	{
		glGenBuffers(1, &myVBO);

		glBindBuffer(GL_ARRAY_BUFFER, myVBO);
	}

	myVertType = descriptor.myVertType;
	switch (myVertType)
	{
	case Vertex::Type:
		// tell the VAO that 0 is the position element
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)offsetof(Vertex, myPos));

		// uvs at 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)offsetof(Vertex, myUv));

		// normals at 2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void**)(offsetof(Vertex, myNormal)));
		break;
	case PosColorVertex::Type:
		// tell the VAO that 0 is the position element
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PosColorVertex), (void**)offsetof(PosColorVertex, myPos));

		// color is at 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PosColorVertex), (void**)offsetof(PosColorVertex, myColor));
		break;
	default:
		ASSERT_STR(false, "Unrecognized vertex type!");
		break;
	}

	switch (descriptor.myPrimitiveType)
	{
	case GPUResource::Primitive::Lines:		myDrawMode = GL_LINES; break;
	case GPUResource::Primitive::Triangles: myDrawMode = GL_TRIANGLES; break;
	default: ASSERT(false);
	}

	myUsage = descriptor.myUsage;
	glBindVertexArray(0);
}

bool ModelGL::Upload(any aDescriptor)
{
	const Model::UploadDescriptor& descriptor = 
		any_cast<const Model::UploadDescriptor&>(aDescriptor);
	
	ASSERT_STR(myVAO, "Don't have a buffer alocated!");

	glBindVertexArray(myVAO);

	// first need to count how many vertices and indices are there in total
	size_t newPrimCount = 0;
	size_t vertCount = 0;
	size_t indexCount = 0;
	for(const Model::UploadDescriptor* currDesc = &descriptor; 
		currDesc != nullptr; 
		currDesc = currDesc->myNextDesc)
	{
		vertCount += currDesc->myVertCount;
		indexCount += currDesc->myIndCount;
		newPrimCount += currDesc->myPrimitiveCount;
	}

	ASSERT_STR((indexCount != 0 && myEBO) || (indexCount == 0 && !myEBO), "Didn't have indices for an index buffer!");

	// Maybe the model has been dynamically modified - might need to grow.
	// Definitelly need to allocate if it's our first upload
	if (vertCount > myVertCount)
	{
		AllocateVertices(vertCount);
	}
	if (indexCount > myIndexCount)
	{
		AllocateIndices(indexCount);
	}
	
	// Now that we have enough allocated, we can start uploading data from
	// the descriptors
	size_t vertOffset = 0;
	size_t indOffset = 0;
	for (const Model::UploadDescriptor* currDesc = &descriptor;
		currDesc != nullptr;
		currDesc = currDesc->myNextDesc)
	{
		size_t currVertCount = currDesc->myVertCount;
		ASSERT_STR(currVertCount > 0, "Missing additional vertices for a model!");
		UploadVertices(currDesc->myVertices, currVertCount, vertOffset);
		vertOffset += currVertCount;

		if (indexCount > 0)
		{
			size_t currIndCount = currDesc->myIndCount;
			ASSERT_STR(currIndCount > 0, "Missing additional indices for a model!");
			UploadIndices(currDesc->myIndices, currIndCount, indOffset);
			indOffset += currIndCount;
		}
	}

	myPrimitiveCount = newPrimCount;

	return true;
}

void ModelGL::Unload()
{
	ASSERT_STR(myVAO, "Uninitialized Model detected!");
	if (myEBO)
	{
		const int size = 2;
		uint32_t buffersToDelete[size] = { myVBO, myEBO };
		glDeleteBuffers(size, buffersToDelete);
	}
	else
	{
		const int size = 1;
		glDeleteBuffers(1, &myVBO);
	}
	glDeleteVertexArrays(1, &myVAO);
}

void ModelGL::AllocateVertices(size_t aTotalCount)
{
	ASSERT_STR(myVBO, "Tried to allocate vertices for missing buffer!");

	uint32_t usage = myUsage == GPUResource::Usage::Static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	size_t vertSize = 0;
	switch (myVertType)
	{
	case Vertex::Type:
	{
		vertSize = sizeof(Vertex);
		break;
	}
	case PosColorVertex::Type:
	{
		vertSize = sizeof(PosColorVertex);
		break;
	}
	default:
		ASSERT(false);
	}

	glBindBuffer(GL_ARRAY_BUFFER, myVBO);
	glBufferData(GL_ARRAY_BUFFER, vertSize * aTotalCount, NULL, usage);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	myVertCount = aTotalCount;
}

void ModelGL::AllocateIndices(size_t aTotalCount)
{
	ASSERT_STR(myEBO, "Tried to allocate indices for missing buffer!");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myEBO);
	uint32_t usage = myUsage == GPUResource::Usage::Static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Model::IndexType) * aTotalCount, NULL, GL_STATIC_DRAW);
	myIndexCount = aTotalCount;
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ModelGL::UploadVertices(const void* aVertices, size_t aVertexCount, size_t anOffset)
{
	ASSERT_STR(myVBO, "Tried to upload vertices for missing buffer!");
	ASSERT_STR(aVertexCount > 0 && aVertices != nullptr, "Missing vertices!");

	size_t vertSize = 0;
	switch (myVertType)
	{
	case Vertex::Type:
		vertSize = sizeof(Vertex);
		break;
	case PosColorVertex::Type:
		vertSize = sizeof(PosColorVertex);
		break;
	default:
		ASSERT(false);
	}

	glBindBuffer(GL_ARRAY_BUFFER, myVBO);
	glBufferSubData(GL_ARRAY_BUFFER, anOffset * vertSize, aVertexCount * vertSize, aVertices);
	//glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ModelGL::UploadIndices(const Model::IndexType* aIndices, size_t aIndexCount, size_t anOffset)
{
	ASSERT_STR(myEBO, "Tried to upload indices for missing buffer!");
	ASSERT_STR(aIndexCount > 0 && aIndices != nullptr, "Missing indices!");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myEBO);
	size_t indSize = sizeof(Model::IndexType);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, anOffset * indSize, aIndexCount * indSize, aIndices);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}