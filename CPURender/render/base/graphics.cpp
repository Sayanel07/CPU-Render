#include "graphics.h"

#include <stdexcept>

#include "../../common/color.h"
#include "../../common/triangle.h"
#include "mesh.h"
#include "base-shader.h"
#include "clipping.h"
#include "rasterizer.h"

void Graphics::Draw(const Mesh& _mesh, uint32 count) {
    assert(m_state.m_descriptor.GetVertexShader().IsValid());
    assert(m_state.m_descriptor.GetPixelShader().IsValid() || m_state.m_isRTUsed == false);
    assert(m_state.m_renderTarget.IsValid() || m_state.m_isRTUsed == false);
    assert(m_state.m_isRTUsed || m_state.m_isDBTest);
    assert(m_state.m_depthBuffer.IsValid() || m_state.m_isDBTest == false);
    assert(m_state.m_isDBTest == false || m_state.m_isRTUsed == false || m_state.m_buffer2D[m_state.m_renderTarget.GetIndex()].GetWidth() == m_state.m_buffer2D[m_state.m_depthBuffer.GetIndex()].GetWidth());
    assert(m_state.m_isDBTest == false || m_state.m_isRTUsed == false ||  m_state.m_buffer2D[m_state.m_renderTarget.GetIndex()].GetHeight() == m_state.m_buffer2D[m_state.m_depthBuffer.GetIndex()].GetHeight());

    BindTable table;
    for (uint32 i = 0; i < m_state.m_descriptor.GetSize(); i++)
        table.slots[i] = Resolve(m_state.m_entries[i]);
    
    Buffer2D* renderTarget = nullptr;
    Buffer2D* depthBuffer = nullptr;

    uint32 width;
    uint32 height;
    uint32 offsetX = 0;
    uint32 offsetY = 0;

    if (m_state.m_isDBTest) {
        depthBuffer = &m_state.m_buffer2D[m_state.m_depthBuffer.GetIndex()];
        width = depthBuffer->GetWidth();
        height = depthBuffer->GetHeight();
        if (m_state.m_isRTUsed)
            renderTarget = &m_state.m_buffer2D[m_state.m_renderTarget.GetIndex()];
    }
    else {
        renderTarget = &m_state.m_buffer2D[m_state.m_renderTarget.GetIndex()];
        width = renderTarget->GetWidth();
        height = renderTarget->GetHeight();
    }

    if (m_state.m_viewPort.usCustom)
    {
        width = m_state.m_viewPort.w;
        height = m_state.m_viewPort.h;
        offsetX += m_state.m_viewPort.x;
        offsetY += m_state.m_viewPort.y;
    }
    
    if (m_state.m_isRTUsed)
    {
        for (uint32 i = 0; i < m_state.m_descriptor.GetSize(); i++)
            assert(table.slots[i] != renderTarget);
    }

    VertexShaderBase* vertexShader = dynamic_cast<VertexShaderBase*>(m_state.m_shader[m_state.m_descriptor.GetVertexShader().GetIndex()].get());
    assert(vertexShader != nullptr);
    vertexShader->entries = table.slots;

    PixelShaderBase* pixelShader = nullptr;
    if (m_state.m_isRTUsed) {
        pixelShader = dynamic_cast<PixelShaderBase*>(m_state.m_shader[m_state.m_descriptor.GetPixelShader().GetIndex()].get());
        pixelShader->entries = table.slots;
    }
    
    Buffer* vertexBuffer = &m_state.m_buffer[_mesh.GetVertexBuffer().GetIndex()];
    Buffer* indexBuffer = &m_state.m_buffer[_mesh.GetIndexBuffer().GetIndex()];
    
    // Vertex Shader
    uint64 nbrOfVertices = vertexBuffer->GetSize() / sizeof(Vertex);
    std::vector<VertexOut> vOut(nbrOfVertices * count);
    #pragma omp parallel for
    for (uint32 c = 0; c < count; c++) {
        #pragma omp parallel for
        for (uint64 i = 0; i < nbrOfVertices; i++)
            vOut[c * nbrOfVertices + i] = vertexShader->VS_MAIN(vertexBuffer->GetData<Vertex>()[i], c);
    }

    uint64 nbrOfIndices = indexBuffer->GetSize() / sizeof(uint32);
    std::vector<Triangle> geoTriangle;
    geoTriangle.reserve((nbrOfIndices / 3) * count);
    for (uint32 c = 0; c < count; c++) {
        for (uint64 i = 0; i + 2 < nbrOfIndices; i += 3)
        {
            // Triangle build
            VertexOut triangle[3] = {
                vOut[c * nbrOfVertices + indexBuffer->GetData<uint32>()[i]],
                vOut[c * nbrOfVertices + indexBuffer->GetData<uint32>()[i + 1]],
                vOut[c * nbrOfVertices + indexBuffer->GetData<uint32>()[i + 2]]
            };

            // BackFaceCulling
            if (m_state.m_backFaceCulling)
            {
                if (!IsFacingCamera(triangle))
                    continue;
            }

            Triangle tri;
            tri.vertices = {
                triangle[0],
                triangle[1],
                triangle[2] };

            //Clipping
            std::vector<Triangle> tris = Clipping::ClipTriangle(tri);

            for (Triangle& t : tris)
            {
                // Perspective Divide
                for (VertexOut& v : t.vertices) {
                    float invW = 1.0f / v.pos.w;
                    v.pos.x *= invW;
                    v.pos.y *= invW;
                    v.pos.z *= invW;
                }
                geoTriangle.push_back(t);
            }
        }
    }
    
    // Rasterizing
    RasterizerArgs args;
    args.pixelShader = pixelShader;
    args.renderTarget = renderTarget;
    args.depthBuffer = depthBuffer;
    args.isDepthWriting = m_state.m_isDBWrite && m_state.m_isBlendUsed == false;
    args.width = static_cast<int32>(width);
    args.height = static_cast<int32>(height);
    args.offsetX = static_cast<int32>(offsetX);
    args.offsetY = static_cast<int32>(offsetY);
    args.useBlend = m_state.m_isBlendUsed;
    args.blendDescriptor = m_state.m_blendDescriptor;
    Rasterizer::RasterizeFrame(args, geoTriangle);
}

