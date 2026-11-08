#pragma once
#include "../base/base-shader.h"
#include "Light.h"
#include "../../Particule.h"
#include "../../common/PerlinNoise2D.h"

struct PassCB
{
    glm::mat4x4 view;
    glm::mat4x4 proj;
    float time;
};

struct ObjectCB {
    glm::mat4x4 model;
    glm::vec3 min;
    glm::vec3 max;
};

struct WaterCB
{
    glm::mat4 model;
    glm::mat4 mirrorViewProj;
    glm::vec3 cameraPos;
    glm::vec3 cameraForward;
    float nearPlane;
    float farPlane;
    glm::vec3 deepColor;
    float absorption;
    float time;
    float waveAmp = 1.0f;
    float waveScale = 1.0f;
    float waveSpeed = 1.0f;
    //between 0 and 1
    float ondulationReflect = 0.03f;
    float refractionStrength = 30.0f;
    bool usLightEffect = false;
    int idWater;
};

VERTEX_SHADER(VertexDefault)
{

    BIND_R(PassCB, cbPass, 1)
    BIND_R(ObjectCB, cbObject, 2)

    VertexOut VS_MAIN(Vertex _in, uint64 id) override {
        VertexOut out;
        out.color = _in.color;
        out.uv = _in.uv;

        glm::mat4 model = cbObject().model;

        out.worldPos = glm::vec3(model * glm::vec4(_in.pos, 1.0f));

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        out.tbn[2] = glm::normalize(normalMatrix * _in.normal);

        out.pos = cbPass().proj * cbPass().view * model * glm::vec4(_in.pos, 1);
        return out;
    }
};

VERTEX_SHADER(VertexWater)
{
    BIND_R(PassCB, passCb, 1)

    BIND_R(WaterCB, waterCb, 2)

    float WaterPos(glm::vec2 p, float t)
    {
        const glm::vec2 dir[3] = { {1.0f, 0.2f}, {-0.6f, 0.8f}, {0.3f, -1.0f} };
        const float freq[3] = { 0.8f, 1.7f, 3.1f };
        const float amp[3] = { 0.05f, 0.025f, 0.012f };
        const float spd[3] = { 1.0f, 1.6f, 2.3f };

        float dy = 0.0f;
        for (int i = 0; i < 3; ++i)
        {
            glm::vec2 d = glm::normalize(dir[i]);
            float phase = glm::dot(d, p) * freq[i] * waterCb().waveScale + t * spd[i] * waterCb().waveSpeed;
            dy += waterCb().waveAmp * amp[i] * sinf(phase);
        }

        return dy;
    }

    VertexOut VS_MAIN(Vertex _in, uint64 id) override 
    {
        VertexOut out;
        glm::vec4 world = waterCb().model * glm::vec4(_in.pos, 1.0f);
        world.y += WaterPos(glm::vec2(_in.pos.x, _in.pos.z), waterCb().time);
        out.worldPos = glm::vec3(world);
        out.pos = passCb().proj * passCb().view * world;
        out.uv = _in.uv;
        out.tbn[2] = glm::vec3(0, 1, 0);
        return out;
    }
};

PIXEL_SHADER(PixelDefault)
{
    BIND_R(Buffer2D, texture, 0)

    Color PS_MAIN(VertexOut _in) override {
        return  Graphics::Sample(texture(), _in.uv.x, _in.uv.y);
    }
};

