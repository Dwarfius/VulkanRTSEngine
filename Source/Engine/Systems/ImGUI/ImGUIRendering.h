#pragma once

#include <glm/gtx/hash.hpp>
#include <Core/VertexDescriptor.h>
#include <Core/RWBuffer.h>
#include <Graphics/UniformAdapterRegister.h>
#include <Graphics/Resources/Pipeline.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/Resources/Texture.h>
#include <Graphics/Descriptor.h>
#include <Graphics/GraphicsConfig.h>

#include "../../Graphics/RenderPasses/GenericRenderPasses.h"

class GPUModel;
class GPUPipeline; 
class GPUTexture;
class UniformBuffer;

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

class ImGUIAdapter : RegisterUniformAdapter<ImGUIAdapter>
{
public:
	struct ImGUIData : AdapterSourceData
	{
		glm::mat4 myOrthoProj;
	};
	constexpr static Descriptor ourDescriptor{
		{ Descriptor::UniformType::Mat4 }
	};
	static void FillUniformBlock(const AdapterSourceData& aData, UniformBlock& aUB);
};

class ImGUIRenderPass final : public RenderPass
{
public:
	struct Params
	{
		uint32_t myOffset;
		uint32_t myCount;
		int myScissorRect[4];
		Handle<GPUTexture> myTexture;
	};

	struct ImGUIFrame
	{
		Model::UploadDescriptor<ImGUIVertex> myDesc;
		glm::mat4 myMatrix;
		std::vector<Params> myParams;
	};

	constexpr static uint32_t kId = Utils::CRC32("ImGUIRenderPass");
	ImGUIRenderPass(Handle<Pipeline> aPipeline, Handle<Texture> aFontAtlas, Graphics& aGraphics);

	Id GetId() const final { return kId; }

	bool IsReady() const;
	void SetDestFrameBuffer(std::string_view aFrameBuffer) { myDestFrameBuffer = aFrameBuffer; }

	std::string_view GetTypeName() const override { return "ImGUIRenderPass"; }

private:
	RenderContext CreateContext(Graphics& aGraphics) const;

	// We're using Execute to generate all work and schedule updates of assets (model)
	void Execute(Graphics& aGraphics) override;
	ImGUIFrame PrepareFrame();

	std::string_view myDestFrameBuffer;

	Handle<GPUPipeline> myPipeline;
	Handle<GPUModel> myModel;
	Handle<GPUTexture> myFontAtlas;
	RenderPassJob* myCurrentJob;
	Handle<UniformBuffer> myUniformBuffer;
};