void Graphics::Dispatch(glm::uvec3 dispatchThreadDimension) {
    assert(m_state.m_descriptor.GetComputeShader().IsValid());

    BindTable table;
    for (uint32 i = 0; i < m_state.m_descriptor.GetSize(); i++)
        table.slots[i] = Resolve(m_state.m_entries[i]);
    
    ComputeShaderBase* computeShader = dynamic_cast<ComputeShaderBase*>(m_state.m_shader[m_state.m_descriptor.GetComputeShader().GetIndex()].get());
    assert(computeShader != nullptr);
    computeShader->entries = table.slots;
    
    // Execute Compute
    uint64 size = static_cast<uint64>(dispatchThreadDimension.z) * dispatchThreadDimension.y * dispatchThreadDimension.x;
    #pragma omp parallel for
    for (uint64 i = 0; i < size; i++) {
        uint32 z = static_cast<uint32>(i / (static_cast<uint64>(dispatchThreadDimension.y) * dispatchThreadDimension.x));
        uint32 rem = static_cast<uint32>(i % (static_cast<uint64>(dispatchThreadDimension.y) * dispatchThreadDimension.x));
        uint32 y = rem / dispatchThreadDimension.x;
        uint32 x = rem % dispatchThreadDimension.x;
        computeShader->CS_MAIN({x,y,z});
    }
}

void Graphics::SetShader(const ShaderDescriptor& shader) {
    m_state.m_descriptor = shader;
    for (GPU_HANDLE& e : m_state.m_entries) 
        e = GPU_HANDLE{};
}

void Graphics::SetEntryBuffer(uint8 _register, GPU_HANDLE hwnd) {
    ShaderInput input = m_state.m_descriptor.GetEntry(_register);
    if ((input.format & hwnd.GetFormat()) == 0)
        return;
    
    if (hwnd.GetFormat() & BUFFER1D) {
        uint64 size = GetSize(hwnd);
        if (input.size < size)
            return;
    }
    
    m_state.m_entries[_register] = hwnd;
}

void Graphics::SetRenderTarget(GPU_HANDLE _renderTargetHandle, bool _isUsed) {
    assert(_renderTargetHandle.GetFormat() & Format::BUFFER2D || (_renderTargetHandle.IsValid() == false && _isUsed == false));
    m_state.m_isRTUsed = _isUsed;
    m_state.m_renderTarget = _renderTargetHandle;
}

void Graphics::SetRenderTargetUse(bool _isUsed) {
    assert(m_state.m_renderTarget.GetFormat() & Format::BUFFER2D);
    m_state.m_isRTUsed = _isUsed;
}

