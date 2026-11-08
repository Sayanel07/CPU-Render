#pragma once

#include <SDL3/SDL.h>
#include "../../ParticuleManager.h"
#include "../../render/renderer/camera.h"
#include "../../common/chrono.h"
#include "../../render/base/common.h"
#include "../../render/base/shader-descriptor.h"
#include "../../render/renderer/texture.h"
#include "../../render/renderer/Renderer.h"
#include "../../render/renderer/Object.h"
#include "../../render/renderer/Light.h"
//Students can change whatever they want in this file

union SDL_Event;
struct Mesh;

struct Engine 
{
	SDL_Window* window;

	void init(int width, int height);

	void handleEvent(const SDL_Event& event);

	void Update();

	void renderFrame(FinalFrameBuffer& frameBufferOut);

	void destroy();

	void UpdateLight();

	void CreateSkyBox(std::string path);

private:
	Renderer m_renderer;
	Camera cam = Camera(PERSPECTIVE);
	float camYaw = 0.0f;
	float camPitch = 0.0f;
	float camSensitivity = 0.002f ;
	float camSpeed = 10.0f;
	
	bool mouseActive = false;

	std::vector<Object> objects;
	std::vector<Object> frameBufferObjects;
	std::vector<WaterObject> waterObjects;
	std::vector<Light> lights;
	
	RendererShader m_heightMapShader;
	RendererShader m_shader;

	Chrono m_chrono;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;
    
    Texture m_texture;

	ParticuleManager m_particuleManager;
};