PIXEL_SHADER(PixelLightDefault)
{
    BIND_R(LightPass, light, 3)

    BIND_R(Buffer2D, texture, 0)

    BIND_R(Buffer2D, shadowAtlas, 4)

    BIND_R(glm::vec3, viewPos, 5)

    float ShadowFactor(int _i, const glm::vec3 & _pos, const glm::vec3 & _normal, const glm::vec3 & _lightDir)
    {
        const int res = shadowAtlas().GetWidth();
        const int yOffset = _i * res;

        float texelWorld = 20.0f / res;
        glm::vec3 offsetPos = _pos + _normal * texelWorld * 1.5f;
        //glm::vec3 offsetPos = _pos;

        glm::vec4 lp = light().lightViewProj[_i] * glm::vec4(offsetPos, 1.0f);
        lp /= lp.w;

        int px = (int)((lp.x * 0.5f + 0.5f) * res);
        int py = (int)((1.0f - (lp.y * 0.5f + 0.5f)) * res);

        if (px < 0 || py < 0 || px >= res || py >= res)
            return 1.0f;

        float cosTheta = glm::max(glm::dot(_normal, -_lightDir), 0.0f);
        float bias = glm::max(0.01f * (1.0f - cosTheta), 0.002f);

        float lit = 0.0f;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
            {
                int x = glm::clamp(px + dx, 0, res - 1);
                int y = glm::clamp(py + dy, 0, res - 1) + yOffset;
                lit += (lp.z - bias > shadowAtlas().Get<float>(x, y)) ? 0.0f : 1.0f;
            }

        return lit / 9.0f;
    }
    
    glm::vec3 BlingPhongShadow(glm::vec3 _albedo, glm::vec3 _normal, glm::vec3 _worldPos) {
        glm::vec3 ambient(0.1f);
        glm::vec3 diffuseSum(0.0f);
        glm::vec3 specularSum(0.0f);
        float specularStrength = 0.5f;
        float shininess = 32.f;

        glm::vec3 viewDir = glm::normalize(viewPos() - _worldPos);

        int count = static_cast<int>(light().lightCount);
        for (int i = 0; i < count; ++i)
        {
            const auto& L = light().lights[i];

            glm::vec3 lightDir = L.lightDir;
            float diff = glm::max(glm::dot(_normal, -lightDir), 0.0f);

            if (diff <= 0.0f)
                continue;

            float shadow = 1.0f;
            if (i < MAX_LIGHTS && diff > 0.0f)
                shadow = ShadowFactor(i, _worldPos, _normal, lightDir);
            
            glm::vec3 halfDir = glm::normalize(viewDir - lightDir);
            float spec = std::pow(glm::max(glm::dot(_normal, halfDir), 0.0f), shininess);
            
            diffuseSum  += L.color * (diff * L.strength) * shadow;
            specularSum += L.color * (specularStrength * spec * L.strength) * shadow;
        }
        
        return _albedo * (ambient + diffuseSum) + specularSum;
    }

    Color PS_MAIN(VertexOut _in) override
    {
        Color tex = Graphics::Sample(texture(), _in.uv.x, _in.uv.y);
        glm::vec3 albedo (tex.r, tex.g, tex.b);
        glm::vec3 normal = glm::normalize(_in.tbn[2]);
        
        glm::vec3 rgb = glm::clamp(BlingPhongShadow(albedo, normal, _in.worldPos),0.0f,1.0f);
        return Color(rgb.x, rgb.y, rgb.z, tex.a);

    }
};

PIXEL_SHADER(PixelLightColorDefault)
{
    BIND_R(LightPass, light, 3)

    BIND_R(Buffer2D, shadowAtlas, 4)

    BIND_R(glm::vec3, viewPos, 5)

    float ShadowFactor(int _i, const glm::vec3 & _pos, const glm::vec3 & _normal, const glm::vec3 & _lightDir)
    {
        const int res = shadowAtlas().GetWidth();
        const int yOffset = _i * res;

        float texelWorld = 20.0f / res;
        glm::vec3 offsetPos = _pos + _normal * texelWorld * 1.5f;
        //glm::vec3 offsetPos = _pos;

        glm::vec4 lp = light().lightViewProj[_i] * glm::vec4(offsetPos, 1.0f);
        lp /= lp.w;

        int px = (int)((lp.x * 0.5f + 0.5f) * res);
        int py = (int)((1.0f - (lp.y * 0.5f + 0.5f)) * res);

        if (px < 0 || py < 0 || px >= res || py >= res)
            return 1.0f;

        float cosTheta = glm::max(glm::dot(_normal, -_lightDir), 0.0f);
        float bias = glm::max(0.01f * (1.0f - cosTheta), 0.002f);

        float lit = 0.0f;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
            {
                int x = glm::clamp(px + dx, 0, res - 1);
                int y = glm::clamp(py + dy, 0, res - 1) + yOffset;
                lit += (lp.z - bias > shadowAtlas().Get<float>(x, y)) ? 0.0f : 1.0f;
            }

        return lit / 9.0f;
    }
    
    glm::vec3 BlingPhongShadow(glm::vec3 _albedo, glm::vec3 _normal, glm::vec3 _worldPos) {
        glm::vec3 ambient(0.1f);
        glm::vec3 diffuseSum(0.0f);
        glm::vec3 specularSum(0.0f);
        float specularStrength = 0.5f;
        float shininess = 32.f;

        glm::vec3 viewDir = glm::normalize(viewPos() - _worldPos);

        int count = static_cast<int>(light().lightCount);
        for (int i = 0; i < count; ++i)
        {
            const auto& L = light().lights[i];

            glm::vec3 lightDir = L.lightDir;
            float diff = glm::max(glm::dot(_normal, -lightDir), 0.0f);

            if (diff <= 0.0f)
                continue;

            float shadow = 1.0f;
            if (i < MAX_LIGHTS && diff > 0.0f)
                shadow = ShadowFactor(i, _worldPos, _normal, lightDir);
            
            glm::vec3 halfDir = glm::normalize(viewDir - lightDir);
            float spec = std::pow(glm::max(glm::dot(_normal, halfDir), 0.0f), shininess);
            
            diffuseSum  += L.color * (diff * L.strength) * shadow;
            specularSum += L.color * (specularStrength * spec * L.strength) * shadow;
        }
        
        return _albedo * (ambient + diffuseSum) + specularSum;
    }

    Color PS_MAIN(VertexOut _in) override
    {
        Color tex = _in.color;
        glm::vec3 albedo(tex.r, tex.g, tex.b);
        glm::vec3 normal = glm::normalize(_in.tbn[2]);
        
        glm::vec3 rgb = glm::clamp(BlingPhongShadow(albedo, normal, _in.worldPos),0.0f,1.0f);
        return Color(rgb.x, rgb.y, rgb.z, tex.a);
    }
};

