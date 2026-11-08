#pragma once
#include "../../base/base-shader.h"
#include "../shaders.h"
#include "../../base/graphics.h"

struct MaterialCB
{
    Color color = {1.f,1.f,1.f,1.f};
    bool useNormal = false;
    bool useAlbedo = false;
    bool useMetallic = false;
    bool useRoughness = false;
    bool useLight = true;
};

VERTEX_SHADER(LitVertexShader) {
    
    BIND_R(PassCB, cbPass, 0)
    BIND_R(ObjectCB, cbObject, 4)
    
    inline VertexOut VS_MAIN(Vertex _in, uint64 id) override {
        VertexOut out;
        out.color = _in.color;
        out.uv = _in.uv;

        glm::mat4 model = cbObject().model;

        out.worldPos = glm::vec3(model * glm::vec4(_in.pos, 1.0f));

        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        glm::vec3 T = glm::normalize(glm::vec3(model * glm::vec4(_in.tangent, 0.0)));
        glm::vec3 B = glm::normalize(glm::vec3(model * glm::vec4(_in.biTangent, 0.0)));
        glm::vec3 N = glm::normalize(glm::vec3(normalMatrix * glm::vec4(_in.normal, 0.0)));
        out.tbn = glm::mat3(T, B, N);
        
        out.pos = cbPass().proj * cbPass().view * model * glm::vec4(_in.pos, 1);
        return out;
    }
};

inline glm::vec3 F_Schlick(glm::vec3 F0, float cosTheta) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// GGX Normal Distribution
// Took from this article : https://learnopengl.com/PBR/Theory
inline float D_GGX(float nh, float a) { 
    float a2 = a * a;
    float denom = nh * nh * (a2 - 1.0f) + 1.0f;
    return a2 / (glm::pi<float>() * denom * denom);
}

// Smith GGX Geometry
// Stolen from this article : https://github.khronos.org/Vulkan-Site/tutorial/latest/Building_a_Simple_Engine/Lighting_Materials/04_lighting_implementation.html
inline float G_SmithGGX(float nv, float nl, float a) {
    float k = a / 2.0f; // Direct lighting version
    
    float Gv = nv / (nv * (1.0f - k) + k);
    float Gl = nl / (nl * (1.0f - k) + k);
    return Gv * Gl;
}

// ACES Tone mapper : https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
inline glm::vec3 ACESFilm(glm::vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return glm::clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.f, 1.f);
}

inline Color TriplanarSample(const Buffer2D& t, glm::vec3 p, glm::vec3 normal, glm::vec2 min, glm::vec2 max, float tiling = 0.2f) {
    glm::vec3 tw = glm::pow(glm::abs(normal), glm::vec3(4.0f));
    tw /= (tw.x + tw.y + tw.z);

    glm::vec3 q = p * tiling;

    // Inset by half a texel so bilinear filtering never reads a neighboring tile
    glm::vec2 texel = 1.0f / glm::vec2(t.GetWidth(), t.GetHeight());
    glm::vec2 lo = min + texel * 0.5f;
    glm::vec2 hi = max - texel * 0.5f;

    auto sampleTile = [&](glm::vec2 c) -> glm::vec4 {
        glm::vec2 f = c - glm::floor(c);          // repeat inside the tile
        glm::vec2 uv = lo + f * (hi - lo);        // remap to the atlas region
        return Graphics::Sample(t, uv, 0, BILINEAR);
    };

    glm::vec4 sx = sampleTile({q.y, q.z});
    glm::vec4 sy = sampleTile({q.x, q.z});
    glm::vec4 sz = sampleTile({q.x, q.y});

    return sx * tw.x + sy * tw.y + sz * tw.z;
}

