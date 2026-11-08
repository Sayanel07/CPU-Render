#include "rasterizer.h"

#include "base-shader.h"
#include "buffer2D.h"
#include "../../common/color.h"

void Rasterizer::ClipSpaceToScreenSpace(Triangle& _triangle, int32 _width, int32 _height)
{
	//pos between -1,-1(top left) and 1, 1(bottom right) to the actual resolution
	for (VertexOut& vertice : _triangle.vertices)
	{
		//convert to screenPos
		vertice.pos.x = ((vertice.pos.x + 1.0f) * static_cast<float>(_width)) / 2.0f;
		vertice.pos.y = ((1.0f - vertice.pos.y) * static_cast<float>(_height)) / 2.0f;
	}
}

void Rasterizer::GenerateBoundingBox(Triangle& _triangle, int32 _width, int32 _height)
{
	_triangle.min.x = std::min({ _triangle.vertices[0].pos.x, _triangle.vertices[1].pos.x, _triangle.vertices[2].pos.x });
	_triangle.min.y = std::min({ _triangle.vertices[0].pos.y, _triangle.vertices[1].pos.y, _triangle.vertices[2].pos.y });
	_triangle.max.x = std::max({ _triangle.vertices[0].pos.x, _triangle.vertices[1].pos.x, _triangle.vertices[2].pos.x });
	_triangle.max.y = std::max({ _triangle.vertices[0].pos.y, _triangle.vertices[1].pos.y, _triangle.vertices[2].pos.y });

	_triangle.min.x = static_cast<float>(std::max(0, static_cast<int>(std::floor(_triangle.min.x))));
	_triangle.max.x = static_cast<float>(std::min(_width - 1, static_cast<int>(std::ceil(_triangle.max.x))));
	_triangle.min.y = static_cast<float>(std::max(0, static_cast<int>(std::floor(_triangle.min.y))));
    _triangle.max.y = static_cast<float>(std::min(_height - 1, static_cast<int>(std::ceil(_triangle.max.y))));
}