void Graphics::SetDepthBuffer(GPU_HANDLE _depthBufferHandle, bool _isTestUsed, bool _isWriteUsed) {
    assert(_isTestUsed || _isWriteUsed == false);
    assert(_depthBufferHandle.GetFormat() & Format::BUFFER2D || ( _depthBufferHandle.IsValid() == false && _isTestUsed == false));
    m_state.m_isDBTest = _isTestUsed;
    m_state.m_isDBWrite = _isWriteUsed;
    m_state.m_depthBuffer = _depthBufferHandle;
}

void Graphics::SetDepthBufferUse( bool _isTestUsed, bool _isWriteUsed) {
    assert(m_state.m_depthBuffer.GetFormat() & Format::BUFFER2D);
    assert(_isTestUsed || _isWriteUsed == false);
    m_state.m_isDBTest = _isTestUsed;
    m_state.m_isDBWrite = _isWriteUsed;
}

void Graphics::SetBlendDescriptor(BlendDescriptor _descriptor, bool _isUsed) {
    m_state.m_isBlendUsed = _isUsed;
    m_state.m_blendDescriptor = _descriptor;
}

void Graphics::SetBlendDescriptorUse(bool _isUsed) {
    m_state.m_isBlendUsed = _isUsed;
}

void Graphics::ClearDrawTarget(Color _color) {
    // Clear Both Render Target and Depth Buffer
    if (m_state.m_renderTarget.IsValid())
        ClearRenderTarget(_color);
    
    if (m_state.m_depthBuffer.IsValid())
        ClearDepth(m_state.m_depthBuffer);
}

void Graphics::ClearRenderTarget(Color _color) {
    assert(m_state.m_renderTarget.IsValid());

    Buffer2D* rT = &m_state.m_buffer2D[m_state.m_renderTarget.GetIndex()];
    uint32 height = rT->GetHeight();
    uint32 width = rT->GetWidth();
    uint64 size = static_cast<uint64>(rT->GetHeight()) * rT->GetWidth();
    std::vector<uint32> clear(size, color::ToUint32(_color));
    rT->SetData(clear,width,height);
}

void Graphics::ClearDepth(GPU_HANDLE _buffer) 
{
    assert(_buffer.IsValid());

    Buffer2D& depth = m_state.m_buffer2D[_buffer.GetIndex()];
    depth.Fill<float>(std::numeric_limits<float>::max());
    

   /* Buffer2D* depth = &m_state.m_buffer2D[_buffer.GetIndex()];
    uint32 height = depth->GetHeight();
    uint32 width = depth->GetWidth();
    uint64 size = static_cast<uint64>(depth->GetHeight()) * depth->GetWidth();
    std::vector<float> clear(size,std::numeric_limits<float>::max());
    depth->SetData(clear, width, height);*/
}

void Graphics::SetBackFaceCulling(BackfaceCullingType _type, bool _isUsed) {
    m_state.m_backFaceCulling = _isUsed;
    m_state.m_backfaceCullingType = _type;
}

void Graphics::SetBackFaceCullingUse(bool _isUsed) {
    m_state.m_backFaceCulling = _isUsed;
}

BackfaceCullingType Graphics::GetBackFaceCullingType()
{
    return m_state.m_backfaceCullingType;
}

GPU_HANDLE Graphics::CreateBuffer(const void* _data, uint64 _size) {
    Buffer buffer(_data,_size);
    uint32 id =  m_state.m_buffer.Add(buffer);
    return GPU_HANDLE(id, Format::BUFFER1D);
}

GPU_HANDLE Graphics::CreateBuffer(uint64 _size) {
    Buffer buffer(_size);
    uint32 id =  m_state.m_buffer.Add(buffer);
    return GPU_HANDLE(id, Format::BUFFER1D);
}

GPU_HANDLE Graphics::CreateTextureBuffer() {
    Buffer2D buffer;
    uint32 id =  m_state.m_buffer2D.Add(buffer);
    return GPU_HANDLE(id, Format::BUFFER2D);
}

GPU_HANDLE Graphics::CreateTextureBuffer(uint32 _width, uint32 _height, uint32 _formatSize) {
    Buffer2D buffer(_width, _height, _formatSize);
    uint32 id =  m_state.m_buffer2D.Add(buffer);
    return GPU_HANDLE(id, Format::BUFFER2D);
}

