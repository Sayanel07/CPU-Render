#include "Renderer.h"
#include "camera.h"
#include "texture.h"
#include "../../common/Physics.h"
#include "../base/mesh.h"
#include "shaders.h"
#include "../../main.h" // TODO CHANGE
#include "../../particuleManager.h"
#include "../../common/GeoFactory.h"
#include "material.h"
#include "../../common/chrono.h"
#include "shaders/3D-lit.h"
#include "Object.h"

void Renderer::DebugShadowMap(FinalFrameBuffer& _frameBufferOut, int _screenW, int _screenH, int _lightIndex)
{
	std::vector<char> raw = Graphics::ReadTextureBuffer(m_lightData.m_shadowAtlas);
	const float* depth = reinterpret_cast<const float*>(raw.data());

	const int res = SHADOW_RES;      // taille d'une shadow map (une tuile)
	const int atlasW = SHADOW_RES;  // largeur totale de l'atlas en pixels
	const int atlasH = SHADOW_RES * MAX_LIGHTS;  // hauteur totale de l'atlas en pixels
	assert(raw.size() >= (size_t)atlasW * atlasH * sizeof(float));

	// Position de la tuile dans l'atlas
	const int tilesPerRow = atlasW / res;
	const int tilesPerCol = atlasH / res;
	assert(_lightIndex >= 0 && _lightIndex < tilesPerRow * tilesPerCol);

	const int tileX = (_lightIndex % tilesPerRow) * res;  // origine en pixels
	const int tileY = (_lightIndex / tilesPerRow) * res;

	auto isEmpty = [](float d) { return !std::isfinite(d) || d >= 1.0f; };

	// Lecture d'un pixel de la tuile (coordonnées locales 0..res-1)
	auto fetch = [&](int lx, int ly) -> float
		{
			return depth[(size_t)(tileY + ly) * atlasW + (tileX + lx)];
		};

	// 1. min/max uniquement sur la tuile
	float mn = std::numeric_limits<float>::max();
	float mx = std::numeric_limits<float>::lowest();
	for (int ly = 0; ly < res; ++ly)
	{
		for (int lx = 0; lx < res; ++lx)
		{
			const float d = fetch(lx, ly);
			if (isEmpty(d)) continue;
			mn = std::min(mn, d);
			mx = std::max(mx, d);
		}
	}
	const float range = (mx > mn) ? (mx - mn) : 1.f;

	// 2. Affichage de la tuile sur tout l'écran
	uint32_t* out = reinterpret_cast<uint32_t*>(_frameBufferOut.colorData);
	for (int y = 0; y < _screenH; ++y)
	{
		for (int x = 0; x < _screenW; ++x)
		{
			const int lx = x * res / _screenW;
			const int ly = y * res / _screenH;
			const float d = fetch(lx, ly);

			const uint32_t g = isEmpty(d) ? 0u : (uint32_t)(255.f * (1.f - (d - mn) / range));
			out[y * _screenW + x] = g * 0x01010101u;
		}
	}
}

RendererShader Renderer::CreateRendererShader() const
{
	ShaderDescriptor shader;
	shader.SetEntry(0, ShaderInput(Format::BUFFER1D, sizeof(PassCB)));
	shader.SetEntry(1, ShaderInput(Format::BUFFER1D, sizeof(LightPass)));
	shader.SetEntry(2, ShaderInput(Format::BUFFER2D, SHADOW_RES * SHADOW_RES * MAX_LIGHTS * sizeof(float))); //shadow atlas
	shader.SetEntry(3, ShaderInput(Format::BUFFER1D, sizeof(glm::vec3)));
	shader.SetEntry(4, ShaderInput(Format::BUFFER1D, sizeof(ObjectCB)));
	shader.SetEntry(5, ShaderInput(Format::BUFFER1D, sizeof(MaterialCB)));
	shader.SetEntry(6, ShaderInput(Format::BUFFER2D));
	shader.SetEntry(7, ShaderInput(Format::BUFFER2D));
	shader.SetEntry(8, ShaderInput(Format::BUFFER2D));
	shader.SetEntry(9, ShaderInput(Format::BUFFER2D));
	RendererShader rShader;
	rShader.m_shader  = shader;
	rShader.m_isInitialized = true;
	return rShader;
}