void Rasterizer::RasterizeFrame(const RasterizerArgs& args, std::vector<Triangle>& _triangles) {
    std::vector<TriangleData> triangleDatas(_triangles.size());
    // Pass 1 : Per-triangle data setup so we compute some of the data before doing rasterizing
    #pragma omp parallel for
    for (uint64 i = 0; i < _triangles.size(); ++i)
    {
        Triangle& t = _triangles[i];
        TriangleData& data = triangleDatas[i];

        ClipSpaceToScreenSpace(t, args.width, args.height);
        GenerateBoundingBox(t, args.width, args.height);

        data.a = &t.vertices[0]; 
        data.b = &t.vertices[1]; 
        data.c = &t.vertices[2];
        
        data.minX = (int)t.min.x; 
        data.maxX = (int)t.max.x;
        data.minY = (int)t.min.y; 
        data.maxY = (int)t.max.y;

        glm::vec2 p0(data.a->pos.x, data.a->pos.y);
        glm::vec2 v0 = glm::vec2(data.b->pos.x, data.b->pos.y) - p0;
        glm::vec2 v1 = glm::vec2(data.c->pos.x, data.c->pos.y) - p0;

        float area = v0.x * v1.y - v0.y * v1.x;
        if (std::abs(area) < 1e-6f) continue;
        float invArea = 1.0f / area;

        data.dudx =  v1.y * invArea;  
        data.dudy = -v1.x * invArea;
        data.dvdx = -v0.y * invArea;  
        data.dvdy =  v0.x * invArea;

        glm::vec2 d0 = glm::vec2(data.minX + 0.5f, data.minY + 0.5f) - p0;
        data.u0 = (d0.x * v1.y - d0.y * v1.x) * invArea;
        data.v0 = (v0.x * d0.y - v0.y * d0.x) * invArea;

        data.iw0 = 1.0f / data.a->pos.w;
        data.iw1 = 1.0f / data.b->pos.w;
        data.iw2 = 1.0f / data.c->pos.w;
        data.valid = true;
    }
    
    int tilesX = (args.width  + BAND_WIDTH  - 1) / BAND_WIDTH; // ceiling for integer
    int tilesY = (args.height + BAND_HEIGHT - 1) / BAND_HEIGHT; // ceiling for integer
    int tileCount = tilesX * tilesY;
    
    // Set triangle only in tiles it's in
    std::vector<std::vector<uint32>> tileTris(tileCount);
    for (uint32 i = 0; i < triangleDatas.size(); ++i) {
        if (!triangleDatas[i].valid)
            continue;
        int minY = triangleDatas[i].minY / BAND_HEIGHT; // floor
        int maxY = triangleDatas[i].maxY / BAND_HEIGHT; // floor so we compensate with <=
        int minX = triangleDatas[i].minX / BAND_WIDTH; // floor
        int maxX = triangleDatas[i].maxX / BAND_WIDTH; // floor so we compensate with <=
        
        for (int ty = minY; ty <= maxY; ty++)
            for (int tx = minX; tx <= maxX; tx++)
                tileTris[ty * tilesX + tx].push_back(i);
    }
    
    // Pass 2 : Do the work by tile of 32 by 32
    #pragma omp parallel for schedule(dynamic)
    for (int tile = 0; tile < tileCount; tile++)
    {
        int x0 = (tile % tilesX) * BAND_WIDTH; // Get us the screen coordinate by multiplying by BAND_WIDTH
        int x1 = std::min(x0 + BAND_WIDTH,  args.width)  - 1; // We limit the width of the tile in case it goes out of screen
        int y0 = (tile / tilesX) * BAND_HEIGHT; // Get us the screen coordinate by multiplying by BAND_HEIGHT
        int y1 = std::min(y0 + BAND_HEIGHT, args.height) - 1; // We limit the height of the tile in case it goes out of screen
        
        for (const uint32& i : tileTris[tile]) {
            const TriangleData& data = triangleDatas[i];
            
            int yStart = std::max(data.minY, y0); // We take either the min of the triangle or the min of the tile in case it goes out of the tile
            int yEnd = std::min(data.maxY, y1); // We take either the max of the triangle or the max of the tile in case it goes out of the tile
            int xStart = std::max(data.minX, x0); // We take either the min of the triangle or the min of the tile in case it goes out of the tile
            int xEnd = std::min(data.maxX, x1); // We take either the max of the triangle or the max of the tile in case it goes out of the tile

            for (int y = yStart; y <= yEnd; ++y) {
                // No iteration so no dependency
                float uRow = data.u0 + (y - data.minY) * data.dudy; // Offset so we start at y = 0 if yStart is data.minY
                float vRow = data.v0 + (y - data.minY) * data.dvdy; // Offset so we start at y = 0 if yStart is data.minY

                for (int x = xStart; x <= xEnd; ++x) {
                    // No iteration so no dependency
                    float u = uRow + (x - data.minX) * data.dudx; // Offset so we start at x = 0 if xStart is data.minX
                    float v = vRow + (x - data.minX) * data.dvdx; // Offset so we start at x = 0 if xStart is data.minX
                    // glm::vec2 d(x + 0.5f - p0.x, y + 0.5f - p0.y);
                    // float u = (d.x * v1.y - d.y * v1.x) * invArea;
                    // float v = (v0.x * d.y - v0.y * d.x) * invArea;
                    float w = 1.0f - u - v;
                    if (u < 0.f || v < 0.f || w < 0.f) 
                        continue;

                    int dx = x + args.offsetX;
                    int dy = y + args.offsetY;

                    float z = data.a->pos.z * w + data.b->pos.z * u + data.c->pos.z * v;
                    if (args.depthBuffer != nullptr) 
                    {
                        if (z >= args.depthBuffer->Get<float>(dx, dy)) 
                            continue;
                    }

                    if (args.renderTarget == nullptr) {
                        if (args.isDepthWriting)
                            args.depthBuffer->Set<float>(&z, dx, dy);
                        continue;
                    }


                    glm::vec3 l(w * data.iw0, u * data.iw1, v * data.iw2);
                    l /= (l.x + l.y + l.z);

                    VertexOut out;
                    out.pos = glm::vec4(x + 0.5f, y + 0.5f, z, 1.0f);
                    out.color = data.a->color * l.x + data.b->color * l.y + data.c->color * l.z;
                    out.uv = data.a->uv * l.x + data.b->uv * l.y + data.c->uv * l.z;
                    
                    float uX = u + data.dudx;
                    float vX = v + data.dvdx;
                    float wX = 1.0f - uX - vX;
                    glm::vec3 lX = glm::vec3(wX * data.iw0, uX * data.iw1, vX * data.iw2);
                    lX /= (lX.x + lX.y + lX.z);
                    
                    glm::vec2 uvX = data.a->uv * lX.x + data.b->uv * lX.y + data.c->uv * lX.z;
                    
                    float uY = u + data.dudy;
                    float vY = v + data.dvdy;
                    float wY = 1.0f - uY - vY;
                    glm::vec3 lY = glm::vec3(wY * data.iw0, uY * data.iw1, vY * data.iw2);
                    lY /= (lY.x + lY.y + lY.z);
                    
                    glm::vec2 uvY = data.a->uv * lY.x + data.b->uv * lY.y + data.c->uv * lY.z;
                    
                    out.duvX = uvX - out.uv;
                    out.duvY = uvY - out.uv;
                    
                    out.worldPos = data.a->worldPos * l.x + data.b->worldPos * l.y + data.c->worldPos * l.z;
                    out.tbn = data.a->tbn * l.x + data.b->tbn * l.y + data.c->tbn * l.z;
                    
                    // Pixel shader
                    Color colorOut = args.pixelShader->PS_MAIN(out);
                    if (colorOut.a == PIXEL_DISCARD)
                        continue;
                    if (args.useBlend) {
                        Color dest =  color::FromUint32(args.renderTarget->Get<uint32>(static_cast<uint32>(dx), static_cast<uint32>(dy)));
                        colorOut = args.blendDescriptor.Blend(colorOut, dest);
                    }
                    if (args.isDepthWriting) {
                        args.depthBuffer->Set<float>(&z, dx, dy);
                    }
                    uint32 c = color::ToUint32(colorOut);
                    args.renderTarget->Set(&c, static_cast<uint32>(dx), static_cast<uint32>(dy), false);
                }
            }
        }
    }
}