PIXEL_SHADER(LitPixelShader) {
    
    BIND_R(LightPass, light, 1)
    BIND_R(Buffer2D, shadowAtlas, 2)
    BIND_R(glm::vec3, viewPos, 3)
    BIND_R(MaterialCB, cbMaterial, 5)
    BIND_R(Buffer2D, texture, 6)
    BIND_R(Buffer2D, normalMap, 7)
    BIND_R(Buffer2D, metallicMap, 8)
    BIND_R(Buffer2D, roughnessMap, 9)

    inline float ShadowFactor(int _i, const glm::vec3 & _pos, const glm::vec3 & _normal, const glm::vec3 & _lightDir)
    {
        const int res = shadowAtlas().GetWidth();
        const int yOffset = _i * res;

        float texelWorld = 20.0f / res;
        glm::vec3 offsetPos = _pos + _normal * texelWorld * 1.5f;

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
    
    inline glm::vec3 BlingPhongShadow(glm::vec3 _albedo, glm::vec3 _normal, glm::vec3 _worldPos, float metallic, float roughness) {
        const float minimumRoughness = 0.04f;
        const float epsilon = 0.0001f;
        
        glm::vec3 viewDir = glm::normalize(viewPos() - _worldPos);
        
        float r = glm::clamp(roughness, minimumRoughness, 1.0f);
        float a = r * r;
        float m = glm::clamp(metallic,0.f,1.f);
        
        float nv = glm::clamp(glm::dot(_normal, viewDir),0.f,1.f);

        // F0 for dielectrics is 0.04, for metals this the albedo
        glm::vec3 F0 = glm::mix(glm::vec3(0.04f, 0.04f, 0.04f), glm::vec3(_albedo), m);
    
        glm::vec3 diffuseColor = glm::mix(glm::vec3(_albedo), glm::vec3(0.0f, 0.0f, 0.0f), m); // Metals have no diffuse, dielectrics do

        glm::vec3 Lo(0.0f);
        
        int count = static_cast<int>(light().lightCount);
        for (int i = 0; i < count; ++i)
        {
            const Light& L = light().lights[i];

            glm::vec3 lightDir;
            float attenuation = 1.0f;

            if (L.type == DIRECTIONAL)
            {
                lightDir = L.lightDir;
            }
            else
            {
                glm::vec3 toSurface = _worldPos - L.pos;
                float dist = glm::length(toSurface);
                if (dist >= L.range)
                    continue;

                lightDir = toSurface / dist;
                
                float cosAngle = glm::dot(lightDir, glm::normalize(L.lightDir));
                float cone = glm::smoothstep(L.cosOuter, L.cosInner, cosAngle);
                if (cone <= 0.0f)
                    continue;

                float d2 = dist * dist;
                float window = glm::clamp(1.0f - (d2 * d2) / (L.range * L.range * L.range * L.range), 0.0f, 1.0f);
                attenuation = cone * window * window / glm::max(d2, 0.01f);
            }

            float diff = glm::max(glm::dot(_normal, -lightDir), 0.0f);

            if (diff <= 0.0f)
                continue;

            float shadow = 1.0f;
            if (i < MAX_LIGHTS && diff > 0.0f)
                shadow = ShadowFactor(i, _worldPos, _normal, lightDir);
            
            glm::vec3 halfDir = glm::normalize(viewDir - lightDir);
            float nh = glm::clamp(glm::dot(_normal, halfDir), 0.0f, 1.0f);
            float lh = glm::clamp(glm::dot(-lightDir, halfDir), 0.0f, 1.0f);
            
            // Fresnel
            glm::vec3 F = F_Schlick(F0, lh);
        
            // Normal Distribution
            float D = D_GGX(nh, a);
        
            // Geometry
            float G = G_SmithGGX(nv, diff, a);

            // Cook-Torrance specular
            glm::vec3 specular = (D * F * G) / std::max(4.0f * nv * diff, epsilon); // https://learnopengl.com/PBR/Lighting
        
            // Lambertian diffuse (only for non-metals)
            glm::vec3 diffuse = diffuseColor / glm::pi<float>(); 

            // Energy conservation
            glm::vec3 kD = (1.0f - F);
        
            Lo += (kD * diffuse + specular) * glm::vec3(L.color) * L.strength * diff * shadow * attenuation;
        }
        
        glm::vec3 ambientFresnel = F_Schlick(F0, nv);
        glm::vec3 ambient = light().ambient * glm::mix(diffuseColor, F0, ambientFresnel);
    
        return Lo + ambient;
    }

    inline Color PS_MAIN(VertexOut _in) override
    {
        glm::vec4 albedo;
        if (cbMaterial().useAlbedo) {
            
            Color tex = Graphics::SampleLOD(texture(), _in.uv.x, _in.uv.y, _in.duvX, _in.duvY, BILINEAR);
            albedo = tex;
        }
        else {
            albedo = glm::clamp(_in.color * cbMaterial().color, 0.f,1.f);
        }
        
        glm::vec3 normal;
        if (cbMaterial().useNormal) {
            Color sNormal = Graphics::Sample(normalMap(), _in.uv, 0, BILINEAR);
            glm::vec3 nTS = glm::vec3(sNormal) * 2.0f - 1.0f;
            normal = glm::normalize(_in.tbn * nTS);
        }
        else {
            normal = glm::normalize(_in.tbn[2]);
        }
        
        float metallic = 0.0f;
        if (cbMaterial().useMetallic)
        {
            metallic = color::ToUint32(Graphics::Sample(metallicMap(), _in.uv, 0, BILINEAR));
        }
        
        float roughness = 0.5f;
        if (cbMaterial().useRoughness)
        {
            roughness = color::ToUint32(Graphics::Sample(roughnessMap(), _in.uv, 0, BILINEAR));
        }
        
        if (cbMaterial().useLight)
        {
            glm::vec3 rgb = ACESFilm(BlingPhongShadow(albedo, normal, _in.worldPos, metallic, roughness));
            return { rgb.x, rgb.y, rgb.z, albedo.a };
        }
        else
            return { albedo.x, albedo.y, albedo.z, albedo.a };
    }
};

PIXEL_SHADER(HeightMapPixelShader)
{
    BIND_R(LightPass, light, 1)
    BIND_R(Buffer2D, shadowAtlas, 2)
    BIND_R(glm::vec3, viewPos, 3)
    BIND_R(ObjectCB, cbObject, 4)
    BIND_R(MaterialCB, cbMaterial, 5)
    BIND_R(Buffer2D, texture, 6)
    BIND_R(Buffer2D, normalMap, 7)
    BIND_R(Buffer2D, metallicMap, 8)
    BIND_R(Buffer2D, roughnessMap, 9)
    
    inline float ShadowFactor(int _i, const glm::vec3 & _pos, const glm::vec3 & _normal, const glm::vec3 & _lightDir)
    {
        const int res = shadowAtlas().GetWidth();
        const int yOffset = _i * res;

        float texelWorld = 20.0f / res;
        glm::vec3 offsetPos = _pos + _normal * texelWorld * 1.5f;

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
    
    inline glm::vec3 BlingPhongShadow(glm::vec3 _albedo, glm::vec3 _normal, glm::vec3 _worldPos, float metallic, float roughness) {
        const float minimumRoughness = 0.04f;
        const float epsilon = 0.0001f;
        
        glm::vec3 viewDir = glm::normalize(viewPos() - _worldPos);
        
        float r = glm::clamp(roughness, minimumRoughness, 1.0f);
        float a = r * r;
        float m = glm::clamp(metallic,0.f,1.f);
        
        float nv = glm::clamp(glm::dot(_normal, viewDir),0.f,1.f);

        // F0 for dielectrics is 0.04, for metals this the albedo
        glm::vec3 F0 = glm::mix(glm::vec3(0.04f, 0.04f, 0.04f), glm::vec3(_albedo), m);
    
        glm::vec3 diffuseColor = glm::mix(glm::vec3(_albedo), glm::vec3(0.0f, 0.0f, 0.0f), m); // Metals have no diffuse, dielectrics do

        glm::vec3 Lo(0.0f);
        
        int count = static_cast<int>(light().lightCount);
        for (int i = 0; i < count; ++i)
        {
            const Light& L = light().lights[i];

            glm::vec3 lightDir;
            float attenuation = 1.0f;

            if (L.type == DIRECTIONAL)
            {
                lightDir = L.lightDir;
            }
            else
            {
                glm::vec3 toSurface = _worldPos - L.pos;
                float dist = glm::length(toSurface);
                if (dist >= L.range)
                    continue;

                lightDir = toSurface / dist;

                float cosAngle = glm::dot(lightDir, glm::normalize(L.lightDir));
                float cone = glm::smoothstep(L.cosOuter, L.cosInner, cosAngle);
                if (cone <= 0.0f)
                    continue;

                float d2 = dist * dist;
                float window = glm::clamp(1.0f - (d2 * d2) / (L.range * L.range * L.range * L.range), 0.0f, 1.0f);
                attenuation = cone * window * window / glm::max(d2, 0.01f);
            }

            float diff = glm::max(glm::dot(_normal, -lightDir), 0.0f);

            if (diff <= 0.0f)
                continue;

            float shadow = 1.0f;
            if (i < MAX_LIGHTS && diff > 0.0f)
                shadow = ShadowFactor(i, _worldPos, _normal, lightDir);
            
            glm::vec3 halfDir = glm::normalize(viewDir - lightDir);
            float nh = glm::clamp(glm::dot(_normal, halfDir), 0.0f, 1.0f);
            float lh = glm::clamp(glm::dot(-lightDir, halfDir), 0.0f, 1.0f);
            
            // Fresnel
            glm::vec3 F = F_Schlick(F0, lh);
        
            // Normal Distribution
            float D = D_GGX(nh, a);
        
            // Geometry
            float G = G_SmithGGX(nv, diff, a);

            // Cook-Torrance specular
            glm::vec3 specular = (D * F * G) / std::max(4.0f * nv * diff, epsilon); // https://learnopengl.com/PBR/Lighting
        
            // Lambertian diffuse (only for non-metals)
            glm::vec3 diffuse = diffuseColor / glm::pi<float>(); 

            // Energy conservation
            glm::vec3 kD = (1.0f - F);
        
            Lo += (kD * diffuse + specular) * glm::vec3(L.color) * L.strength * diff * shadow * attenuation;
        }
        
        glm::vec3 ambientFresnel = F_Schlick(F0, nv);
        glm::vec3 ambient = light().ambient * glm::mix(diffuseColor, F0, ambientFresnel);
    
        return Lo + ambient;
    }
    
    enum LevelName {
        GRASS,
        ROCK,
        SAND,
        SNOW
    };
    
    struct Level {
        LevelName name;
        float metallic;
        float roughness;
        float weight;
    };
    
    Color PS_MAIN(VertexOut _in) override
    {
        std::vector<Level> levels = {
            {GRASS, 0.f, 0.63f, 0.f},
            {ROCK, 0.f, 0.9f, 0.f},
            {SAND, 0.f, 0.8f ,0.f},
            {SNOW, 0.f, 0.7f, 0.f}
        };
        uint32 width = 4;
        uint32 height = levels.size() / width;
        
        glm::vec3 normal = glm::normalize(_in.tbn[2]);
        glm::vec3 p = _in.worldPos;

        // Found these parameters on the internet
        float flat = glm::smoothstep(0.55f, 0.90f, normal.y);
        float low = 1.0f - glm::smoothstep(cbObject().min.y, glm::mix(cbObject().min.y, cbObject().max.y, 0.5f) , p.y);
        float high = glm::smoothstep(glm::mix(cbObject().min.y, cbObject().max.y, 0.8f), cbObject().max.y, p.y);
 
        float wSand  = flat * low;
        float wSnow  = flat * high;
        float wGrass = flat * (1.0f - low) * (1.0f - high);
        float wRock  = 1.0f - flat;
 
        // Divide all weight by the sum of all of them so the new sum would be one
        float sum = wSand + wGrass + wRock + wSnow;
        levels[0].weight = wGrass / sum;
        levels[1].weight = wRock / sum;
        levels[2].weight = wSand / sum;
        levels[3].weight = wSnow / sum;
 
        // Base metallic and roughness
        float metallic  = 0.0f;
        float roughness = 0.0f;
        
        glm::vec2 size = {1.f / width, 1.f / height};
        
        glm::vec4 albedo(0.0f);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Level& l = levels[y * width + x];
                if (l.weight < 1e-2f)
                    continue;
                albedo += TriplanarSample(texture(), p, normal, size * glm::vec2{x,y}, size * glm::vec2{x+1,y+1}) * l.weight;
                metallic += l.metallic * l.weight;
                roughness += l.roughness * l.weight;
            }
        }
 
        glm::vec3 rgb = ACESFilm(BlingPhongShadow(albedo, normal, p, metallic, roughness));
        return { rgb.x, rgb.y, rgb.z, 1.0f };
    }
};