void Renderer::SetShader(RendererShader& _shader)
{
	if (_shader.m_isInitialized == false)
		return;
	
	if (_shader.m_shader == m_drawData.m_lastShader.m_shader)
		return;
    
    assert(_shader.m_shader.GetPixelShader().IsValid());
    assert(_shader.m_shader.GetVertexShader().IsValid());
	
	m_drawData.m_lastShader = _shader;
	Graphics::SetShader(m_drawData.m_lastShader.m_shader);
	Graphics::SetEntryBuffer(0, m_frameBuffer);
	Graphics::SetEntryBuffer(1, m_lightData.m_lightBuffer);
	Graphics::SetEntryBuffer(2, m_lightData.m_shadowAtlas);
	Graphics::SetEntryBuffer(3, m_viewPos);
}

RendererShader Renderer::GetDefault() const {
	RendererShader defaultShader;
	defaultShader.m_shader = m_drawData.m_defaultShader;
	return defaultShader;
}

void Renderer::Init(int width, int height)
{
	m_frameBuffer = Graphics::CreateBuffer(sizeof(PassCB));
	m_waterData.m_waterBuffer = Graphics::CreateBuffer(sizeof(WaterCB));
	m_renderTarget = Graphics::CreateTextureBuffer(width, height, sizeof(uint32));
	m_depthBuffer = Graphics::CreateTextureBuffer(width, height, sizeof(float));
	m_frameBuffer = Graphics::CreateBuffer(sizeof(PassCB));
	m_viewPos = Graphics::CreateBuffer(sizeof(glm::vec3));

	m_particuleShader.SetVS(Graphics::CompileShader(VertexParticule()));
	m_particuleShader.SetPS(Graphics::CompileShader(PixelParticule()));
	m_particuleShader.SetEntry(0, ShaderInput(Format::BUFFER2D));
	m_particuleShader.SetEntry(1, ShaderInput(Format::BUFFER1D, sizeof(PassCB)));
	m_particuleShader.SetEntry(2, ShaderInput(Format::BUFFER1D));
	
	// Default
	m_drawData.m_defaultShader.SetVS(Graphics::CompileShader(LitVertexShader()));
	m_drawData.m_defaultShader.SetPS(Graphics::CompileShader(LitPixelShader()));
	m_drawData.m_defaultShader.SetEntry(0, ShaderInput(Format::BUFFER1D, sizeof(PassCB)));
	m_drawData.m_defaultShader.SetEntry(1, ShaderInput(Format::BUFFER1D, sizeof(LightPass)));
	m_drawData.m_defaultShader.SetEntry(2, ShaderInput(Format::BUFFER2D, SHADOW_RES * SHADOW_RES * MAX_LIGHTS * sizeof(float))); //shadow atlas
	m_drawData.m_defaultShader.SetEntry(3, ShaderInput(Format::BUFFER1D, sizeof(glm::vec3)));
	m_drawData.m_defaultShader.SetEntry(4, ShaderInput(Format::BUFFER1D, sizeof(ObjectCB)));
	m_drawData.m_defaultShader.SetEntry(5, ShaderInput(Format::BUFFER1D, sizeof(MaterialCB)));
	m_drawData.m_defaultShader.SetEntry(6, ShaderInput(Format::BUFFER2D));
	m_drawData.m_defaultShader.SetEntry(7, ShaderInput(Format::BUFFER2D));
	m_drawData.m_defaultShader.SetEntry(8, ShaderInput(Format::BUFFER2D));
	m_drawData.m_defaultShader.SetEntry(9, ShaderInput(Format::BUFFER2D));
	m_drawData.m_objectBuffer = Graphics::CreateBuffer(sizeof(ObjectCB));
	m_drawData.m_materialBuffer = Graphics::CreateBuffer(sizeof(MaterialCB));

	// Light
	m_lightData.m_shadowPassBuffer = Graphics::CreateBuffer(sizeof(PassCB));
	m_lightData.m_lightBuffer = Graphics::CreateBuffer(sizeof(LightPass));
	m_lightData.m_shadowAtlas = Graphics::CreateTextureBuffer(SHADOW_RES, SHADOW_RES * MAX_LIGHTS, sizeof(float));
	
	m_lightData.m_shadowShader.SetVS(Graphics::CompileShader(VertexDefault()));
	m_lightData.m_shadowShader.SetEntry(0, ShaderInput(Format::BUFFER2D));
	m_lightData.m_shadowShader.SetEntry(1, ShaderInput(Format::BUFFER1D, sizeof(PassCB)));
	m_lightData.m_shadowShader.SetEntry(2, ShaderInput(Format::BUFFER1D, sizeof(ObjectCB)));
	
	// Water
	m_waterData.m_refractionColor = Graphics::CreateTextureBuffer(width, height, sizeof(uint32));
	m_waterData.m_reflectionBuffer = Graphics::CreateTextureBuffer(width / 2, height / 2 * 2, sizeof(uint32));
	m_waterData.m_reflectionDeth = Graphics::CreateTextureBuffer(width / 2, height / 2 * 2, sizeof(uint32));
	m_waterData.m_renderTargetCopy = Graphics::CreateTextureBuffer(width, height, sizeof(uint32));
	
	m_waterData.m_waterShader.SetVS(Graphics::CompileShader(VertexWater()));
	m_waterData.m_waterShader.SetPS(Graphics::CompileShader(PixelShaderWater()));
	m_waterData.m_waterShader.SetEntry(0, ShaderInput(Format::BUFFER2D));
	m_waterData.m_waterShader.SetEntry(1, ShaderInput(Format::BUFFER1D, sizeof(PassCB)));
	m_waterData.m_waterShader.SetEntry(2, ShaderInput(Format::BUFFER1D, sizeof(WaterCB)));
	m_waterData.m_waterShader.SetEntry(3, ShaderInput(Format::BUFFER2D, sizeof(m_waterData.m_refractionColor)));
	m_waterData.m_waterShader.SetEntry(4, ShaderInput(Format::BUFFER2D, sizeof(m_waterData.m_reflectionDeth)));
	m_waterData.m_waterShader.SetEntry(5, ShaderInput(Format::BUFFER2D, sizeof(m_waterData.m_reflectionBuffer)));
	m_waterData.m_waterShader.SetEntry(6, ShaderInput(Format::BUFFER1D, sizeof(LightPass)));
	
	// Skybox 
	m_skybox.shaderSkybox.SetVS(Graphics::CompileShader(VertexShaderSkybox()));
	m_skybox.shaderSkybox.SetPS(Graphics::CompileShader(PixelShaderSkybox()));
	m_skybox.shaderSkybox.SetEntry(0, ShaderInput(Format::CUBEMAP));
	m_skybox.shaderSkybox.SetEntry(1, ShaderInput(Format::BUFFER1D, sizeof(PassCB)));
	m_skybox.cubeMesh = Mesh(GeoFactory::CreateCube(1, false));
}

