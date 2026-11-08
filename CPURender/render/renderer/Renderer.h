#pragma once
#include "../../common/transform.h"
#include "../base/graphics.h"
#include "../../common/frustum.h"
#include "Light.h"
#include "../base/mesh.h"

struct Material;
struct Texture;
struct FinalFrameBuffer;
class ParticuleManager;
class Camera;
class Chrono;
class WaterObject;

#define SHADOW_RES 1024 / 2

struct RendererShader
{
	void SetPS(GPU_HANDLE _hwnd) { m_shader.SetPS(_hwnd); }
	void SetVS(GPU_HANDLE _hwnd) { m_shader.SetVS(_hwnd); }
private:
	ShaderDescriptor m_shader;
	bool m_isInitialized = false;
	
	friend class Renderer;
};

class Renderer
{
private:
	struct SkyboxData 
	{
		Mesh cubeMesh;
		GPU_HANDLE cubeMap;
		ShaderDescriptor shaderSkybox;
	};
	
	struct DrawData {
		GPU_HANDLE m_objectBuffer;
		GPU_HANDLE m_materialBuffer;
		ShaderDescriptor m_defaultShader;
		RendererShader m_lastShader;
	};
	
	struct LightData {
		GPU_HANDLE m_lightBuffer;
		GPU_HANDLE m_shadowAtlas;
		GPU_HANDLE m_shadowPassBuffer;
		ShaderDescriptor m_shadowShader;
		glm::vec3 m_ambient = glm::vec3(0.1f);
	};
	
	struct WaterData
	{
		GPU_HANDLE m_waterBuffer;
		GPU_HANDLE m_reflectionBuffer;
		GPU_HANDLE m_reflectionDeth;
		GPU_HANDLE m_refractionColor;
		GPU_HANDLE m_renderTargetCopy;
	
		ShaderDescriptor m_waterShader;
	};
	
	GPU_HANDLE m_frameBuffer;
	GPU_HANDLE m_renderTarget;
	GPU_HANDLE m_depthBuffer;
	
	GPU_HANDLE m_viewPos;

	ShaderDescriptor m_particuleShader;
	
	DrawData m_drawData;
	LightData m_lightData;
	WaterData m_waterData;


	SkyboxData m_skybox;
	Color m_clearColor = {0.f,0.f,0.f,1.f};
	
	Camera* m_pCamera = nullptr;
	Frustum m_frustum;

	std::vector<glm::mat4> m_lightsViewProj;
	std::vector<Frustum> m_lightsFrustum;
	int m_currentIDLight = 0;
	std::vector<glm::mat4x4> m_mirrorViewProj;

	Chrono* chrono;

	void DebugShadowMap(FinalFrameBuffer& _frameBufferOut, int _screenW, int _screenH, int _lightIndex);

public:
	RendererShader CreateRendererShader() const;
	void SetShader(RendererShader& _shader);
	RendererShader GetDefault() const;
	
	void Init(int width, int height);
	void Destroy();
	void SetCamera(Camera* _pCamera);
	void SetSkybox(GPU_HANDLE _cubeMap);
	void SetClearColor(Color _clearColor);
	void SetAmbientLight(glm::vec3 _ambient);
	void BeginDraw();
	void Draw(Mesh* _pMesh, Transform& _transform, Material* _pMat);
	void DrawParticuleEmitter(ParticuleManager& _emitter);
	void BeginFrameBufferObjDraw();
	void FrameBufferDraw(Mesh* _pMesh, Transform& _Transform);
	void EndFrameBufferDraw();
	void PrepareShadows(const std::vector<Light>& _lights);
	void BeginShadowPass(int _lightIndex);
	void CastShadow(Mesh* _pMesh, Transform& _transform);
	void EndShadowPass();
	void PrepareReflection();
	void BeginReflectionPass(Mesh* _pMesh, Transform& _Transformn, bool& _isInFrustum, bool _reflectSkybox, int _idWater);
	void RenderReflection(Mesh* _pMesh, Transform& _Transform, int _idWater);
	void EndReflectionPass();
	void BeginWaterPass();
	void DrawWater(WaterObject* _waterObject, int _idWater);
	void EndDrawWater();
	void EndDraw(FinalFrameBuffer& _frameBufferOut);
	void UpdateLight(std::vector<Light>& _light);
	void updateViewPos(glm::vec3 _pos);
	void SetChrono(Chrono* _pChrono);
	void CopySceneForRefraction();
};

