#pragma once
#include <vector>

#include "common.h"

struct ShaderDescriptor
{
    ShaderDescriptor() = default;
    explicit ShaderDescriptor(const ShaderDescriptor& _descriptor);
    
    ShaderDescriptor& operator=(const ShaderDescriptor& _descriptor) = default;
    
    void SetEntry(uint8 _index, ShaderInput _input);
    void SetVS(GPU_HANDLE _vs);
    void SetPS(GPU_HANDLE _ps);
    void SetCS(GPU_HANDLE _cs);
    
    ShaderInput GetEntry(uint8 _index) const;
    uint8 GetSize() const;
    GPU_HANDLE GetVertexShader() const;
    GPU_HANDLE GetPixelShader() const;
    GPU_HANDLE GetComputeShader() const;
    
    bool operator==(const ShaderDescriptor& _descriptor) const {
        return VS == _descriptor.VS && PS == _descriptor.PS && CS == _descriptor.CS;
    }
    
    bool operator!=(const ShaderDescriptor& _descriptor) const {
        return VS != _descriptor.VS || PS != _descriptor.PS || CS != _descriptor.CS;
    }
    
    std::vector<ShaderInput> GetInputs() const;
    
    void Free();
protected:
    std::vector<ShaderInput> m_entries;
    GPU_HANDLE VS = {};
    GPU_HANDLE PS = {};
    GPU_HANDLE CS = {};
};