void Renderer::Destroy()
{
	Graphics::FreeResource(m_drawData.m_objectBuffer);
	m_drawData.m_defaultShader.Free();
	
	Graphics::FreeResource(m_skybox.cubeMap);
	m_skybox.cubeMesh.Free();
	m_skybox.shaderSkybox.Free();
	
	Graphics::FreeResource(m_lightData.m_lightBuffer);
	Graphics::FreeResource(m_lightData.m_shadowAtlas);
	m_lightData.m_shadowShader.Free();
	
	Graphics::FreeResource(m_frameBuffer);
	Graphics::FreeResource(m_renderTarget);
	Graphics::FreeResource(m_depthBuffer);

	m_particuleShader.Free();
}

void Renderer::SetCamera(Camera* _pCamera)
{
	m_pCamera = _pCamera;
}

void Renderer::SetSkybox(GPU_HANDLE _cubeMap) {
	assert(_cubeMap.GetFormat() & Format::CUBEMAP);
	m_skybox.cubeMap = _cubeMap;
}

void Renderer::SetClearColor(Color _clearColor) {
	m_clearColor = _clearColor;
}

void Renderer::SetAmbientLight(glm::vec3 _ambient)
{
	m_lightData.m_ambient = _ambient;
}

void Renderer::BeginDraw()
{
	Graphics::SetRenderTarget(m_renderTarget);
	Graphics::SetDepthBuffer(m_depthBuffer);
	Graphics::ClearDrawTarget(m_clearColor);

	PassCB cb;
	cb.proj = m_pCamera->GetProjectionMatrix();
	cb.view = m_pCamera->GetViewMatrix();
	cb.time = chrono->CurrentTime();

	Graphics::SetBuffer(m_frameBuffer, &cb, sizeof(cb));

	if (m_skybox.cubeMap.IsValid()) {
		Graphics::SetDepthBufferUse(false, false);
		Graphics::SetShader(m_skybox.shaderSkybox);
		Graphics::SetEntryBuffer(0, m_skybox.cubeMap);
		Graphics::SetEntryBuffer(1, m_frameBuffer);
	
		Graphics::Draw(m_skybox.cubeMesh);
	}
	
	Graphics::SetDepthBufferUse(true, true);
	if (m_drawData.m_lastShader.m_isInitialized == false)
		Graphics::SetShader(m_drawData.m_defaultShader);
	else
		Graphics::SetShader(m_drawData.m_lastShader.m_shader);
	Graphics::SetEntryBuffer(0, m_frameBuffer);
	Graphics::SetEntryBuffer(1, m_lightData.m_lightBuffer);
	Graphics::SetEntryBuffer(2, m_lightData.m_shadowAtlas);
	Graphics::SetEntryBuffer(3, m_viewPos);

	m_frustum = m_pCamera->GetFrustum();
}

