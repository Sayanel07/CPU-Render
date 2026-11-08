#pragma once
#include <memory>
#include <vector>

#include "../../common/types.h"
#include "../../common/pool.h"
#include "common.h"
#include "buffer.h"
#include "buffer2D.h"
#include "base-shader.h"
#include "blend.h"
#include "cube-map.h"
#include "shader-descriptor.h"

struct Mesh;

enum FilteringMode : uint8
{
    NEAREST = 0,
    BILINEAR,
};

enum WrapMode : uint8
{
    REPEAT = 0,
    CLAMP,
    MIRROR,
};

struct Viewport 
{
    int32 x;
    int32 y;
    int32 w;
    int32 h;

    bool usCustom;
};

enum BackfaceCullingType : uint8
{
    CLOCK_WISE,
    COUNTER_CLOCK_WISE
};

struct BindTable {
    void* slots[MAX_ENTRIES] = {};
};

struct GraphicsState final {
    GraphicsState(const GraphicsState& _other) = delete;
    GraphicsState(GraphicsState&& _other) noexcept = delete;
    GraphicsState& operator=(const GraphicsState& _other) = delete;
    GraphicsState& operator=(GraphicsState&& _other) noexcept = delete;

    GraphicsState() = default;
    ~GraphicsState() = default;
private:
    ShaderDescriptor m_descriptor;
    
    GPU_HANDLE m_entries[MAX_ENTRIES];
    
    GPU_HANDLE m_renderTarget;
    bool m_isRTUsed;
    GPU_HANDLE m_depthBuffer;
    bool m_isDBTest;
    bool m_isDBWrite;
    
    Pool<Buffer> m_buffer;
    Pool<Buffer2D> m_buffer2D;
    Pool<CubeMap> m_cubeMap;
    Pool<std::unique_ptr<CompiledShader>> m_shader;
    
    BackfaceCullingType m_backfaceCullingType = CLOCK_WISE;
    bool m_backFaceCulling = true;

    Viewport m_viewPort;
    
    BlendDescriptor m_blendDescriptor;
    bool m_isBlendUsed = false;
    
    friend class Graphics;
};

class Graphics final
{
public:
    static void Draw(const Mesh& _mesh, uint32 count = 1);
    static void Dispatch(glm::uvec3 dispatchThreadDimension);
    
    static void SetShader(const ShaderDescriptor& shader);
    static void SetEntryBuffer(uint8 _register, GPU_HANDLE hwnd);
    static void SetRenderTarget(GPU_HANDLE _renderTargetHandle, bool _isUsed = true);
    static void SetRenderTargetUse(bool _isUsed);
    static void SetDepthBuffer(GPU_HANDLE _depthBufferHandle, bool _isTestUsed = true, bool _isWriteUsed = true);
    static void SetDepthBufferUse( bool _isTestUsed, bool _isWriteUsed);
    
    static void SetBlendDescriptor(BlendDescriptor _descriptor, bool _isUsed = true);
    static void SetBlendDescriptorUse(bool _isUsed);
    
    static void ClearDrawTarget(Color _color = {0,0,0,1});
    static void ClearRenderTarget(Color _color = {0,0,0,1});
    static void ClearDepth(GPU_HANDLE _buffer);
    
    static void SetBackFaceCulling(BackfaceCullingType _type, bool _isUsed = true);
    static void SetBackFaceCullingUse(bool _isUsed);
    static BackfaceCullingType GetBackFaceCullingType();
    
    static GPU_HANDLE CreateBuffer(const void* _data, uint64 _size);
    static GPU_HANDLE CreateBuffer(uint64 _size);
    static GPU_HANDLE CreateTextureBuffer();
    static GPU_HANDLE CreateTextureBuffer(uint32 _width, uint32 _height, uint32 _formatSize);
    static GPU_HANDLE CreateCubeMap(GPU_HANDLE right, GPU_HANDLE left, GPU_HANDLE top, GPU_HANDLE bottom, GPU_HANDLE back, GPU_HANDLE front);
    
    static void CopyTextureBuffer(GPU_HANDLE _to, GPU_HANDLE _from);

    static void SetUserClipPlane(glm::vec4 pos, glm::mat4x4 viewProj);
    static void DisableUserClipPlane();
    static void EnableUserClipPlane();

    template<typename T>
    static GPU_HANDLE CompileShader(const T& _shader);
    template<typename T>
    static GPU_HANDLE CompileShader(T&& _shader);
    