GPU_HANDLE Graphics::CreateCubeMap(GPU_HANDLE right, GPU_HANDLE left, GPU_HANDLE top, GPU_HANDLE bottom, GPU_HANDLE back, GPU_HANDLE front) {
    assert(right.GetFormat() & Format::BUFFER2D);
    assert(left.GetFormat() & Format::BUFFER2D);
    assert(top.GetFormat() & Format::BUFFER2D);
    assert(bottom.GetFormat() & Format::BUFFER2D);
    assert(back.GetFormat() & Format::BUFFER2D);
    assert(front.GetFormat() & Format::BUFFER2D);
    
    CubeMap cubeMap;
    cubeMap.map[0] = right;
    cubeMap.map[1] = left;
    cubeMap.map[2] = top;
    cubeMap.map[3] = bottom;
    cubeMap.map[4] = back;
    cubeMap.map[5] = front;
    uint32 id =  m_state.m_cubeMap.Add(cubeMap);
    return GPU_HANDLE(id, Format::CUBEMAP);
}

void Graphics::CopyTextureBuffer(GPU_HANDLE _to, GPU_HANDLE _from)
{
    assert(_to.IsValid() && _from.IsValid());
    assert(!(_to == _from));

    Buffer2D& dst = m_state.m_buffer2D[_to.GetIndex()];
    const Buffer2D& src = m_state.m_buffer2D[_from.GetIndex()];

    dst.CopyFrom(src);
}

void Graphics::SetUserClipPlane(glm::vec4 pos, glm::mat4x4 viewProj)
{
    Clipping::SetUserClipPlane(pos, viewProj);
}

void Graphics::DisableUserClipPlane()
{
    Clipping::DisableUserClipPlane();
}

void Graphics::EnableUserClipPlane()
{
    Clipping::EnableUserClipPlane();
}

void Graphics::SetBuffer(GPU_HANDLE _hwnd, const void* _data, uint64 _size, uint64 _index) {
    assert(_hwnd.GetFormat() & Format::BUFFER1D);
    m_state.m_buffer[_hwnd.GetIndex()].SetData(_data, _size, _index);
}

std::vector<char> Graphics::ReadTextureBuffer(GPU_HANDLE _hwnd, uint32 _mipLevel) {
    assert(_hwnd.GetFormat() & Format::BUFFER2D);
    Buffer2D* buffer = &m_state.m_buffer2D[_hwnd.GetIndex()];
    return buffer->GetDecodedData(_mipLevel);
}

void Graphics::FreeResource(GPU_HANDLE _hwnd) {
    uint8 f = _hwnd.GetFormat();
    if (f & Format::BUFFER1D)
        m_state.m_buffer.Remove(_hwnd.GetIndex());
    else if (f & Format::BUFFER2D)
        m_state.m_buffer2D.Remove(_hwnd.GetIndex());
    else if (f & Format::SHADER)
        m_state.m_shader.Remove(_hwnd.GetIndex());
    else if (f & Format::CUBEMAP)
        m_state.m_cubeMap.Remove(_hwnd.GetIndex());
}

uint64 Graphics::GetSize(GPU_HANDLE _hwnd) {
    uint8 f = _hwnd.GetFormat();
    if (f & Format::BUFFER1D)
        return m_state.m_buffer[_hwnd.GetIndex()].GetSize();
    if (f & Format::BUFFER2D)
        return static_cast<uint64>(m_state.m_buffer2D[_hwnd.GetIndex()].GetWidth()) * m_state.m_buffer2D[_hwnd.GetIndex()].GetHeight();
    if (f & Format::SHADER)
        return MAX_ENTRIES * sizeof(void*);
    
    throw std::runtime_error("Graphics::GetSize(): Invalid format");
}

void Graphics::SetViewPort(int32 x, int32 y, int32 w, int32 h)
{
    m_state.m_viewPort = { x, y, w, h, true };
}

void Graphics::ResetViewPort()
{
    m_state.m_viewPort = { 0, 0, 0, 0, false };
}