void Renderer::Draw(Mesh* _pMesh, Transform& _transform, Material* _pMat)
{
    if (Physics::isOnFrustum(_pMesh->GetBoundingBox(), m_frustum, _transform) == false)
        return;
	
    ObjectCB objectCb;
    objectCb.model = _transform.getWorldMatrix();
    AABB transformedAABB = _pMesh->GetBoundingBox();
    transformedAABB.TransformAABB(_transform.getWorldMatrix());
    objectCb.min = transformedAABB.min;
    objectCb.max = transformedAABB.max;
	
	SetShader(_pMat->shader);
	
	MaterialCB materialCb;
	materialCb.color = _pMat->color;

    if (_pMat != nullptr) {
        if (_pMat->texture != nullptr) {
	        Graphics::SetEntryBuffer(6, _pMat->texture->GetTextureBuffer());
        	materialCb.useAlbedo = true;
        }
        if (_pMat->normalMap != nullptr) {
	        Graphics::SetEntryBuffer(7, _pMat->normalMap->GetTextureBuffer());
        	materialCb.useNormal = true;
        }        
    	if (_pMat->metallicMap != nullptr) {
	        Graphics::SetEntryBuffer(8, _pMat->metallicMap->GetTextureBuffer());
        	materialCb.useMetallic = true;
        }        
    	if (_pMat->roughnessMap != nullptr) {
	        Graphics::SetEntryBuffer(9, _pMat->roughnessMap->GetTextureBuffer());
        	materialCb.useRoughness = true;
        }
        if (_pMat->_isTransparent)
    	    Graphics::SetBlendDescriptor(_pMat->blendDescriptor);
    }
	
    Graphics::SetBuffer(m_drawData.m_objectBuffer, &objectCb, sizeof(ObjectCB));
    Graphics::SetEntryBuffer(4, m_drawData.m_objectBuffer);
    Graphics::SetBuffer(m_drawData.m_materialBuffer, &materialCb, sizeof(MaterialCB));
	Graphics::SetEntryBuffer(5, m_drawData.m_materialBuffer);

    Graphics::Draw(*_pMesh);
    Graphics::SetBlendDescriptorUse(false);
}

void Renderer::BeginFrameBufferObjDraw()
{
	Graphics::SetRenderTarget(m_renderTarget);
	Graphics::SetDepthBuffer(m_depthBuffer);

	PassCB cb;
	cb.proj = m_pCamera->GetProjectionMatrix();
	cb.view = m_pCamera->GetViewMatrix();
	cb.time = chrono->CurrentTime();

	Graphics::SetBuffer(m_frameBuffer, &cb, sizeof(cb));

	Graphics::CopyTextureBuffer(m_waterData.m_renderTargetCopy, m_renderTarget);

	Graphics::SetDepthBufferUse(true, true);
	Graphics::SetShader(m_drawData.m_defaultShader);
	Graphics::SetEntryBuffer(0, m_frameBuffer);
	Graphics::SetEntryBuffer(1, m_lightData.m_lightBuffer);
	Graphics::SetEntryBuffer(2, m_lightData.m_shadowAtlas);
	Graphics::SetEntryBuffer(3, m_viewPos);

	m_frustum = m_pCamera->GetFrustum();
}


