#pragma once

#include <glm/gtx/hash.hpp>
#include <Core/VertexDescriptor.h>
#include <Core/Threading/AssertMutex.h>
#include <Graphics/UniformAdapter.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>

#include "../../Graphics/RenderPasses/GenericRenderPasses.h"

class GPUModel;
class GPUPipeline; 
class GPUTexture;

struct ImGUIVertex
{
	glm::vec2 myPos;
	glm::vec2 myUv;
	uint32_t myColor;

	ImGUIVertex() = default;
	constexpr ImGUIVertex(glm::vec2 aPos, glm::vec2 aUv, uint32_t aColor)
		: myPos(aPos)
		, myUv(aUv)
		, myColor(aColor)
	{
	}

	static constexpr VertexDescriptor GetDescriptor()
	{
		using ThisType = ImGUIVertex; // for copy-paste convenience
		return {
			sizeof(ThisType),
			3,
			{
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myPos) },
				{ VertexDescriptor::MemberType::F32, 3, offsetof(ThisType, myUv) },
				{ VertexDescriptor::MemberType::U32, 1, offsetof(ThisType, myColor) }
			}
		};
	}
};
static_assert(std::is_trivially_destructible_v<ImGUIVertex>, "Not trivially destructible! Engine relies on cheap deallocations of vertices");

namespace std
{
	template<> struct hash<ImGUIVertex>
	{
		size_t operator()(const ImGUIVertex& vertex) const
		{
			return ((hash<glm::vec2>()(vertex.myPos) ^
				(hash<glm::vec2>()(vertex.myUv) << 1)) >> 1) ^
				(hash<uint32_t>()(vertex.myColor) << 1);
		}
	};
}

struct ImGUIRenderParams : public IRenderPass::IParams
{
	int myScissorRect[4];
	Handle<GPUTexture> myTexture;
};

// Utility adapter if someone else wants to use it - we set the uniforms directly
// We have to provide it because UniformAdapters that are referenced
// in assets must be registered (even if they are not used)
class ImGUIAdapter : public UniformAdapter
{
	DECLARE_REGISTER(ImGUIAdapter);
public:
	struct ImGUIData : SourceData
	{
		glm::mat4 myOrthoProj;
	};

	void FillUniformBlock(const SourceData& aData, UniformBlock& aUB) const final;
};

class ImGUIRenderPass : public IRenderPass
{
	Handle<GPUPipeline> myPipeline;
	Handle<GPUModel> myModel;
	Handle<GPUTexture> myFontAtlas;
	RenderPassJob* myCurrentJob;
	std::shared_ptr<UniformBlock> myUniformBlock;
	std::vector<ImGUIRenderParams> myScheduledImGuiParams;
	AssertMutex myRenderJobMutex;

public:
	constexpr static uint32_t kId = Utils::CRC32("ImGUIRenderPass");
	ImGUIRenderPass(Handle<Pipeline> aPipeline, Handle<Texture> aFontAtlas, Graphics& aGraphics);

	Id GetId() const final { return kId; }

	void SetProj(const glm::mat4& aMatrix);
	void UpdateImGuiVerts(const Model::UploadDescriptor<ImGUIVertex>& aDescriptor);
	void AddImGuiRenderJob(const ImGUIRenderParams& aParams);
	bool IsReady() const;
	void SetDestFrameBuffer(std::string_view aFrameBuffer) { myDestFrameBuffer = aFrameBuffer; }

protected:
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;

	// We're using BeginPass to generate all work and schedule updates of assets (model)
	void BeginPass(Graphics& aGraphics) final;
	void SubmitJobs(Graphics& anInterface) final
	{
		// Do nothing because we already scheduled everything 
		// part of the BeginPass call
	}

	bool HasDynamicRenderContext() const final { return true; }

	std::string_view myDestFrameBuffer;
};