    static void SetBuffer(GPU_HANDLE _hwnd, const void* _data, uint64 _size, uint64 _index = 0);
    template<typename T>
    static void SetTextureBuffer(GPU_HANDLE _hwnd, const std::vector<T>& _data, uint32 _width, uint32 _height, uint32 _mipLevels = 1);
    template<typename T>
    static void SetTextureBuffer(GPU_HANDLE _hwnd, const T* _data, uint32 _width, uint32 _height, uint32 _mipLevels = 1);
    
    template<typename T>
    static T ReadBuffer(GPU_HANDLE _hwnd);
    static std::vector<char> ReadTextureBuffer(GPU_HANDLE _hwnd, uint32 _mipLevel = 0);
    
    static void FreeResource(GPU_HANDLE _hwnd);
    static uint64 GetSize(GPU_HANDLE _hwnd);

    static void SetViewPort(int32 x, int32 y, int32 w, int32 h);
    static void ResetViewPort();

    static Color Sample(const CubeMap& _cubeMap, const glm::vec3& _dir, FilteringMode _fMode = NEAREST, WrapMode _wMode = CLAMP);
    
    static Color SampleLOD(const Buffer2D& _frame, float _u, float _v, glm::vec2 _dX, glm::vec2 _dY,  FilteringMode _fMode = NEAREST, WrapMode _wMode = CLAMP);
    
    static Color Sample(const Buffer2D& _frame, float _u, float _v, uint32 _mipLevel = 0, FilteringMode _fMode = NEAREST, WrapMode _wMode = CLAMP);
    static Color Sample(const Buffer2D& _frame, glm::vec2 _uv, uint32 _mipLevel = 0, FilteringMode _fMode = NEAREST, WrapMode _wMode = CLAMP);

    static float SampleFloat(const Buffer2D& _frame, float _u, float _v, uint32 _mipLevel = 0, FilteringMode _fMode = NEAREST, WrapMode _wMode = CLAMP);
    static float SampleFloat(const Buffer2D& _frame, glm::vec2 _uv, uint32 _mipLevel = 0, FilteringMode _fMode = NEAREST, WrapMode _wMode = CLAMP);
    
    static float FindLODLevel(const Buffer2D& _frame, glm::vec2 _dx, glm::vec2 _dy);
    
    static glm::ivec2 Wrap(int32 _x, int32 _y, int32 _w, int32 _h, WrapMode _wMode);
private:
    static void* Resolve(GPU_HANDLE _hwnd);
    static bool IsFacingCamera(VertexOut _triangle[3]);
    
    static inline GraphicsState m_state = GraphicsState();
};

template <typename T>
void Graphics::SetTextureBuffer(GPU_HANDLE _hwnd, const std::vector<T>& _data, uint32 _width, uint32 _height, uint32 _mipLevels) {
    assert(_hwnd.GetFormat() & Format::BUFFER2D);
    m_state.m_buffer2D[_hwnd.GetIndex()].SetData<T>(_data, _width, _height, _mipLevels);
}

template <typename T>
void Graphics::SetTextureBuffer(GPU_HANDLE _hwnd, const T* _data, uint32 _width, uint32 _height,uint32 _mipLevels) {
    assert(_hwnd.GetFormat() & Format::BUFFER2D);
    m_state.m_buffer2D[_hwnd.GetIndex()].SetData<T>(_data, _width, _height, _mipLevels);
}

template <typename T>
T Graphics::ReadBuffer(GPU_HANDLE _hwnd) {
    assert(_hwnd.GetFormat() & Format::BUFFER1D);
    return *m_state.m_buffer[_hwnd.GetIndex()].GetData<T>();
}

template<typename T>
GPU_HANDLE Graphics::CompileShader(const T& _shader) {
    static_assert(std::is_base_of_v<CompiledShader, T>, "T must derive from CompiledShader");
    uint32 id = m_state.m_shader.Add(std::make_unique<T>(_shader));
    return GPU_HANDLE(id, _shader.GetFormat());
}

template<typename T>
GPU_HANDLE Graphics::CompileShader(T&& _shader) {
    static_assert(std::is_base_of_v<CompiledShader, T>, "T must derive from CompiledShader");
    Format f = _shader.GetFormat();
    uint32 id = m_state.m_shader.Add(std::make_unique<T>(std::move(_shader)));
    return GPU_HANDLE(id, f);
}
