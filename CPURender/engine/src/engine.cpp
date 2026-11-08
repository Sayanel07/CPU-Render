#include "engine.h"
//#include <stdlib.h>
//#include <SDL3/SDL_events.h>
////Students can change whever they want in this file
//
#define STB_IMAGE_IMPLEMENTATION
#include <memory>
#include <glm/gtc/quaternion.hpp>

#include "../../common/stb_image.h"
#include "../../render/base/Mesh.h"
#include "../../render/base/graphics.h"
#include "../../render/renderer/Renderer.h"
#include "../../common/GeoFactory.h"
#include "../../common/utils.h"
#include "../../common/obj-importer.h"
#include "../../common/HeightMap.h"
#include "../../common/Marching cubes/MarchingCubes.h"
#include "../../render/renderer/shaders/3D-lit.h"

//DEBUG//
Profiler prof;
//DEBUG//

void Engine::init(int width, int height)
{
    printf("Init engine \n");
    m_chrono.Start();
    m_renderer.SetCamera(&cam);
    
    std::shared_ptr<Texture> atlas = std::make_shared<Texture>();
    atlas->LoadFromFile("Assets/atlas.png");
    
    std::shared_ptr<Material> mat = std::make_shared<Material>();
    mat->texture = atlas;
    
    m_heightMapShader = m_renderer.CreateRendererShader();
    m_heightMapShader.SetVS(Graphics::CompileShader(LitVertexShader()));
    m_heightMapShader.SetPS(Graphics::CompileShader(HeightMapPixelShader()));
    mat->shader = m_heightMapShader;

    m_shader = m_renderer.CreateRendererShader();
    m_shader.SetVS(Graphics::CompileShader(LitVertexShader()));
    m_shader.SetPS(Graphics::CompileShader(LitPixelShader()));
    
    std::shared_ptr<Material> blueWater = std::make_shared<Material>();
    blueWater->color = { 0.0f, 0.2f, 0.35f, 1.0f };
    blueWater->SetWaterEffect(width, height);
    blueWater->shader = m_shader;

    std::shared_ptr<Material> matGrass = std::make_shared<Material>();
    Texture tGrass;
    tGrass.LoadFromFile("Assets/grass.png");
    matGrass->texture = std::make_shared<Texture>(tGrass);
    matGrass->shader = m_shader;

    //heightmap//
    HeightMap map;
    map.Generate(64,64,-10,10,4, true);
    std::shared_ptr<Mesh> mPlane = std::make_shared<Mesh>(*map.GetGeometry());

    Object objPlane;
    objPlane.mesh = mPlane;
    objPlane.material = mat;
    objPlane.transform.setPosition(glm::vec3{ 0, -2, 10 });
    objects.push_back(objPlane);

    //water//
    Geometry planeGeo = GeoFactory::CreatePlane(64, 64, 32);

    std::shared_ptr<Mesh> planeMesh = std::make_shared<Mesh>(planeGeo);

    WaterObject waterObj;
    waterObj.mesh = planeMesh;
    waterObj.material = blueWater;
    waterObj.transform.setPosition({ 32, -3, 26+16 });
    waterObjects.push_back(waterObj);

    //particules//
    vec3 pos = { 16, -2, -40 };
    //vec3 pos = { 16, -2, 26 };

    Geometry waterGeo = GeoFactory::CreatePlane(5, 5, 5);

    std::shared_ptr<Mesh> waterMesh = std::make_shared<Mesh>(waterGeo);

    WaterObject waterObj2;
    waterObj2.mesh = waterMesh;
    waterObj2.material = blueWater;
    waterObj2.transform.setPosition({ pos.x + 7, pos.y + 2, pos.z - 10 });
    waterObj2.waveAmp = 0;
    //waterObj2.reflectSkybox = false;
    waterObjects.push_back(waterObj2); //need to manage multiple water obj

    std::shared_ptr<Material> leavesMat = std::make_shared<Material>();
    Texture leavesTex;
    leavesTex.LoadFromFile("Assets/Particules/leaves.png");
    leavesMat->texture = std::make_shared<Texture>(leavesTex);
    leavesMat->shader = m_shader;

    std::shared_ptr<Material> volcMat = std::make_shared<Material>();
    Texture volcTex;
    volcTex.LoadFromFile("Assets/stone.png");
    volcMat->texture = std::make_shared<Texture>(volcTex);
    volcMat->shader = m_shader;

    std::shared_ptr<Material> metalMat = std::make_shared<Material>();
    Texture metalTex;
    metalTex.LoadFromFile("Assets/stone.png");
    metalMat->texture = std::make_shared<Texture>(metalTex);
    metalMat->metallicMap = std::make_shared<Texture>(metalTex);
    metalMat->shader = m_shader;
    
    Geometry grassGeo = GeoFactory::CreatePlane(32, 32, 32);
    std::shared_ptr<Mesh> grassMesh = std::make_shared<Mesh>(grassGeo);
    Geometry cubeG = GeoFactory::CreateCube();
    std::shared_ptr<Mesh> cubeM = std::make_shared<Mesh>(cubeG);
    Geometry coneGeo = GeoFactory::CreateCone();
    std::shared_ptr<Mesh> coneMesh = std::make_shared<Mesh>(coneGeo);
    Geometry cyliGeo = GeoFactory::CreateCylinder();
    std::shared_ptr<Mesh> cyliMesh = std::make_shared<Mesh>(cyliGeo);

    Object grassObj;
    grassObj.mesh = grassMesh;
    grassObj.material = matGrass;
    grassObj.transform.setPosition(pos);
    objects.push_back(grassObj);

    Object cubeO;
    cubeO.mesh = cubeM;
    cubeO.material = leavesMat;
    cubeO.transform.setPosition({ pos.x + 8, pos.y + 4, pos.z - 10 });
    objects.push_back(cubeO);

    Object coneObj;
    coneObj.mesh = coneMesh;
    coneObj.material = volcMat;
    coneObj.transform.setPosition({ pos.x - 4, pos.y + 1, pos.z - 0 });
    objects.push_back(coneObj);

    Object cyliObj;
    cyliObj.mesh = cyliMesh;
    cyliObj.material = metalMat;
    cyliObj.transform.setPosition({ pos.x - 0, pos.y + 1, pos.z + 10 });
    objects.push_back(cyliObj);

    m_particuleManager.Init();
    m_particuleManager.SetCamera(&cam);
    m_particuleManager.CreateParticulesPoint({ 25, -1, 28}, PartType::Smoke);
    m_particuleManager.CreateParticulesPoint({ 16, 2, 23 }, PartType::Leaves);
    m_particuleManager.CreateSnow();

    //TV SCREEN//
    Geometry tvGeo = ObjImporter::loadFromFile("Assets/tv.obj");
    std::shared_ptr<Mesh> tvMesh = std::make_shared<Mesh>(tvGeo);
    std::shared_ptr<Material> tvMat = std::make_shared<Material>();
    Texture tvTexture;
    tvTexture.LoadFromFile("Assets/diffuse.png");
    
    
    tvMat->texture = std::make_shared<Texture>(tvTexture);
    tvMat->shader = m_shader;
    Object tvObj;
    tvObj.mesh = tvMesh;
    tvObj.transform.setPosition({ 15, -1, 23 });
    tvObj.transform.setScale(glm::vec3(0.2f ));
    tvObj.transform.rotate({0, -45, 0});
    tvObj.material = tvMat;
    objects.push_back(tvObj);

    Geometry cubeGeo = GeoFactory::CreatePlane();
    std::shared_ptr<Mesh> cubeMesh = std::make_shared<Mesh>(cubeGeo);
    Object cubeObj;
    cubeObj.mesh = cubeMesh;
    cubeObj.transform.setPosition({ 14.8, -1, 23.1 });
    cubeObj.transform.setScale(glm::vec3(0.75f));
    cubeObj.transform.rotate({ glm::radians(90.0f), glm::radians(-58.0f), 0});
    frameBufferObjects.push_back(cubeObj);

    cam.SetViewPort(width, height);
    cam.transform.rotate({0, glm::radians(-110.0f), 0});
    camYaw = glm::radians(-110.0f);
    cam.transform.setPosition({13, -0.8f, 23});

    Light lightr;
    lightr.color = { 1.0, 0.45, 0.1, 1 };
    lightr.lightDir = { -1.0f, -0.5f, -0.0f };
    lights.push_back(lightr);

    Light spotLight;
    spotLight.color = { 1.0, 0.7, 0.3, 1.0f };
    spotLight.lightDir = { 0.0f, -1.0f, 0.0f };
    spotLight.type = SPOT;
    spotLight.pos = { 15, 4, 23 };
    spotLight.range = 15;
    spotLight.strength = 50;
    spotLight.SetAngleD(20, 50);
    lights.push_back(spotLight);

    /*Light lightb;
    lightb.color = { 0.0f, 0, 1.0f, 1 };
    lightb.lightDir = { 0.0f, -0.5f, 1.0f };
    lights.push_back(lightb);
    
    Light lightg;
    lightg.color = { 0.0f, 1, 0.0f, 1 };
    lightg.lightDir = { 0.0f, -1.0f, 0.0f };
    lights.push_back(lightg);*/

    m_renderer.Init(width, height);
    m_renderer.SetChrono(&m_chrono);

    CreateSkyBox("Skybox");
}

