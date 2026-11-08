#pragma once
#include "../../common/types.h"
#include "../../common/vertex.h"
#include "common.h"
#include "buffer2D.h"

struct CompiledShader
{
    virtual Format GetFormat() const = 0;
    
    void** entries = nullptr;
    
    virtual ~CompiledShader() = default;
};

struct VertexShaderBase : CompiledShader
{
    // Put all your variables here
    virtual VertexOut VS_MAIN(Vertex _in, uint64 id) = 0;
    Format GetFormat() const override {return Format::VERTEX_SHADER;}
    
    virtual ~VertexShaderBase() = default;
};

struct PixelShaderBase : CompiledShader
{
    // Put all your variables here
    virtual Color PS_MAIN(VertexOut _in) = 0;
    Format GetFormat() const override {return Format::PIXEL_SHADER;}
    
    virtual ~PixelShaderBase() = default;
};

struct ComputeShaderBase : CompiledShader
{
    virtual void CS_MAIN(glm::uvec3 dispatchThreadID) = 0;
    Format GetFormat() const override {return Format::COMPUTE_SHADER;}
    
    virtual ~ComputeShaderBase() = default;
};

#define VERTEX_SHADER(name) struct name : VertexShaderBase
#define PIXEL_SHADER(name) struct name : PixelShaderBase
#define COMPUTE_SHADER(name) struct name : ComputeShaderBase

#define BIND_R(Type, name, slot) \
const Type& name() const { \
    assert((slot) < MAX_ENTRIES); \
    return *static_cast<Type*>(entries[slot]); \
} \

#define BIND_RW(Type, name, slot) \
Type& name() const { \
assert((slot) < MAX_ENTRIES); \
return *static_cast<Type*>(entries[slot]); \
} \

#define DISCARD glm::vec4(0.0f,0.0f,0.0f,0.0f)