void Renderer::FrameBufferDraw(Mesh* _pMesh, Transform& _Transform)
{
	if (Physics::isOnFrustum(_pMesh->GetBoundingBox(), m_frustum, _Transform) == false)
		return;

	//Graphics::SetShader(m_shader);
	Graphics::SetEntryBuffer(1, m_frameBuffer);
	
	ObjectCB objectCb;
	objectCb.model = _Transform.getWorldMatrix();

	MaterialCB materialCb;

	Graphics::SetEntryBuffer(6, m_waterData.m_renderTargetCopy);
	materialCb.useAlbedo = true;
	materialCb.useLight = false;
	
	Graphics::SetBuffer(m_drawData.m_materialBuffer, &materialCb, sizeof(MaterialCB));
	Graphics::SetBuffer(m_drawData.m_objectBuffer, &objectCb, sizeof(objectCb));
	Graphics::SetEntryBuffer(4, m_drawData.m_objectBuffer);
	Graphics::SetEntryBuffer(5, m_drawData.m_materialBuffer);

	Graphics::Draw(*_pMesh);

	Graphics::SetShader(m_drawData.m_defaultShader);
}

void Renderer::DrawParticuleEmitter(ParticuleManager& _emitter)
{
	Graphics::SetShader(m_particuleShader);

	Graphics::SetEntryBuffer(1, m_frameBuffer);

	int i = 0;
	for (const auto& e : _emitter.GetEmitters())
	{
		Graphics::SetEntryBuffer(2, _emitter.GetHandle(i));
		if (e.mat.texture != nullptr)
			Graphics::SetEntryBuffer(0, e.mat.texture.get()->GetTextureBuffer());

		Graphics::Draw(_emitter.GetMesh(), e.particules.size());
		i++;
	}	
}

void Renderer::EndFrameBufferDraw()
{

}

void Renderer::PrepareShadows(const std::vector<Light>& _lights)
{
    m_lightsViewProj.clear();
    m_lightsFrustum.clear();

    for (const Light& light : _lights)
    {
		if (light.type == DIRECTIONAL)
		{
			glm::mat4 vp = light.GetViewProj(glm::vec3(-100), glm::vec3(100));
			m_lightsViewProj.push_back(vp);
			m_lightsFrustum.push_back(light.GetFrustum(vp));
		}
		else //spot
		{
			glm::mat4 vp = light.GetSpotViewProj(m_pCamera->GetNear());
			m_lightsViewProj.push_back(vp);
			m_lightsFrustum.push_back(light.GetFrustum(vp));
		}
    }

    Graphics::SetDepthBuffer(m_lightData.m_shadowAtlas);
    Graphics::ClearDepth(m_lightData.m_shadowAtlas); //cost a lot
}

void Renderer::BeginShadowPass(int _lightIndex)
{
    m_currentIDLight = _lightIndex;

    PassCB cb;
    cb.proj = m_lightsViewProj[_lightIndex];
    cb.view = glm::mat4(1.f);
    Graphics::SetBuffer(m_lightData.m_shadowPassBuffer, &cb, sizeof(cb));

    Graphics::SetRenderTarget(m_renderTarget, false);
	
    Graphics::SetViewPort(0, _lightIndex * SHADOW_RES, SHADOW_RES, SHADOW_RES);
	
    Graphics::SetShader(m_lightData.m_shadowShader);
    Graphics::SetEntryBuffer(1, m_lightData.m_shadowPassBuffer);
    Graphics::SetBackFaceCulling(Graphics::GetBackFaceCullingType(), false);
}

void Renderer::CastShadow(Mesh* _pMesh, Transform& _transform)
{
    if (Physics::isOnFrustum(_pMesh->GetBoundingBox(), m_lightsFrustum[m_currentIDLight], _transform) == false)
        return;

    ObjectCB objectCB;
    objectCB.model = _transform.getWorldMatrix();
    Graphics::SetBuffer(m_drawData.m_objectBuffer, &objectCB, sizeof(objectCB));
    Graphics::SetEntryBuffer(2, m_drawData.m_objectBuffer);

    Graphics::Draw(*_pMesh);
}

