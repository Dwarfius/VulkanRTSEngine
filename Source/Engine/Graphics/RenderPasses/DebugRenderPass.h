#pragma once

#include <Graphics/RenderPass.h>
#include <Graphics/Camera.h>
#include <Graphics/Resources/Model.h>
#include <Core/Vertex.h>

class DebugDrawer;
class Graphics;
class Pipeline;
class GPUModel;
class GPUPipeline;
class UniformBuffer;

class DebugRenderPass final : public RenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DebugRenderPass");

	DebugRenderPass(Graphics& aGraphics, Handle<Pipeline> aPipeline);
	~DebugRenderPass();

	std::string_view GetTypeName() const override { return "DebugRenderPass"; }

private:
	Id GetId() const override { return kId; };
	void Execute(Graphics& aGraphics) override;

	bool IsReady() const;
	void SetCamera(uint32_t aCamIndex, const Camera& aCamera, Graphics& aGraphics);
	void AddDebugDrawer(uint32_t aCamIndex, const DebugDrawer& aDebugRawer);
	
	RenderContext CreateContext(Graphics& aGraphics) const;

	Handle<GPUPipeline> myPipeline;
	struct PerCameraModel
	{
		using UploadDesc = Model::UploadDescriptor<PosColorVertex>;
		Handle<GPUModel> myModel;
		Camera myCamera;
		UploadDesc myDesc;
		Handle<UniformBuffer> myBuffer;
	};
	std::vector<PerCameraModel> myCameraModels;
};