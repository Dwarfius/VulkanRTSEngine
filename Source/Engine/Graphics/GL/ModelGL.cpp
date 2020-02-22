#include "Precomp.h"
#include "ModelGL.h"

#include <Graphics/Resources/Model.h>

ModelGL::ModelGL()
	: ModelGL(PrimitiveType::Lines, UsageType::Static, 0, false)
{
}

ModelGL::ModelGL(PrimitiveType aPrimType, UsageType aUsage, int aVertType, bool aIsIndexed)
	: myVAO(0)
	, myVBO(0)
	, myEBO(0)
	, myPrimitiveCount(0)
	, myVertCount(0)
	, myIndexCount(0)
	, myUsage(aUsage)
	, myVertType(aVertType)
	, myIsIndexed(aIsIndexed)
{
	switch (aPrimType)
	{
	case PrimitiveType::Lines:	   myDrawMode = GL_LINES; break;
	case PrimitiveType::Triangles: myDrawMode = GL_TRIANGLES; break;
	default: ASSERT(false);
	}
}

void ModelGL::Bind()
{
	ASSERT_STR(myVAO, "Model's GL resources haven't been initialized!");

	glBindVertexArray(myVAO);
}

void ModelGL::OnCreate(Graphics& aGraphics)
{
	ASSERT_STR(!myVBO, "Already have a buffer allocated!");

	const Model* model = myResHandle.Get<const Model>();
	myIsIndexed = model->GetIndexCount() > 0;

	glGenVertexArrays(1, &myVAO);
	glBindVertexArray(myVAO);

	if (myIsIndexed)
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

	myVertType = model->GetVertexType();
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

	switch (model->GetPrimitiveType())
	{
	case PrimitiveType::Lines:		myDrawMode = GL_LINES; break;
	case PrimitiveType::Triangles:	myDrawMode = GL_TRIANGLES; break;
	default: ASSERT(false);
	}

	glBindVertexArray(0);
}

bool ModelGL::OnUpload(Graphics& aGraphics)
{
	const Model* model = static_cast<const Model*>(myResHandle.Get());
	
	ASSERT_STR(myVAO, "Don't have a buffer alocated!");

	glBindVertexArray(myVAO);

	// first need to count how many vertices and indices are there in total
	// and adjust internal buffers if there aren't enough
	char vertsPerPrim = 0;
	switch (model->GetPrimitiveType())
	{
	case PrimitiveType::Lines:		vertsPerPrim = 2; break;
	case PrimitiveType::Triangles:	vertsPerPrim = 3; break;
	default: ASSERT(false);
	}
	const size_t newPrimCount = model->GetVertexCount() / vertsPerPrim;
	const size_t vertCount = model->GetVertexCount();
	const size_t indexCount = model->GetIndexCount();

	ASSERT_STR((indexCount != 0 && myEBO) || (indexCount == 0 && !myEBO), 
		"Didn't have indices for an index buffer!");

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
	if (vertCount > 0)
	{
		UploadVertices(model->GetVertices(), vertCount, 0);
	}

	if (indexCount > 0)
	{
		UploadIndices(model->GetIndices(), indexCount, 0);
	}

	myPrimitiveCount = newPrimCount;
	myCenter = model->GetCenter();
	myRadius = model->GetSphereRadius();

	return true;
}

void ModelGL::OnUnload(Graphics& aGraphics)
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

	uint32_t usage = myUsage == UsageType::Static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	size_t vertSize = Model::GetVertexSize(myVertType);

	glBindBuffer(GL_ARRAY_BUFFER, myVBO);
	glBufferData(GL_ARRAY_BUFFER, vertSize * aTotalCount, NULL, usage);

	myVertCount = aTotalCount;
}

void ModelGL::AllocateIndices(size_t aTotalCount)
{
	ASSERT_STR(myEBO, "Tried to allocate indices for missing buffer!");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myEBO);
	uint32_t usage = myUsage == UsageType::Static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Model::IndexType) * aTotalCount, NULL, GL_STATIC_DRAW);
	myIndexCount = aTotalCount;
}

void ModelGL::UploadVertices(const void* aVertices, size_t aVertexCount, size_t anOffset)
{
	ASSERT_STR(myVBO, "Tried to upload vertices for missing buffer!");
	ASSERT_STR(aVertexCount > 0 && aVertices != nullptr, "Missing vertices!");

	size_t vertSize = Model::GetVertexSize(myVertType);

	glBindBuffer(GL_ARRAY_BUFFER, myVBO);
	glBufferSubData(GL_ARRAY_BUFFER, anOffset * vertSize, aVertexCount * vertSize, aVertices);
}

void ModelGL::UploadIndices(const Model::IndexType* aIndices, size_t aIndexCount, size_t anOffset)
{
	ASSERT_STR(myEBO, "Tried to upload indices for missing buffer!");
	ASSERT_STR(aIndexCount > 0 && aIndices != nullptr, "Missing indices!");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myEBO);
	size_t indSize = sizeof(Model::IndexType);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, anOffset * indSize, aIndexCount * indSize, aIndices);
}