void Renderer::EndShadowPass()
{
    //Graphics::CopyTextureBuffer(m_shadowAtlas, m_currentIDLight * SHADOW_RES, m_shadowMaps[m_currentIDLight]);

    Graphics::ResetViewPort();

    Graphics::SetRenderTarget(m_renderTarget);
    Graphics::SetDepthBuffer(m_depthBuffer);

	Graphics::SetBackFaceCulling(Graphics::GetBackFaceCullingType());
}

void Renderer::PrepareReflection()
{
	Graphics::SetRenderTarget(m_waterData.m_reflectionBuffer);
	Graphics::SetDepthBuffer(m_waterData.m_reflectionDeth);
	Graphics::ClearRenderTarget(m_clearColor);
	Graphics::ClearDepth(m_waterData.m_reflectionDeth);

	m_mirrorViewProj.assign(2, glm::mat4(1.0f));
}

void Renderer::BeginReflectionPass(Mesh* _pMesh, Transform& _Transform, bool& _isInFrustum, bool _reflectSkybox, int _idWater)
{
	m_frustum = m_pCamera->GetFrustum();

	if (Physics::isOnFrustum(_pMesh->GetBoundingBox(), m_frustum, _Transform))
	{
		_isInFrustum = true;
		RenderReflection(_pMesh, _Transform, _idWater);

		if (m_skybox.cubeMap.IsValid() && _reflectSkybox)
		{
			Graphics::SetDepthBufferUse(false, false);
			Graphics::SetShader(m_skybox.shaderSkybox);
			Graphics::SetEntryBuffer(0, m_skybox.cubeMap);
			Graphics::SetEntryBuffer(1, m_frameBuffer);
			Graphics::DisableUserClipPlane();

			Graphics::Draw(m_skybox.cubeMesh);

			Graphics::EnableUserClipPlane();
			Graphics::SetDepthBuffer(m_waterData.m_reflectionDeth);
			if (m_drawData.m_lastShader.m_isInitialized == false)
				Graphics::SetShader(m_drawData.m_defaultShader);
			else
				Graphics::SetShader(m_drawData.m_lastShader.m_shader);
			Graphics::SetEntryBuffer(0, m_frameBuffer);
			Graphics::SetEntryBuffer(1, m_lightData.m_lightBuffer);
			Graphics::SetEntryBuffer(2, m_lightData.m_shadowAtlas);
			Graphics::SetEntryBuffer(3, m_viewPos);
			Graphics::SetDepthBufferUse(true, true);
		}
	}

}

void Renderer::RenderReflection(Mesh* _pMesh, Transform& _Transform, int _idWater)
{
	float h = _Transform.getPosition().y;

	Camera mirror = *m_pCamera;
	glm::vec3 pos = mirror.transform.getPosition();
	mirror.transform.setPosition({ pos.x, 2.0f * h - pos.y, pos.z });
	glm::vec3 fwd = m_pCamera->transform.getForward();
	glm::vec3 up = m_pCamera->transform.getUp();
	fwd.y = -fwd.y;
	up.y = -up.y;
	mirror.transform.lookAt(mirror.transform.getPosition() + fwd, up);
	glm::mat4 view = mirror.GetViewMatrix();
	m_mirrorViewProj[_idWater] = mirror.GetProjectionMatrix() * view;


	m_frustum = mirror.GetFrustum();


	Graphics::SetViewPort(0, _idWater * HEIGHT / 2, WIDTH / 2, HEIGHT / 2);

	if (m_drawData.m_lastShader.m_isInitialized == false)
		Graphics::SetShader(m_drawData.m_defaultShader);
	else
		Graphics::SetShader(m_drawData.m_lastShader.m_shader);

	PassCB cb;
	cb.proj = mirror.GetProjectionMatrix();
	cb.view = mirror.GetViewMatrix();
	cb.time = chrono->CurrentTime();

	Graphics::SetBuffer(m_frameBuffer, &cb, sizeof(cb));

	Graphics::SetEntryBuffer(0, m_frameBuffer);
	Graphics::SetEntryBuffer(1, m_lightData.m_lightBuffer);
	Graphics::SetEntryBuffer(2, m_lightData.m_shadowAtlas);
	Graphics::SetEntryBuffer(3, m_viewPos);


	Graphics::SetUserClipPlane(glm::vec4(0, 1, 0, -(h + 0.05f)), m_mirrorViewProj[_idWater]);
}