Color Graphics::Sample(const CubeMap& _cubeMap, const glm::vec3& _dir, FilteringMode _fMode, WrapMode _wMode)
{
    // Code sample inspired by : https://www.reddit.com/r/opengl/comments/1hdr6rj/cubemap_coordinates_to_texture_2d_conversion/
    glm::vec3 absDir = glm::abs(_dir);
    int faceIndex;
    glm::vec2 uv;

    if (absDir.x >= absDir.y && absDir.x >= absDir.z) {
        if (_dir.x >= 0.0f) {
            faceIndex = 0;
            uv = glm::vec2(-_dir.z, -_dir.y);
        }
        else {      
            faceIndex = 1;
            uv = glm::vec2(_dir.z, -_dir.y);
        }
        uv /= absDir.x;
    }
    else if (absDir.y >= absDir.x && absDir.y >= absDir.z) {
        if (_dir.y >= 0.0f) {
            faceIndex = 2;
            uv = glm::vec2(-_dir.z, _dir.x);
        }
        else {
            faceIndex = 3;
            uv = glm::vec2(_dir.z, _dir.x);
        }
        uv /= absDir.y;
    }
    else {
        if (_dir.z >= 0.0f) {
            faceIndex = 4;
            uv = glm::vec2(_dir.x, -_dir.y);
        }
        else {
            faceIndex = 5;
            uv = glm::vec2(-_dir.x, -_dir.y);
        }
        uv /= absDir.z;
    }
    
    uv = uv * 0.5f + 0.5f;
    return Sample(m_state.m_buffer2D[_cubeMap.map[faceIndex].GetIndex()], uv.x, uv.y, 0, _fMode, _wMode);
}

Color Graphics::SampleLOD(const Buffer2D& _frame, float _u, float _v, glm::vec2 _dX, glm::vec2 _dY,  FilteringMode _fMode, WrapMode _wMode) {
    float lod = FindLODLevel(_frame, _dX, _dY);
    float lod0 = std::floor(lod);
    float lod1 = std::ceil(lod);
    float l = lod - lod0; // after comma
    
    Color c0 = Sample(_frame, _u, _v, static_cast<uint32>(lod0), _fMode, _wMode);
    Color c1 = Sample(_frame, _u, _v, static_cast<uint32>(lod1), _fMode, _wMode);
    
    return glm::mix(c0, c1, l);
}

Color Graphics::Sample(const Buffer2D& _frame, float _u, float _v, uint32 _mipLevel, FilteringMode _fMode, WrapMode _wMode) {
    int32 w = static_cast<int32>(_frame.GetWidth(_mipLevel));
    int32 h = static_cast<int32>(_frame.GetHeight(_mipLevel));
    
    switch (_fMode)
    {
    case FilteringMode::NEAREST: 
        {
            int32 x = static_cast<int32>(std::lround(_u * static_cast<float>(w)));
            int32 y = static_cast<int32>(std::lround(_v * static_cast<float>(h)));
            glm::ivec2 r = Wrap(x, y, w, h, _wMode);
            x = r.x; y = r.y;
            return color::FromUint32(_frame.Get<uint32>(x, y, _mipLevel));
        }
    case FilteringMode::BILINEAR:
        {
            float xm = _u * w - 0.5f;
            float ym = _v * h - 0.5f;
    
            int32 x = static_cast<int32>(std::floor(xm));
            int32 y = static_cast<int32>(std::floor(ym));
            float fx = xm - x;
            float fy = ym - y;
            Color t00 = color::FromUint32(_frame.Get<uint32>(Wrap(x,y,w,h,_wMode), _mipLevel));   
            Color t01 = color::FromUint32(_frame.Get<uint32>(Wrap(x,y + 1,w,h,_wMode), _mipLevel));   
            Color t10 = color::FromUint32(_frame.Get<uint32>(Wrap(x + 1,y,w,h,_wMode), _mipLevel));   
            Color t11 = color::FromUint32(_frame.Get<uint32>(Wrap(x + 1,y + 1,w,h,_wMode), _mipLevel));
    
            Color mix = glm::mix(glm::mix(t00, t10, fx),glm::mix(t01, t11, fx),fy);
            return mix;
        }
    }
}

Color Graphics::Sample(const Buffer2D& _frame, glm::vec2 _uv, uint32 _mipLevel, FilteringMode _fMode, WrapMode _wMode) {
    return Sample(_frame, _uv.x, _uv.y, _mipLevel, _fMode, _wMode);
}

float Graphics::FindLODLevel(const Buffer2D& _frame, glm::vec2 _dx, glm::vec2 _dy) {
    glm::vec2 texSize(static_cast<float>(_frame.GetWidth()), static_cast<float>(_frame.GetHeight()));
    glm::vec2 dx = _dx * texSize;
    glm::vec2 dy = _dy * texSize;
    float rho = glm::max(glm::dot(dx, dx), glm::dot(dy, dy));
    float lod  = 0.5f * std::log2(glm::max(rho, 1e-8f));
    return std::clamp(lod, 0.f, static_cast<float>(_frame.GetMipCount()) - 1);
}