PIXEL_SHADER(PixelColorDefault)
{
    Color PS_MAIN(VertexOut _in) override {
        return _in.color;
    }
};

PIXEL_SHADER(PixelParticule)
{
    BIND_R(Buffer2D, texture, 0);

    Color PS_MAIN(VertexOut _in) override
    {
        Color tex = Graphics::Sample(texture(), _in.uv.x, _in.uv.y);
        return tex;
    }
};


VERTEX_SHADER(VertexParticule)
{
    struct PassCB
    {
        glm::mat4x4 view;
        glm::mat4x4 proj;
    };

    BIND_R(PassCB, cbPass, 1)
    BIND_R(Particule* const, cbParticule, 2)

    VertexOut VS_MAIN(Vertex _in, uint64 id) override {
        VertexOut out;
        out.color = _in.color;
        out.uv = _in.uv;

        glm::mat4 model = cbParticule()[id].transform.getWorldMatrix();

        out.worldPos = glm::vec3(model * glm::vec4(_in.pos, 1.0f));

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
        out.tbn[2] = glm::normalize(normalMatrix * _in.normal);

        out.pos = cbPass().proj * cbPass().view * model * glm::vec4(_in.pos, 1);
        return out;
    }
};

VERTEX_SHADER(VertexShaderSkybox)
{
    BIND_R(PassCB, passCb, 1)
    
    VertexOut VS_MAIN(Vertex _in, uint64 id) override
    {
        VertexOut out;
        out.tbn[2] = _in.pos;
        glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(passCb().view));
        out.pos = passCb().proj * viewNoTranslation * glm::vec4(_in.pos, 1.0);
        return out;
    }
};

PIXEL_SHADER(PixelShaderSkybox)
{
    BIND_R(CubeMap, texture, 0)
    
    Color PS_MAIN(VertexOut _in) override {
        return Graphics::Sample(texture(), _in.tbn[2]);
    }
};

inline float LinearizeDepth(float d, float n, float f) {
    return 2.0f * n * f / (f + n - d * (f - n));
}