void Engine::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        if (event.key.key == SDLK_Z || event.key.key == SDLK_W)
        {
            cam.transform.setPosition(cam.transform.getPosition() + cam.transform.getForward() * camSpeed * deltaTime);
        }
        if (event.key.key == SDLK_S)
        {
            cam.transform.setPosition(cam.transform.getPosition() - cam.transform.getForward() * camSpeed * deltaTime);
        }
        if (event.key.key == SDLK_Q || event.key.key == SDLK_A)
        {
            cam.transform.setPosition(cam.transform.getPosition() - cam.transform.getRight() * camSpeed * deltaTime);
        }
        if (event.key.key == SDLK_D)
        {
            cam.transform.setPosition(cam.transform.getPosition() + cam.transform.getRight() * camSpeed * deltaTime);
        }
        if (event.key.key == SDLK_SPACE)
        {
            cam.transform.setPosition({ cam.transform.getPosition().x, cam.transform.getPosition().y + 1.0f * camSpeed * deltaTime, cam.transform.getPosition().z });
        }
        if (event.key.key == SDLK_LCTRL)
        {
            cam.transform.setPosition({ cam.transform.getPosition().x, cam.transform.getPosition().y - 1.0f * camSpeed * deltaTime, cam.transform.getPosition().z });
        }
        if (event.key.key == SDLK_R)
        {
            cam.transform.setPosition({ 0, 0, 0 });
        }

        if (event.key.key == SDLK_ESCAPE)
        {
            SDL_SetWindowRelativeMouseMode(window, mouseActive);

            mouseActive = !mouseActive;
        }

        if (event.key.key == SDLK_B)
        {
             if (Graphics::GetBackFaceCullingType() == CLOCK_WISE)
                 Graphics::SetBackFaceCulling(COUNTER_CLOCK_WISE);
             else
                 Graphics::SetBackFaceCulling(CLOCK_WISE);
        }
    }
    else if (event.type == SDL_EVENT_MOUSE_MOTION)
    {
        camYaw -= event.motion.xrel * camSensitivity;
        camPitch -= event.motion.yrel * camSensitivity;

        const float limit = glm::radians(89.0f);
        camPitch = glm::clamp(camPitch, -limit, limit);

        glm::quat q = glm::angleAxis(camYaw, glm::vec3(0, 1, 0)) * glm::angleAxis(camPitch, glm::vec3(1, 0, 0));
        cam.transform.setRotation(q);
    }
}