float Graphics::SampleFloat(const Buffer2D& _frame, float _u, float _v, uint32 _mipLevel, FilteringMode _fMode, WrapMode _wMode)
{
    int32 w = static_cast<int32>(_frame.GetWidth(_mipLevel));
    int32 h = static_cast<int32>(_frame.GetHeight(_mipLevel));
    
    switch (_fMode)
    {
    case FilteringMode::NEAREST: 
        {
            int32 x = static_cast<int32>(std::lround(_u * static_cast<float>(w)));
            int32 y = static_cast<int32>(std::lround(_v * static_cast<float>(h)));
            glm::ivec2 r = Wrap(x, y, w, h, _wMode);
            x = r.x; y = r.y;
            return _frame.Get<float>(x, y, _mipLevel);
        }
    case FilteringMode::BILINEAR:
        {
            float xm = _u * w - 0.5f;
            float ym = _v * h - 0.5f;
    
            int32 x = static_cast<int32>(std::floor(xm));
            int32 y = static_cast<int32>(std::floor(ym));
            float fx = xm - x;
            float fy = ym - y;
            float t00 = _frame.Get<float>(Wrap(x,y,w,h,_wMode), _mipLevel);   
            float t01 = _frame.Get<float>(Wrap(x,y + 1,w,h,_wMode), _mipLevel);   
            float t10 = _frame.Get<float>(Wrap(x + 1,y,w,h,_wMode), _mipLevel);   
            float t11 = _frame.Get<float>(Wrap(x + 1,y + 1,w,h,_wMode), _mipLevel);
    
            float mix = glm::mix(glm::mix(t00, t10, fx),glm::mix(t01, t11, fx),fy);
            return mix;
        }
    }
}

float Graphics::SampleFloat(const Buffer2D& _frame, glm::vec2 _uv, uint32 _mipLevel, FilteringMode _fMode, WrapMode _wMode)
{
    return SampleFloat(_frame, _uv.x, _uv.y, _mipLevel, _fMode, _wMode);
}

bool Graphics::IsFacingCamera(VertexOut _triangle[3]) {
    float det = glm::determinant(glm::mat3(
        glm::vec3(_triangle[0].pos.x, _triangle[0].pos.y, _triangle[0].pos.w),
        glm::vec3(_triangle[1].pos.x, _triangle[1].pos.y, _triangle[1].pos.w),
        glm::vec3(_triangle[2].pos.x, _triangle[2].pos.y, _triangle[2].pos.w)));

    switch (m_state.m_backfaceCullingType) {
    case CLOCK_WISE :
        if (det <= 0.0f) return false;
        break;
    case COUNTER_CLOCK_WISE:
        if (det > 0.0f) return false;
        break;
    }

    return true;
}

void* Graphics::Resolve(GPU_HANDLE _hwnd) {
    if (_hwnd.GetFormat() & Format::BUFFER1D)
        return m_state.m_buffer[_hwnd.GetIndex()].GetData<void>();
    if (_hwnd.GetFormat() & Format::BUFFER2D)
        return &m_state.m_buffer2D[_hwnd.GetIndex()];
    if (_hwnd.GetFormat() & Format::CUBEMAP)
        return &m_state.m_cubeMap[_hwnd.GetIndex()];
    return nullptr;
}

glm::ivec2 Graphics::Wrap(int32 _x, int32 _y, int32 _w, int32 _h, WrapMode _wMode)
{
    glm::ivec2 result;
    switch (_wMode)
    {
    case WrapMode::CLAMP: {
            result.x = std::clamp(_x, 0, _w-1);
            result.y = std::clamp(_y, 0, _h-1);
            break;
        }
    case WrapMode::REPEAT: {
            int32 rX = _x % _w;
            result.x = rX < 0 ? rX + _w : rX;
            int32 rY = _y % _h;
            result.y = rY < 0 ? rY + _h : rY;
            break;
        }
    case WrapMode::MIRROR: {
            int32 period = 2 * _w;
            int32 r = _x % period;
            r = r < 0 ? r + _w : r;
            r = r < _w ? r : period - 1 - r;
            result.x = r;
        
            period = 2 * _h;
            r = _y % period;
            r = r < 0 ? r + _h : r;
            r = r < _h ? r : period - 1 - r;
            result.y = r;
            break;
        }
    }
    return result;
}