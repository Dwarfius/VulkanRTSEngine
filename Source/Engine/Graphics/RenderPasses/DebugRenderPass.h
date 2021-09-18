#pragma once

#include <Graphics/RenderPass.h>
#include <Graphics/Camera.h>
#include <Graphics/Resources/Model.h>
#include <Graphics/UniformBlock.h>
#include <Core/Vertex.h>

class DebugDrawer;
class Graphics;
class Pipeline;
class GPUModel;
class GPUPipeline;

class DebugRenderPass : public IRenderPass
{
public:
	constexpr static uint32_t kId = Utils::CRC32("DebugRenderPass");

	DebugRenderPass(Graphics& aGraphics, Handle<Pipeline> aPipeline);
	~DebugRenderPass();

	bool IsReady() const;
	void SetCamera(uint32_t aCamIndex, const Camera& aCamera, Graphics& aGraphics);
	void AddDebugDrawer(uint32_t aCamIndex, const DebugDrawer& aDebugRawer);

private:
	Id GetId() const final { return kId; };
	bool HasDynamicRenderContext() const final { return true; }
	void PrepareContext(RenderContext& aContext, Graphics& aGraphics) const final;
	void BeginPass(Graphics& anInterface) final;
	void SubmitJobs(Graphics& anInterface) final;

	Handle<GPUPipeline> myPipeline;
	struct PerCameraModel
	{
		using UploadDesc = Model::UploadDescriptor<PosColorVertex>;
		Handle<GPUModel> myModel;
		Camera myCamera;
		UploadDesc myDesc;
		std::shared_ptr<UniformBlock> myBlock;
	};
	std::vector<PerCameraModel> myCameraModels;
};