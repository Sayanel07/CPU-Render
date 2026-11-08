#include "shader-descriptor.h"

#include "graphics.h"

ShaderDescriptor::ShaderDescriptor(const ShaderDescriptor& _descriptor) {
    m_entries = _descriptor.m_entries;
    VS = _descriptor.VS;
    PS = _descriptor.PS;
    CS = _descriptor.CS;
}

void ShaderDescriptor::SetEntry(uint8 _index, ShaderInput _input) {
    assert(_index < MAX_ENTRIES);
    assert((_input.format & SHADER) == false);
    if (m_entries.size() <= _index)
        m_entries.resize(_index + 1);
    
    m_entries[_index] = _input;
}

void ShaderDescriptor::SetVS(GPU_HANDLE _vs) {
    assert(_vs.GetFormat() & Format::VERTEX_SHADER);
    VS = _vs;
}

void ShaderDescriptor::SetPS(GPU_HANDLE _ps) {
    assert(_ps.GetFormat() & Format::PIXEL_SHADER);
    PS = _ps;
}

void ShaderDescriptor::SetCS(GPU_HANDLE _cs) {
    assert(_cs.GetFormat() & Format::COMPUTE_SHADER);
    CS = _cs;
}

ShaderInput ShaderDescriptor::GetEntry(uint8 _index) const {
    assert(_index < m_entries.size());
    return m_entries[_index];
}

uint8 ShaderDescriptor::GetSize() const {
    return static_cast<uint8>(m_entries.size());
}

GPU_HANDLE ShaderDescriptor::GetVertexShader() const {
    return VS;
}

GPU_HANDLE ShaderDescriptor::GetPixelShader() const {
    return PS;
}

GPU_HANDLE ShaderDescriptor::GetComputeShader() const {
    return CS;
}

std::vector<ShaderInput> ShaderDescriptor::GetInputs() const {
    return m_entries;
}

void ShaderDescriptor::Free() {
    Graphics::FreeResource(VS);
    VS = {};
    Graphics::FreeResource(PS);
    PS = {};
    Graphics::FreeResource(CS);
    CS = {};
}