PIXEL_SHADER(PixelShaderWater)
{
    BIND_R(WaterCB, waterCb, 2)

    BIND_R(Buffer2D, refractionColor, 3)

    BIND_R(Buffer2D, sceneDepth, 4)

    BIND_R(Buffer2D, reflectionColor, 5)

    BIND_R(LightPass, light, 6);

    glm::vec3 WaterNormal(glm::vec2 p, float t)
    {
        const glm::vec2 dir[8] = {
                                    { 1.0f,  0.2f},   // houle principale (vent)
                                    { 0.8f,  0.6f},
                                    { 0.9f, -0.4f},
                                    {-0.6f,  0.8f},
                                    { 0.3f, -1.0f},
                                    {-0.9f, -0.3f},
                                    { 0.5f,  0.9f},
                                    {-0.4f, -0.9f}
                                        };
        const float freq[8] = { 0.8f, 1.1f, 1.7f, 2.3f, 3.1f, 4.3f, 5.9f, 8.1f };
        const float amp[8] = { 0.05f, 0.04f, 0.025f, 0.018f, 0.012f, 0.007f, 0.004f, 0.0025f };
        const float spd[8] = { 1.0f, 1.2f, 1.6f, 1.9f, 2.3f, 2.7f, 3.1f, 3.6f };
        const float phase[8] = { 0.0f, 1.3f, 2.7f, 4.1f, 5.5f, 0.9f, 3.3f, 2.1f };

        float dx = 0.0f;
        float dz = 0.0f;
        for (int i = 0; i < 5; ++i)
        {
            glm::vec2 d = glm::normalize(dir[i]);
            float c = waterCb().waveAmp * amp[i] * freq[i] * waterCb().waveScale * cosf(glm::dot(d, p) * freq[i] * waterCb().waveScale + t * spd[i] * waterCb().waveSpeed + phase[i]);
            dx += c * d.x;
            dz += c * d.y;
        }

        return glm::normalize(glm::vec3(-dx, 1.0f, -dz));
    }

    Color PS_MAIN(VertexOut _in) override 
    {
        int px = (int)_in.pos.x;
        int py = (int)_in.pos.y;
        int W = refractionColor().GetWidth();
        int H = refractionColor().GetHeight();
        float n = waterCb().nearPlane; 
        float f = waterCb().farPlane;

        //profondeur
        float rawDepth = sceneDepth().Get<float>(px, py);
        float sceneZ = (rawDepth >= std::numeric_limits<float>::max()) ? f : LinearizeDepth(rawDepth, n, f);
        float waterZ = LinearizeDepth(_in.pos.z, n, f);

        glm::vec3 bg = color::FromUint32(refractionColor().Get<uint32>(px, py));
        if (sceneZ <= waterZ)
            return glm::vec4(bg, 1.0f);

        float s = sceneZ / waterZ;
        float depthV = (waterCb().cameraPos.y - _in.worldPos.y) * (s - 1.0f);

        //normal
        glm::vec3 N = WaterNormal(glm::vec2(_in.worldPos.x, _in.worldPos.z), waterCb().time);

        //refraction decalee
        float strength = waterCb().refractionStrength * glm::clamp(depthV / 2.0f, 0.0f, 1.0f);
        int rx = glm::clamp(px + (int)(N.x * strength), 0, W - 1);
        int ry = glm::clamp(py + (int)(N.z * strength), 0, H - 1);

        float zr = sceneDepth().Get<float>(rx, ry);
        float zrLin = (zr >= std::numeric_limits<float>::max()) ? f : LinearizeDepth(zr, n, f);
        if (zrLin < waterZ)
        {
            rx = px;
            ry = py;
        }

        glm::vec3 refr = color::FromUint32(refractionColor().Get<uint32>(rx, ry));
        float k = 1.0f - expf(-depthV * waterCb().absorption);
        refr = glm::mix(refr, waterCb().deepColor, k);

        //refelxion
        glm::vec4 clip = waterCb().mirrorViewProj * glm::vec4(_in.worldPos, 1.0f);
        glm::vec3 refl = refr;
        if (clip.w > 0.0f)
        {
            glm::vec2 ndc = glm::vec2(clip.x, clip.y) / clip.w;
            glm::vec2 uv = ndc * 0.5f + 0.5f;
            uv.y = 1.0f - uv.y;
            uv += glm::vec2(N.x, N.z) * waterCb().ondulationReflect;

            int RW = reflectionColor().GetWidth();
            int RH = reflectionColor().GetHeight() / 2;
            int mx = glm::clamp((int)(uv.x * RW), 0, RW - 1);
            int my = glm::clamp((int)(uv.y * RH), 0, RH - 1) + waterCb().idWater * RH;
            refl = color::FromUint32(reflectionColor().Get<uint32>(mx, my));
        }

        //fresnel
        glm::vec3 V = glm::normalize(waterCb().cameraPos - _in.worldPos);
        float cosT = glm::clamp(glm::dot(N, V), 0.0f, 1.0f);
        float F = 0.02f + 0.98f * powf(1.0f - cosT, 5.0f);
        F = glm::clamp(F, 0.1f, 1.0f);

        glm::vec3 color = glm::mix(refr, refl, F);

        if (waterCb().usLightEffect)
        {
            for (int i = 0; i < light().lightCount; ++i)
            {
                const Light& Li = light().lights[i];

                glm::vec3 L;
                float attenuation = 1.0f;

                if (Li.type == DIRECTIONAL)
                {
                    L = glm::normalize(-Li.lightDir);
                }
                else
                {
                    glm::vec3 toLight = Li.pos - _in.worldPos;
                    float dist = glm::length(toLight);
                    if (dist > Li.range)
                        continue;

                    L = toLight / dist;

                    float cosAngle = glm::dot(-L, glm::normalize(Li.lightDir));
                    float cone = glm::smoothstep(Li.cosOuter, Li.cosInner, cosAngle);
                    if (cone <= 0.0f)
                        continue;

                    float d2 = dist * dist;
                    float r4 = Li.range * Li.range * Li.range * Li.range;
                    float window = glm::clamp(1.0f - (d2 * d2) / r4, 0.0f, 1.0f);
                    attenuation = cone * window * window / glm::max(d2, 0.01f);
                }

                glm::vec3 H = glm::normalize(L + V);
                float spec = powf(glm::max(glm::dot(N, H), 0.0f), 128.0f);
                glm::vec3 addedColor = light().lights[i].color * spec * attenuation * Li.strength;
                color += addedColor;
            }
        }
        return glm::vec4(glm::clamp(color, 0.0f, 1.0f), 1.0f);
    }
};