void Engine::Update()
{
    float currentTime = m_chrono.CurrentTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    m_particuleManager.Update(deltaTime);
}

void Engine::renderFrame(FinalFrameBuffer& frameBufferOut)
{
    Update();
    { ScopedTimer t(prof, ST_UPDATE_LIGHT); m_renderer.UpdateLight(lights); }

    { ScopedTimer t(prof, ST_PREPARE); m_renderer.PrepareShadows(lights); }    


    for (int i = 0; i < (int)lights.size() && i < MAX_LIGHTS; ++i)
    {
        { ScopedTimer t(prof, ST_SHADOW_BEGIN); m_renderer.BeginShadowPass(i); }

        {   // UN seul chrono autour de la boucle, pas un par objet
            ScopedTimer t(prof, ST_SHADOW_CAST);
            for (Object& obj : objects)
                if (obj.castShadow)
                    m_renderer.CastShadow(obj.mesh.get(), obj.transform);
        }
    }

    { ScopedTimer t(prof, ST_SHADOW_END); m_renderer.EndShadowPass(); }
    m_renderer.updateViewPos(cam.transform.getPosition());

    if (!waterObjects.empty())
    {
        m_renderer.PrepareReflection();

        int i = 0;
        for (WaterObject& waterObj : waterObjects)
        {
            bool isInFrustum = false;
            { ScopedTimer t(prof, ST_REFLECTION_BEGIN); m_renderer.BeginReflectionPass(waterObj.mesh.get(), waterObj.transform, isInFrustum, waterObj.reflectSkybox, i); }

            if (isInFrustum)
            {
                ScopedTimer t(prof, ST_REFLECTION_DRAW);
                for (Object& obj : objects)
                    m_renderer.Draw(obj.mesh.get(), obj.transform, obj.material.get());
            }

            i++;
        }

        { ScopedTimer t(prof, ST_REFLECTION_END); m_renderer.EndReflectionPass(); }
    }
    
    { ScopedTimer t(prof, ST_MAIN_BEGIN);   m_renderer.BeginDraw(); }
    {
        ScopedTimer t(prof, ST_MAIN_DRAW);
        for (Object& obj : objects) 
            m_renderer.Draw(obj.mesh.get(), obj.transform, obj.material.get());

        {
            ScopedTimer t(prof, ST_PARTICUL);
            m_renderer.DrawParticuleEmitter(m_particuleManager);
        }

        if (!waterObjects.empty())
        {
            ScopedTimer t(prof, ST_COPY_SCENE);
            m_renderer.CopySceneForRefraction();
        }

        if (!waterObjects.empty())
        {
            {
                ScopedTimer t(prof, ST_WATER_BEGIN);
                m_renderer.BeginWaterPass();
            }

            {
                ScopedTimer t(prof, ST_WATER_DRAW);
                int i = 0;
                for (WaterObject& obj : waterObjects)
                {
                    m_renderer.DrawWater(&obj, i);
                    i++;
                }
            }

            m_renderer.EndDrawWater();
        }

        {
            ScopedTimer t(prof, ST_BEGIN_FBO);
            m_renderer.BeginFrameBufferObjDraw();
        }

        {
            ScopedTimer t1(prof, ST_FRAMEBUFFEROBJ_DRAW);
            for (Object& obj : frameBufferObjects) m_renderer.FrameBufferDraw(obj.mesh.get(), obj.transform);
        }

        { ScopedTimer t(prof, ST_MAIN_END);     m_renderer.EndDraw(frameBufferOut); }

        prof.EndFrame();
    }
}

