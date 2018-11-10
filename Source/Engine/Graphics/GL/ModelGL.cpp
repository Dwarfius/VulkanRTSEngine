#include "Precomp.h"
#include "ModelGL.h"

ModelGL::ModelGL()
	: GPUResource()
	, myVAO(0)
	, myVBO(0)
	, myEBO(0)
	, myDrawMode(0)
	, myPrimitiveCount(0)
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

	// bind it in case it's a separate call
	glBindBuffer(GL_ARRAY_BUFFER, myVBO);

	// upload the data
	size_t vertCount = descriptor.myVertCount;
	uint32_t usage = myUsage == GPUResource::Usage::Static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	switch (myVertType)
	{
	case Vertex::Type:
	{
		const Vertex* vertices = (const Vertex*)descriptor.myVertices;
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertCount, vertices, usage);
		break;
	}
	case PosColorVertex::Type:
	{
		const PosColorVertex* vertices = (const PosColorVertex*)descriptor.myVertices;
		glBufferData(GL_ARRAY_BUFFER, sizeof(PosColorVertex) * vertCount, vertices, usage);
		break;
	}
	default:
		ASSERT(false);
	}

	size_t indexCount = descriptor.myIndCount;
	if (indexCount > 0)
	{
		ASSERT_STR(myEBO, "Tried to upload indices into missing buffer!");
		const Model::IndexType* indices = descriptor.myIndices;
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Model::IndexType) * indexCount, indices, GL_STATIC_DRAW);
	}
	else
	{
		ASSERT_STR(!myEBO, "Didn't have indices for an index buffer!");
	}

	myPrimitiveCount = descriptor.myPrimitiveCount;

	// we're done setting up the VBA, so unbind all resources
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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