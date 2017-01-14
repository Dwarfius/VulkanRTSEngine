#include "Game.h"
#include "Graphics.h"

Game* Game::inst = nullptr;

Game::Game()
{
	inst = this;
	gameObjects.reserve(2000);
}

Game::~Game()
{

}

void Game::Init()
{

}

void Game::Update(float deltaTime)
{
	// first we update the camera
	vec3 forward = camera.GetForward();
	vec3 right = camera.GetRight();
	vec3 up = camera.GetUp();
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_W) == GLFW_REPEAT)
		camera.Translate(forward * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_S) == GLFW_REPEAT)
		camera.Translate(-forward * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_D) == GLFW_REPEAT)
		camera.Translate(right * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_A) == GLFW_REPEAT)
		camera.Translate(-right * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_Q) == GLFW_REPEAT)
		camera.Translate(-up * deltaTime * speed);
	if (glfwGetKey(Graphics::GetWindow(), GLFW_KEY_E) == GLFW_REPEAT)
		camera.Translate(up * deltaTime * speed);

	oldMPos = curMPos;
	double x, y;
	glfwGetCursorPos(Graphics::GetWindow(), &x, &y);
	curMPos = vec2(x, y);
	vec2 deltaPos = curMPos - oldMPos;
	camera.Rotate((float)deltaPos.x * mouseSens, (float)-deltaPos.y * mouseSens);

	// [NYI]start updating the objects
	// ...
}

void Game::Render()
{
	// multithread this
	Graphics::Render();

	Graphics::Display();
}

void Game::CleanUp()
{

}