void Engine::destroy()
{
    m_renderer.Destroy();

    m_texture.Free();
    
    for (Object& obj : objects) {
        obj.mesh->Free();
    }
    
    printf("Killing engine \n");
}


void Engine::UpdateLight()
{
    m_renderer.UpdateLight(lights);
}

void Engine::CreateSkyBox(std::string path)
{
    Texture up;
    Texture down;
    Texture right;
    Texture left;
    Texture front;
    Texture back;

    std::string fullPath = "Assets/" + path + "/";

    std::string upPath = fullPath + "UP.png";
    std::string downPath = fullPath + "DOWN.png";
    std::string rightPath = fullPath + "RIGHT.png";
    std::string leftPath = fullPath + "LEFT.png";
    std::string frontPath = fullPath + "FRONT.png";
    std::string backPath = fullPath + "BACK.png";

    up.LoadFromFile(upPath.c_str());
    down.LoadFromFile(downPath.c_str());
    right.LoadFromFile(rightPath.c_str());
    left.LoadFromFile(leftPath.c_str());
    front.LoadFromFile(frontPath.c_str());
    back.LoadFromFile(backPath.c_str());

    GPU_HANDLE cubeMap = Graphics::CreateCubeMap(right.GetTextureBuffer(), left.GetTextureBuffer(), up.GetTextureBuffer(), down.GetTextureBuffer(), back.GetTextureBuffer(), front.GetTextureBuffer());
    m_renderer.SetSkybox(cubeMap);
}