void Renderer::EndReflectionPass()
{
	Graphics::ResetViewPort();

	Graphics::DisableUserClipPlane();

	m_frustum = m_pCamera->GetFrustum();
}

void Renderer::BeginWaterPass()
{
	Graphics::SetRenderTarget(m_renderTarget);
	Graphics::SetDepthBuffer(m_depthBuffer);

	PassCB cb;
	cb.proj = m_pCamera->GetProjectionMatrix();
	cb.view = m_pCamera->GetViewMatrix();
	cb.time = chrono->CurrentTime();

	Graphics::SetBuffer(m_frameBuffer, &cb, sizeof(cb));

	Graphics::SetShader(m_waterData.m_waterShader);
	Graphics::SetEntryBuffer(1, m_frameBuffer);
	Graphics::SetEntryBuffer(3, m_waterData.m_refractionColor);
	Graphics::SetEntryBuffer(4, m_depthBuffer);
	Graphics::SetDepthBufferUse(true, false);

	m_frustum = m_pCamera->GetFrustum();
}

void Renderer::DrawWater(WaterObject* _waterObject, int _idWater)
{
	WaterCB wCB;
	wCB.model = _waterObject->transform.getWorldMatrix();
	wCB.mirrorViewProj = m_mirrorViewProj[_idWater];
	wCB.cameraPos = m_pCamera->transform.getPosition();
	wCB.cameraForward = m_pCamera->transform.getForward();
	wCB.nearPlane = m_pCamera->GetNear();
	wCB.farPlane = m_pCamera->GetFar();
	wCB.deepColor = _waterObject->material->color;
	wCB.absorption = 0.4f;
	wCB.time = chrono->CurrentTime();
	wCB.waveAmp = _waterObject->waveAmp;
	wCB.ondulationReflect = _waterObject->ondulationReflect;
	wCB.refractionStrength = _waterObject->refractionStrength;
	wCB.usLightEffect = true;
	wCB.idWater = _idWater;

	Graphics::SetBuffer(m_waterData.m_waterBuffer, &wCB, sizeof(wCB));
	Graphics::SetEntryBuffer(2, m_waterData.m_waterBuffer);
	Graphics::SetEntryBuffer(3, m_waterData.m_refractionColor);
	Graphics::SetEntryBuffer(5, m_waterData.m_reflectionBuffer);
	Graphics::SetEntryBuffer(6,  m_lightData.m_lightBuffer);

	//Graphics::SetViewPort(_idWater * WIDTH / 2, _idWater * HEIGHT / 2, WIDTH, HEIGHT);

	Graphics::Draw(*_waterObject->mesh);
}

void Renderer::EndDrawWater()
{
	Graphics::ResetViewPort();

	m_mirrorViewProj.clear();
}

void Renderer::EndDraw(FinalFrameBuffer& _frameBufferOut)
{
	std::vector<char> renderTarget = Graphics::ReadTextureBuffer(m_renderTarget);

	memcpy(_frameBufferOut.colorData, renderTarget.data(), renderTarget.size());

	//DebugShadowMap(_frameBufferOut, WIDTH, HEIGHT, 0);
}

void Renderer::UpdateLight(std::vector<Light>& _light)
{
    LightPass pass = {};

    uint32_t count = (uint32_t)std::min<size_t>(_light.size(), MAX_LIGHTS);
    for (uint32_t i = 0; i < count; ++i)
        pass.lights[i] = _light[i];
    pass.lightCount = count;
	pass.ambient = m_lightData.m_ambient;

    size_t nbMat = std::min<size_t>(m_lightsViewProj.size(), MAX_LIGHTS);
    for (size_t i = 0; i < nbMat; ++i)
        pass.lightViewProj[i] = m_lightsViewProj[i];

    Graphics::SetBuffer(m_lightData.m_lightBuffer, &pass, sizeof(LightPass));
}

void Renderer::updateViewPos(glm::vec3 _pos)
{
    Graphics::SetBuffer(m_viewPos, &_pos, sizeof(glm::vec3));
}

void Renderer::SetChrono(Chrono* _pChrono)
{
	chrono = _pChrono;
}

void Renderer::CopySceneForRefraction()
{
	Graphics::CopyTextureBuffer(m_waterData.m_refractionColor, m_renderTarget);
}
