#include "Precomp.h"
#include "ImGUISystem.h"

#include "Game.h"
#include "Systems/ImGUI/ImGUIRendering.h"

#include <Graphics/Graphics.h>
#include <Graphics/Resources/GPUTexture.h>

#include <Core/Resources/AssetTracker.h>
#include <Core/Profiler.h>

ImGUISystem::ImGUISystem(Game& aGame)
	: myGame(aGame)
	, myGLFWImpl(aGame)
	, myRenderPass(nullptr)
{
}

void ImGUISystem::Init(GLFWwindow& aWindow)
{
	Profiler::ScopedMark profile("ImGUISystem::Init");

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	myGLFWImpl.Init(aWindow);

	Handle<Pipeline> imGUIPipeline = myGame.GetAssetTracker().GetOrCreate<Pipeline>("Engine/imgui.ppl");
	Handle<Texture> fontAtlas = new Texture();
	fontAtlas->SetMinFilter(ITexture::Filter::Linear);
	fontAtlas->SetMagFilter(ITexture::Filter::Linear);
	{
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		fontAtlas->SetWidth(width);
		fontAtlas->SetHeight(height);
		fontAtlas->SetFormat(ITexture::Format::UNorm_RGBA);
		fontAtlas->SetPixels(pixels, false);
	}

	myRenderPass = new ImGUIRenderPass(imGUIPipeline, fontAtlas, *myGame.GetGraphics());
	myGame.GetGraphics()->AddRenderPass(myRenderPass);
}

void ImGUISystem::Shutdown()
{
	myGLFWImpl.Shutdown();
	ImGui::DestroyContext();
}

void ImGUISystem::NewFrame(float aDeltaTime)
{
	myGLFWImpl.NewFrame(aDeltaTime);
	ImGui::NewFrame();
}

void ImGUISystem::Image(Handle<GPUTexture> aTexture, glm::vec2 aSize, glm::vec2 aUV0, glm::vec2 aUV1, glm::vec4 aTintColor, glm::vec4 aBorderColor)
{
	// we're not locking because all calls to imgui should be
	// happening under a lock
	ImGui::Image(aTexture.Get(),
		ImVec2(aSize.x, aSize.y),
		ImVec2(aUV0.x, aUV0.y),
		ImVec2(aUV1.x, aUV1.y),
		ImVec4(aTintColor.x, aTintColor.y, aTintColor.z, aTintColor.w),
		ImVec4(aBorderColor.x, aBorderColor.y, aBorderColor.z, aBorderColor.w)
	);
}