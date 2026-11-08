#pragma once
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include "../main.h"
#include <chrono>

//DEBUG// AI GENERATED
enum Stage {
    ST_PREPARE, ST_UPDATE_LIGHT, ST_SHADOW_BEGIN, ST_SHADOW_CAST, ST_SHADOW_END, ST_REFLECTION_BEGIN, ST_REFLECTION_DRAW, ST_REFLECTION_END, ST_MAIN_BEGIN, ST_MAIN_DRAW, ST_PARTICUL, ST_COPY_SCENE,
    ST_WATER_BEGIN, ST_WATER_DRAW, ST_BEGIN_FBO, ST_FRAMEBUFFEROBJ_DRAW, ST_MAIN_END, ST_COUNT
};

static const char* STAGE_NAMES[ST_COUNT] = {
    "PrepareShadows","UpdateLight", "BeginShadowPass", "CastShadow", "EndShadowPass", "BeginReflectionPass", "DrawReflection", "EndReflectionPass",
     "BeginDraw", "Draw", "ParticuleDraw", "CopyScene", "WaterBegin", "WaterDraw", "BeginFBO", "FrameObjectDraw", "EndDraw"
};

struct Profiler
{
    using Clock = std::chrono::steady_clock;   // monotone, haute résolution
    double total[ST_COUNT] = {};               // ms accumulées
    int frames = 0;

    void Add(Stage s, Clock::duration d)
    {
        total[s] += std::chrono::duration<double, std::milli>(d).count();
    }

    void EndFrame()
    {
        if (++frames < 60) return;             // affichage toutes les 60 frames
        double sum = 0;
        for (int i = 0; i < ST_COUNT; ++i) sum += total[i];
        double avg = sum / frames;
        printf("---- moyenne sur %d frames : %.2f ms/frame soit %.0f fps ----\n", frames, avg, 1000.0f /avg);
        for (int i = 0; i < ST_COUNT; ++i)
            printf("%-16s %7.3f ms  (%4.1f %%)\n", STAGE_NAMES[i], total[i] / frames, 100.0 * total[i] / sum);
        for (double& t : total) t = 0;
        frames = 0;
    }
};

struct ScopedTimer   // chronomètre une portée, ajoute le résultat à la sortie du bloc
{
    Profiler& p; Stage s; Profiler::Clock::time_point t0;
    ScopedTimer(Profiler& _p, Stage _s) : p(_p), s(_s), t0(Profiler::Clock::now()) {}
    ~ScopedTimer() { p.Add(s, Profiler::Clock::now() - t0); }
};
//DEBUG//

class Utils
{
public:
	static uint32_t RGB_255_ToInt(int r, int g, int b);
	static uint32_t RGB_255_ToInt(glm::vec3 color);
	static void DrawLine(glm::vec2 point1, glm::vec2 point2, FinalFrameBuffer& frameBufferOut, uint32_t color);
};

