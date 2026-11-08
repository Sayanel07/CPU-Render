#pragma once
#include "blend.h"
#include "../../common/types.h"
#include "../../common/triangle.h"

#define BAND_HEIGHT 32
#define BAND_WIDTH 32

struct PixelShaderBase;
struct Buffer2D;

struct RasterizerArgs {
    Buffer2D* depthBuffer;
    Buffer2D* renderTarget;
    int32 width;
    int32 height;
    PixelShaderBase* pixelShader;
    int32 offsetX;
    int32 offsetY;
    bool isDepthWriting;
    bool useBlend;
    BlendDescriptor blendDescriptor;
};

struct Rasterizer
{
private:
    struct TriangleData
    {
        const VertexOut *a, *b, *c;
        int minX, maxX, minY, maxY;
        float u0, v0;
        float dudx, dudy, dvdx, dvdy;
        float iw0, iw1, iw2;
        bool valid = false;
    };
    
	static void ClipSpaceToScreenSpace(Triangle& _triangle, int32 _width, int32 _height);
	static void GenerateBoundingBox(Triangle& _triangle, int32 _width, int32 _height);

public:
	static void RasterizeFrame(const RasterizerArgs& args, std::vector<Triangle>& _triangles);
};

