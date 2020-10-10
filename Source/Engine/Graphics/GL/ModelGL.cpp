#include "Precomp.h"
#include "ModelGL.h"

#include <Graphics/Resources/Model.h>

ModelGL::ModelGL()
	: ModelGL(PrimitiveType::Lines, UsageType::Static, VertexDescriptor::GetEmpty(), false)
{
}

ModelGL::ModelGL(PrimitiveType aPrimType, UsageType aUsage, const VertexDescriptor& aVertDesc, bool aIsIndexed)
	: myVAO(0)
	, myVBO(0)
	, myEBO(0)
	, myVertCount(0)
	, myIndexCount(0)
	, myUsage(aUsage)
	, myVertDescriptor(aVertDesc)
	, myIsIndexed(aIsIndexed)
{
	switch (aPrimType)
	{
	case PrimitiveType::Lines:	   myDrawMode = GL_LINES; break;
	case PrimitiveType::Triangles: myDrawMode = GL_TRIANGLES; break;
	default: ASSERT(false); myDrawMode = GL_TRIANGLES;
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
	myIsIndexed = model->HasIndices();

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

	myVertDescriptor = model->GetVertexDescriptor();
	const GLsizei vertSize = static_cast<GLsizei>(myVertDescriptor.mySize);	
	const uint8_t vertMemberCount = myVertDescriptor.myMemberCount;
	for (uint8_t vertMemberInd = 0; vertMemberInd < vertMemberCount; vertMemberInd++)
	{
		const VertexDescriptor::MemberDescriptor memberDesc = myVertDescriptor.myMembers[vertMemberInd];
		glEnableVertexAttribArray(vertMemberInd);
		
		GLenum attribType = 0;
		switch (memberDesc.myType)
		{
			// TODO: use c++20 using-enum decl here
		case VertexDescriptor::MemberType::Bool: attribType = GL_UNSIGNED_BYTE; break;
		case VertexDescriptor::MemberType::U8: attribType = GL_UNSIGNED_BYTE; break;
		case VertexDescriptor::MemberType::S8: attribType = GL_BYTE; break;
		case VertexDescriptor::MemberType::U16: attribType = GL_UNSIGNED_SHORT; break;
		case VertexDescriptor::MemberType::S16: attribType = GL_SHORT; break;
		case VertexDescriptor::MemberType::F16: attribType = GL_HALF_FLOAT; break;
		case VertexDescriptor::MemberType::U32: attribType = GL_UNSIGNED_INT; break;
		case VertexDescriptor::MemberType::S32: attribType = GL_INT; break;
		case VertexDescriptor::MemberType::F32: attribType = GL_FLOAT; break;
		case VertexDescriptor::MemberType::F64: attribType = GL_DOUBLE; break;
		default: ASSERT(false);
		}
		const bool isIntegral = attribType != GL_FLOAT 
								&& attribType != GL_HALF_FLOAT 
								&& attribType != GL_DOUBLE;
		const GLint memberCompCount = memberDesc.myElemCount;
		const void* memberOffsetPtr = reinterpret_cast<void*>(memberDesc.myOffset);
		if (!isIntegral)
		{
			glVertexAttribPointer(vertMemberInd, memberCompCount, attribType, GL_FALSE, vertSize, memberOffsetPtr);
		}
		else
		{
			glVertexAttribIPointer(vertMemberInd, memberCompCount, attribType, vertSize, memberOffsetPtr);
		}
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
	char elemPerPrim = 0;
	switch (model->GetPrimitiveType())
	{
	case PrimitiveType::Lines:		elemPerPrim = 2; break;
	case PrimitiveType::Triangles:	elemPerPrim = 3; break;
	default: ASSERT(false);
	}
	const size_t vertCount = model->GetVertexCount();
	const size_t indexCount = model->GetIndexCount();
	size_t newPrimCount;
	if (myIsIndexed)
	{
		newPrimCount = indexCount;
	}
	else
	{
		newPrimCount = vertCount / elemPerPrim;
	}

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
		ASSERT_STR((indexCount && myEBO) || (!indexCount && !myEBO),
			"Didn't have indices for an index buffer!");
		UploadVertices(model->GetVertices(), vertCount, 0);
	}

	if (indexCount > 0)
	{
		UploadIndices(model->GetIndices(), indexCount, 0);
	}

	ASSERT_STR(newPrimCount < std::numeric_limits<uint32_t>::max(),
		"Primitive count ended up higher than rendering can support!");
	myPrimitiveCount = static_cast<uint32_t>(newPrimCount);
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

	const uint32_t usage = myUsage == UsageType::Static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	const size_t vertSize = myVertDescriptor.mySize;

	glBindBuffer(GL_ARRAY_BUFFER, myVBO);
	glBufferData(GL_ARRAY_BUFFER, vertSize * aTotalCount, NULL, usage);

	myVertCount = aTotalCount;
}

void ModelGL::AllocateIndices(size_t aTotalCount)
{
	ASSERT_STR(myEBO, "Tried to allocate indices for missing buffer!");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myEBO);
	uint32_t usage = myUsage == UsageType::Static ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Model::IndexType) * aTotalCount, NULL, usage);
	myIndexCount = aTotalCount;
}

void ModelGL::UploadVertices(const void* aVertices, size_t aVertexCount, size_t anOffset)
{
	ASSERT_STR(myVBO, "Tried to upload vertices for missing buffer!");
	ASSERT_STR(aVertexCount > 0 && aVertices != nullptr, "Missing vertices!");

	const size_t vertSize = myVertDescriptor.mySize;

	glBindBuffer(GL_ARRAY_BUFFER, myVBO);
	glBufferSubData(GL_ARRAY_BUFFER, anOffset * vertSize, aVertexCount * vertSize, aVertices);
}

void ModelGL::UploadIndices(const Model::IndexType* aIndices, size_t aIndexCount, size_t anOffset)
{
	ASSERT_STR(myEBO, "Tried to upload indices for missing buffer!");
	ASSERT_STR(aIndexCount > 0 && aIndices != nullptr, "Missing indices!");

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, myEBO);
	const size_t indSize = sizeof(Model::IndexType);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, anOffset * indSize, aIndexCount * indSize, aIndices);
}